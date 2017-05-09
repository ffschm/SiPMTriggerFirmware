volatile unsigned long irq_count;
const int input_pin = 2;

unsigned long integration_time = 1000;

const size_t channels = 4;
byte threshold[channels] = {128, 128, 128, 128};

typedef enum {
  constant,
  scanning,
  updated
} mode_t;

mode_t mode[channels] = {constant};

//#include <commands.ino>
void setup_potentiometer();
uint16_t sendCommand(byte control, byte address, byte data);

void setup() {
  Serial.begin(9600);
  Serial.println("SiPMTrigger v4 Control v0.2");

  setup_potentiometer();
  setup_commands();

  attachInterrupt(digitalPinToInterrupt(input_pin), IRQCounter, FALLING);
}

void loop() {
  handle_commands();
  set_thresholds();
  const unsigned long counts = count_interrupts();
  print_interrupts(counts);
}

void set_thresholds() {
  for (size_t channel = 0; channel < 4; channel++) {
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
  }
}

unsigned long count_interrupts() {
  cli();
  irq_count = 0;
  sei();
  
  delay(integration_time);
  
  cli();
  const unsigned long _irq_count = irq_count;
  sei();

  return _irq_count;
}

void print_interrupts(const unsigned long counts) {

  for (size_t channel = 0; channel < channels; channel++) {
    Serial.print(threshold[channel]);
    Serial.print(",");
  }
  Serial.println(counts);
}

void IRQCounter() {
  irq_count++;
}
