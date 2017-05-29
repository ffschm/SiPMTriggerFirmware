#include <FreqCount.h>
#include "mapping.h"

volatile unsigned long irq_count;
unsigned long counts_sum = 0;

// const int input_pin = 47; (set by FreqCount)

unsigned long integration_time = 1000;

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

void setup() {
  Serial.begin(9600);
  FreqCount.begin(integration_time);
  Serial.println("SiPMTrigger v4 Control v0.4");

  setup_potentiometer();
  setup_commands();
}

void loop() {
  handle_commands();
  set_thresholds();
  while (!FreqCount.available()) {
    delay(1);
  }
  const unsigned long count = FreqCount.read();

  print_interrupts(count);
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

void print_interrupts(const unsigned long counts) {
  counts_sum += counts;

  for (size_t channel = 0; channel < channels; channel++) {
    Serial.print(threshold[channel]);
    Serial.print(",");
  }

  for (size_t channel = 0; channel < signal_channels; channel++) {
    Serial.print(1.0*threshold[threshold_channel[channel]]/gain[channel]);
    Serial.print(",");
  }

  Serial.println(counts*(1000/integration_time));
}
