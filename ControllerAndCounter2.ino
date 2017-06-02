#include <FreqCount.h>
#include "mapping.h"
#include "RTClib.h"
#include <LiquidCrystal.h>
#include <DallasTemperature.h>


volatile unsigned long irq_count;
unsigned long counts = 0;
float temperature = DEVICE_DISCONNECTED_C;
unsigned long last_temp_request = 0;
unsigned long last_lcd_update = 0;
unsigned long last_serial_update = 0;

// const int input_pin = 47; (set by FreqCount)

unsigned long integration_time = 1000;
unsigned long temp_interval = 10000;
unsigned long lcd_interval = 1000;
unsigned long serial_interval = 1000;

const size_t channels = 4;
byte threshold[channels] = {128, 128, 128, 128};
const size_t signal_channels = 2;
double gain[signal_channels] = {40, 40};
double scanning_width = 0.1;

typedef enum {
  constant,
  pe_scanning,
  scanning,
  updated
} mode_t;

mode_t mode[channels] = {constant};

//#include <commands.ino>
void setup_potentiometer();
uint16_t sendCommand(byte control, byte address, byte data);

RTC_DS1307 rtc;
LiquidCrystal lcd(30, 32, 34, 36, 38, 40);
LiquidCrystal lcd1(31, 33, 35, 37, 39 , 41);

#define SW_VERSION "0.6"

void setup() {
  Serial.begin(9600);
  FreqCount.begin(integration_time);
  Serial.print("# SiPMTrigger v4 Control v");
  Serial.println(SW_VERSION);
  Serial.println("# CH1(p_width) CH1(THR) CH2(THR) CH1(p_width) CH1(THR/pe) CH2(THR/pe) counts");
  setup_potentiometer();
  setup_commands();
  setup_rtc();
  setup_lcd();
  setup_thermometer();
  update_temperature();
}

void loop() {
  handle_commands();

  if (FreqCount.available()) {
    counts = FreqCount.read();
  }

  const unsigned long current_millis = millis();

  if (current_millis - last_temp_request >= temp_interval) {
    last_temp_request = current_millis;
    update_temperature();
    print_temperature();
  }

  if (current_millis - last_lcd_update >= lcd_interval) {
    last_lcd_update = current_millis;
    update_lcd();
  }

  if (current_millis - last_serial_update >= serial_interval) {
    last_serial_update = current_millis;
    set_thresholds();
    print_interrupts();
  }
}

void set_thresholds() {
  for (size_t channel = 0; channel < channels; channel++) {
    if (mode[channel] == scanning) {
      threshold[channel]++;
      // Write contents of serial register data to RDAC
      sendCommand(1, channel, threshold[channel]);
    }
    else if (mode[channel] == updated) {
      // Write contents of serial register data to RDAC
      sendCommand(1, channel, threshold[channel]);
      mode[channel] = constant;
    }
    else if (mode[channel] == pe_scanning) {
      const double thr = scanning_width * gain[channel];

      if (0 <= thr && thr <= 255) {
        threshold[channel] += thr;
        // Write contents of serial register data to RDAC
        sendCommand(1, channel, threshold[channel]);
      } else {
        for (size_t i = 0; i < signal_channels; i++) {
          mode[i] = constant;
        }
      }
    }
  }
}

void print_interrupts() {
  for (size_t channel = 0; channel < channels; channel++) {
    Serial.print(threshold[channel]);
    Serial.print(" ");
  }

  for (size_t channel = 0; channel < signal_channels; channel++) {
    Serial.print(1.0*threshold[threshold_channel[channel]]/gain[channel]);
    Serial.print(" ");
  }

  Serial.print(counts*(1000/integration_time));
  Serial.print(" ");
  Serial.println(sqrt(counts*(1000/integration_time)));
}

void print_temperature() {
  Serial.print("# T ");
  Serial.print(temperature);
  Serial.print(' ');
  Serial.print((char)176);
  Serial.println("C");
}

void lcd_print_interrupts() {
    lcd.setCursor(0, 1);
    lcd.print("CH1 ");
    lcd.print(threshold[threshold_channel[0]]);
    lcd.print("  CH2 ");
    lcd.print(threshold[threshold_channel[1]]);

    lcd.setCursor(0, 2);
    lcd.print("Rate ");
    lcd.print(counts*(1000/integration_time));
    lcd.print(" Hz");

    lcd.setCursor(0, 3);
    lcd.print("T ");
    lcd.print(temperature);
    lcd.print((char)223);
    lcd.print("C");
}
