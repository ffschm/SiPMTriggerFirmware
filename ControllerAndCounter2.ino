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
  unsigned long integration_time;
  unsigned long lcd_interval;
  unsigned long serial_interval;

  bool display_enabled;
};

global_settings settings = {1000, 1000, 1000, false};


// Declare variables for measured data
volatile unsigned long irq_count;
unsigned long counts = 0;
float humidity = 0, temperature = 0, pressure = 0;

// Declare counter for periodic updates when idling
unsigned long last_lcd_update = 0;
unsigned long last_serial_update = 0;

// Discriminator settings
byte threshold[signal_channels] = {128, 128};

/* Convenience variables to calculate the discriminator thresholds
 * from the number of photoelectrons
 */
double offset[signal_channels] = {0, 0};
double gain[signal_channels] = {40, 40};


/* Define the global mode of the micrcontroller.
 * When idling, wait for commands and print measurements every second.
 * When scanning, wait for commands and scan the thresholds as requested via a serial command.
 */
typedef enum {
  idling,
  scanning,
} mode_t;

mode_t mode = idling;


/* Define variables required for scanning the discrimintor threshold.
 *
 * During a scan the array `spectrum` and `spectrum_error` gets filled
 * with data, starting with a threshold of 0 photoelectrons.
 * In each step, the variable `spectrum_i` is incremented by one and
 * the threshold is increased by 1/`gain` photoelectrons, until the
 * highest possible threshold is reached. In that case the measured
 * spectrum gets printed on the serial console and the global mode is
 * set to `idling` again.
 */

size_t spectrum_i = 0;
double spectrum[256];
double spectrum_error[256];

// Declare driver variables for external devices
AD5144 poti(channels, 53);
RTC_DS1307 rtc;
LiquidCrystal lcd(30, 32, 34, 36, 38, 40);
LiquidCrystal lcd1(31, 33, 35, 37, 39 , 41);

#define SW_VERSION "0.7pre"


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

/* ###################
 * #Command handlers #
 * ################### */

void command_set_time() {
  const long time = parse_integer<long>();

  if (time != NULL) {
    settings.serial_interval = time;
    settings.integration_time = time;
    FreqCount.end();
    FreqCount.begin(settings.integration_time);
  }
}

void command_set_thr() {
  const byte channel = parse_integer<byte>(1, signal_channels);
  const byte value = parse_integer<byte>(0, 255);

  set_threshold(channel - 1, value);
}

void command_scan_thr() {
  const int result1 = set_pe_threshold(0, 0);
  const int result2 = set_pe_threshold(1, 0);

  if (!result1 || !result2) {
    Serial.println("Error: Can't start threshold can, 0p.e. is already out of bounds.");
    Serial.println("Check gain and offset and try again.");
  }

  spectrum_i = 0;
  mode = scanning;
}

void command_set_gain() {
  const byte channel = parse_integer<byte>(1, signal_channels);
  const double value = parse_double();

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
  if (!success) {
    Serial.println("Error: Requested treshold is out of range, check gain and offset and try again.");
  }
}

void command_get_temperature() {
  Serial.print("OO");
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
  sCmd.addCommand("SET PE THR", command_set_pe_thr);
  sCmd.addCommand("SCAN THR", command_scan_thr);

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


/* ###########################
 * # Serial output functions #
 * ########################### */

/* Print measurements while idling */
void print_interrupts() {
  // TODO: Not implemented.
}

void print_spectrum() {
  // TODO: Not implemented.
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
    lcd.print(counts * (1000 / (double) settings.integration_time));
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

  FreqCount.begin(settings.integration_time);
  Serial.println("# CH1(THR) CH2(THR) Counts sqrt(Counts)");
}


/* #################
 * # Loop function #
 * ################# */

void loop() {
  handle_commands();

  if(mode == idling) {
    if (FreqCount.available()) {
      counts = FreqCount.read();
    }

    const unsigned long current_millis = millis();

    if (settings.display_enabled && current_millis - last_lcd_update >= settings.lcd_interval) {
      last_lcd_update = current_millis;
      update_lcd();
    }

    if (current_millis - last_serial_update >= settings.serial_interval) {
      last_serial_update = current_millis;
      print_interrupts();
    }
  } else if (mode == scanning) {
    // TODO: Not implemented.
    if (FreqCount.available()) {
      counts = FreqCount.read();
    }
    spectrum[spectrum_i] = counts;

    spectrum_i += 1;
    const double global_pe = spectrum_i * 1.0 / min(gain[0], gain[1]);
    const int result1 = set_pe_threshold(0, global_pe);
    const int result2 = set_pe_threshold(1, global_pe);
    if (!result1 || !result2) {
      spectrum_i -= 1;
      // At least one channel is out of bounds, spectrum scan finished.
      Serial.println("# Spectrum scan finished.");
      print_spectrum();

      mode = idling;
    }
  }
}
