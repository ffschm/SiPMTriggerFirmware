#include <AD5144.h>
#include <FreqCount.h>
#include <RTClib.h>
#include <LiquidCrystal.h>
#include <DallasTemperature.h>
#include <AnotherSerialCommand.h>
#include <limits.h>
#include <float.h>


// Define board properties (currently for SiPMTrigger v4)
const size_t channels = 4;
const size_t signal_channels = 2;

enum pot_channel {
  CH1_THR = 2,
  CH2_THR = 1,
  CH1_WIDTH = 0,
  CH2_WIDTH = 3
};

// Define arduino settings
struct global_settings {
  unsigned long min_integration_time;
  unsigned long max_integration_time;
  double desired_rel_freq_error;
  bool dynamic_integration_time;

  unsigned long lcd_interval;
  bool display_enabled;
};

global_settings settings = {.min_integration_time = 100,
                            .max_integration_time = 5000,
                            .desired_rel_freq_error = 0.04,
                            .dynamic_integration_time = true,
                            .lcd_interval = 1000,
                            .display_enabled = false};


// Declare variables for measured data
volatile unsigned long irq_count;
unsigned long counts = 0;
float humidity = 0, temperature = 0, pressure = 0;
unsigned long integration_time = 1000;

// Declare counter for periodic updates when idling
unsigned long last_lcd_update = 0;
unsigned long last_serial_update = 0;

// Discriminator settings
byte threshold[signal_channels] = {128, 128};

/* Convenience variables to calculate the discriminator thresholds
 * from the number of photoelectrons
 */
double offset[signal_channels] = {0, 0};
double gain[signal_channels] = {1, 1};


/* Define the global mode of the micrcontroller.
 * When idling, wait for commands and print measurements every second.
 * When scanning, wait for commands and scan the thresholds as requested via a serial command.
 */
typedef enum {
  idling,
  scanning_single,
  scanning,
} mode_t;

mode_t mode = idling;

// Defines which channel get scanned during a single channel scan.
size_t scanning_channel = 0;

/* Define variables required for scanning the discrimintor threshold.
 *
 * During a scan the array `spectrum` and `spectrum_error` gets filled
 * with data, starting with a threshold of `min` photoelectrons.
 * In each step, the variable `spectrum_i` is incremented by one and
 * the threshold is increased by `step` photoelectrons, until the
 * highest possible threshold or `max` photolectrons is reached. In that
 * case the measured spectrum gets printed on the serial console and the
 * global mode is set to `idling` again.
 */
struct spectrum {
  size_t i;
  double freq[256];
  double freq_err[256];
  double step;
  double min;
  double max;
};

spectrum spectrum0 = {.i = 0,
                      .freq = {0},
                      .freq_err = {0},
                      .step = 0.1,
                      .min = 0,
                      .max = -1};


// Declare driver variables for external devices
AD5144 poti(channels, 53);
RTC_DS1307 rtc;
LiquidCrystal lcd(30, 32, 34, 36, 38, 40);
LiquidCrystal lcd1(31, 33, 35, 37, 39 , 41);

#define SW_VERSION "0.7"


/* ###################
 * # Serial Commands #
 * ################### */

SerialCommand sCmd;

/* Parses the serial input buffer and executes the function associated with the parsed command. */
void handle_commands() {
  sCmd.readSerial();
}

/* Read the next token received via serial port and parses it as a variable of type integer */
template<typename T> parse_integer(const T min=0,
                                   const T max=LONG_MAX) {
  const char *arg = sCmd.next();
  if (arg == NULL) {
    Serial.println("# command failed: Argument missing.");
    return NULL;
  }

  T value;
  value = strtol(arg, NULL, 10);

  if (min <= value && value <= max) {
    return value;
  } else {
    Serial.println("# command failed: Invalid argument.");
    return NULL;
  }
}

/* Read the next token received via serial port and parses it as a variable of type double */
double parse_double(const double min=-DBL_MAX,
                    const double max=DBL_MAX) {
  const char *arg = sCmd.next();
  if (arg == NULL) {
    Serial.println("# command failed: Argument missing.");
    return NULL;
  }

  double value;
  value = strtod(arg, NULL);

  if (min <= value && value <= max) {
    return value;
  } else {
    Serial.print("# command failed: Invalid argument [");
    Serial.print(value);
    Serial.println("].");
    return NULL;
  }
}

/* Returns true if at least one byte is received on the serial input, false otherwise */
bool serial_byte_received() {
  size_t bytes = Serial.available();
  if (bytes > 0) {
    /* Abort the threshold scan when receiving any byte on the serial port. */
    while (bytes > 0) {
      Serial.read();
      bytes--;
    }
    return true;
  }
  return false;
}

/* ###################
 * #Command handlers #
 * ################### */

void set_integration_time(const unsigned long time) {
    integration_time = time;
    FreqCount.end();
    FreqCount.begin(integration_time);
}

void adjust_integration_time() {
  /* Adjust the integration time based on the last measured frequency.
   * The new integration time is calculated such that the relative error
   * of the next frequency measurement will equal `desired_rel_freq_error`.
   *
   * If the calculated integration time is lower/higher than
   * `max_integration_time`/`min_integration_time` it is capped to the
   * respective value.
   *
   * If the last measurement didn't capture a single event,
   * the integration time is also set to `max_integration_time`.
   */
  unsigned long new_time = integration_time;

  if (counts == 0) {
      new_time = settings.max_integration_time;
  } else {
      new_time = (1 / settings.desired_rel_freq_error) * (integration_time / sqrt(counts));

      new_time = max(new_time, settings.min_integration_time);
      new_time = min(new_time, settings.max_integration_time);
  }

  set_integration_time(new_time);
  Serial.print("# new integration time = ");
  Serial.println(integration_time);
}

void command_set_time() {
  const long time = parse_integer<long>();

  if (time != NULL) {
    set_integration_time(time);
  }
}

void command_set_thr() {
  const byte channel = parse_integer<byte>(1, signal_channels);
  const byte value = parse_integer<byte>(0, 255);

  set_threshold(channel - 1, value);
}

void command_scan_thr() {
  const byte result = parse_integer<byte>(1, signal_channels);
  if (result == NULL) {
    return;
  }

  // channel 1/2 is the internal channel 0/1.
  const byte channel = result - 1;

  // spectrum_step = 1.0 / min(gain[0], gain[1]);
  spectrum0.step = 1;
  spectrum0.min = 0;
  spectrum0.max = 255;
  spectrum0.i = 0;
  scanning_channel = channel;
  mode = scanning_single;

  // Start scanning
  set_threshold(channel, spectrum0.min);

  // Disable the other channel during the scan to prevent cross-talk
  if (scanning_channel == 1) {
    set_threshold(0, 255);
  } else {
    set_threshold(1, 255);
  }

  Serial.println("# Threshold scan for a single channel started. Please wait...");
}

void command_scan_pe_thr() {
  const double min_offset = -min(offset[0]/gain[0],offset[1]/gain[1]);
  const int result1 = set_pe_threshold(0, min_offset);
  const int result2 = set_pe_threshold(1, min_offset);

  if (result1 != 0 || result2 != 0) {
    Serial.println("# Error: Can't start threshold scan, 0p.e. is already out of bounds.");
    Serial.println("# Check gain and offset and try again.");
  }

  // Start scan from 0 p.e. up to the highest possible threshold in fixed steps
  // spectrum0.step = 1.0 / min(gain[0], gain[1]);
  spectrum0.step = 0.1;
  spectrum0.min = min_offset;
  spectrum0.max = DBL_MAX;
  spectrum0.i = 0;
  mode = scanning;
  Serial.println("# Threshold scan started. Please wait...");
}

void command_set_gain() {
  const byte channel = parse_integer<byte>(1, signal_channels);
  const double value = parse_double();

  if (value == 0) {
    Serial.println("# Command failed: Can't set gain to zero.");
    return;
  }
  gain[channel - 1] = value;
}

void command_set_offset() {
  const byte channel = parse_integer<byte>(1, signal_channels);
  const double value = parse_double();

  offset[channel - 1] = value;
}

void command_set_pe_thr() {
  const byte channel = parse_integer<byte>(1, signal_channels);
  const double value = parse_double();

  const int success = set_pe_threshold(channel - 1, value);
  if (success != 0) {
    Serial.println("# Error: Requested treshold is out of range, check gain and offset and try again.");
  }
}

void command_get_temperature() {
  update_temperature();
  print_temperature();
}

void command_unrecognized(const char *c) {
  Serial.print("# Unknown command: ");
  Serial.println(c);
}

void setup_commands() {
  /* Associate the available commands with their respective function handles */
  sCmd.addCommand("SET TIME", command_set_time);
  sCmd.addCommand("SET GAIN", command_set_gain);
  sCmd.addCommand("SET OFFSET", command_set_offset);

  sCmd.addCommand("SET THR", command_set_thr);
  sCmd.addCommand("SCAN THR", command_scan_thr);
  sCmd.addCommand("SET PE THR", command_set_pe_thr);
  sCmd.addCommand("SCAN PE THR", command_scan_pe_thr);

  sCmd.addCommand("GET TEMP", command_get_temperature);
  sCmd.setDefaultHandler(command_unrecognized);
}


/* ###########################
 * # Discriminator functions #
 * ########################### */

/* Set the discriminator threshold of the given channel by adjusting the digital potentiometer */
void set_threshold(const size_t channel, const byte value) {
  if(channel == 0) {
    poti.set_value(CH1_THR, value);
  } else if (channel == 1) {
    poti.set_value(CH2_THR, value);
  }

  threshold[channel] = value;

  // Wait 35us until the switching noise is gone
  delayMicroseconds(35);

  // Start a new frequency measurement
  // (prevents threshold changes to take place during a single measurement)
  FreqCount.end();
  FreqCount.begin(integration_time);
}


/* Set the discriminator threshold of the given channel by adjusting the digital potentiometer,
 * the treshold is calculated from a given number of photoelectrons using the stored offset and gain
 * of the given channel.
 * THR(channel) = offset(channel) + gain(channel) * no. of photoelectrons
 * Returns 0 on success, -1 if the requested treshold is out of range.
 */
int set_pe_threshold(const size_t channel, const double photoelectrons) {

  const double thr = offset[channel] + gain[channel] * photoelectrons;

  if(thr < 0 || thr > 255) {
    return -1;
  }

  set_threshold(channel, (byte) thr);
  return 0;
}

// Return the treshold of the given channel in photoelectrons, based on gain and offset.
double effective_thr(const size_t channel) {
  return ((double) threshold[channel] - offset[channel]) / gain[channel];
}


/* ###########################
 * # Serial output functions #
 * ########################### */

/* Print measurements while idling */
void print_interrupts() {
  Serial.print(threshold[0]);
  Serial.print(" ");
  Serial.print(threshold[1]);
  Serial.print("  ");

  Serial.print(effective_thr(0));
  Serial.print(" ");
  Serial.print(effective_thr(1));
  Serial.print("  ");

  const double freq = counts * (1000 / (double) integration_time);
  const double freq_err = (1000 / (double) integration_time ) * sqrt(counts);
  Serial.print(freq);
  Serial.print(" ");
  Serial.println(freq_err);
}

void print_spectrum() {
  Serial.println("## THR = OFFSET + GAIN * (No. of photolelectrons)");
  Serial.println("## Gain_CH1 Gain_CH2 Offset_CH1 Offset_CH2");
  Serial.print("## ");
  Serial.print(gain[0]);
  Serial.print(" ");
  Serial.print(gain[1]);
  Serial.print(" ");
  Serial.print(offset[0]);
  Serial.print(" ");
  Serial.println(offset[1]);

  Serial.println("# THR/p.e. R/Hz deltaR/Hz");
  for (size_t i = 0; i <= spectrum0.i; i++) {
    Serial.print(spectrum0.min + spectrum0.step * i);
    Serial.print(" ");
    Serial.print(spectrum0.freq[i]);
    Serial.print(" ");
    Serial.println(spectrum0.freq_err[i]);
  }
  Serial.println("# END Spectrum.");
}


/* #################
 * # LCD functions #
 * ################# */

void lcd_print_interrupts() {
    lcd.setCursor(0, 1);
    lcd.print("CH1 ");
    lcd.print(threshold[0]);
    lcd.print("  CH2 ");
    lcd.print(threshold[1]);

    lcd.setCursor(0, 2);
    lcd.print("Rate ");
    lcd.print(counts * (1000 / (double) integration_time));
    lcd.print(" Hz");

    lcd.setCursor(0, 3);
    lcd.print("T ");
    lcd.print(temperature);
    lcd.print((char) 223);
    lcd.print("C");
}


/* ##################
 * # Setup function #
 * ################## */

void setup() {
  Serial.begin(9600);
  Serial.print("# SiPMTrigger v4 Control v");
  Serial.println(SW_VERSION);

  poti.begin();
  setup_commands();
  setup_rtc();
  setup_lcd();
  setup_thermometer();
  update_temperature();

  FreqCount.begin(integration_time);
  Serial.println("# CH1(THR) CH2(THR)  CH1(THR/pe) CH2(THR/pe)  counts sqrt(counts)");
}


/* #################
 * # Loop function #
 * ################# */

void loop_idling() {
  handle_commands();

  if (FreqCount.available()) {
    counts = FreqCount.read();
  }

  const unsigned long current_millis = millis();

  if (settings.display_enabled && current_millis - last_lcd_update >= settings.lcd_interval) {
    last_lcd_update = current_millis;
    update_lcd();
  }

  if (current_millis - last_serial_update >= integration_time) {
    last_serial_update = current_millis;
    print_interrupts();
  }
}

void loop_scanning_single() {
  if(serial_byte_received()) {
    /* Abort scan if any byte received via serial input. */
    Serial.println("# Threshold scan aborted.");
    print_spectrum();

    mode = idling;
    return;
  }
  if (FreqCount.available()) {
    /* Store the latest frequency measurement and start a new measurement with updated threshold */
    counts = FreqCount.read();

    const double freq = counts * (1000 / (double) integration_time);
    spectrum0.freq[spectrum0.i] = freq;
    spectrum0.freq_err[spectrum0.i] = (1000 / (double) integration_time ) * sqrt(counts);

    print_interrupts();

    spectrum0.i += 1;
    set_threshold(scanning_channel, spectrum0.i);
    if (spectrum0.i > spectrum0.max) {
      spectrum0.i -= 1;
      // The next threshold is out of bounds, spectrum scan finished.
      Serial.println("# Threshold scan finished.");

      mode = idling;
    }
  }
}

void loop_scanning() {
  if(serial_byte_received()) {
    /* Abort scan if any byte received via serial input. */
    Serial.println("# Threshold scan aborted.");
    print_spectrum();

    mode = idling;
    return;
  }

  if (FreqCount.available()) {
    /* Store the latest frequency measurement and start a new measurement with updated threshold */
    counts = FreqCount.read();

    const double freq = counts * (1000 / (double) integration_time);
    spectrum0.freq[spectrum0.i] = freq;
    spectrum0.freq_err[spectrum0.i] = (1000 / (double) integration_time ) * sqrt(counts);

    print_interrupts();

    spectrum0.i += 1;
    const double global_pe = spectrum0.min + spectrum0.i * spectrum0.step;
    const int result1 = set_pe_threshold(0, global_pe);
    const int result2 = set_pe_threshold(1, global_pe);

    if (global_pe > spectrum0.max || result1 != 0 || result2 != 0) {
      spectrum0.i -= 1;
      // At least one channel is out of bounds, spectrum scan finished.
      Serial.println("# Threshold scan finished.");
      print_spectrum();

      mode = idling;
    }

    if (settings.dynamic_integration_time) {
      adjust_integration_time();
    }
  }
}

void loop() {
  if(mode == idling) {
    loop_idling();
  } else if (mode == scanning_single) {
    loop_scanning_single();
  } else if (mode == scanning) {
    loop_scanning();
  }
}
