#include <SerialCommand.h>
#include <limits.h>

SerialCommand sCmd;

// Mapping for SiPMTrigger v4
enum pot_channels {
  CH1_THR = 2,
  CH2_THR = 1,
  CH1_WIDTH = 0,
  CH2_WIDTH = 3
};
const byte threshold_channel[2] = {2, 1};
const byte width_channel[2] = {0, 3};


void setup_commands() {
  sCmd.addCommand("SET TIME", command_set_time);
  sCmd.addCommand("SET THR", command_set_thr);
  sCmd.addCommand("SET WIDTH", command_set_width);
  sCmd.addCommand("SCAN THR", command_scan_thr);
  sCmd.setDefaultHandler(command_unrecognized);
}

void handle_commands() {
  sCmd.readSerial();
}

template<typename T> parse_integer(const T min=0,          // std::numeric_limits<T>::min(),
                                   const T max=LONG_MAX) { //std::numeric_limits<T>::max()) {
  const char *arg = sCmd.next();
  if (arg == NULL) {
    Serial.println("# command failed: Argument missing.");
    return NULL;
  }

  T value;
  sscanf(arg, "%d", &value);

  if (min <= value && value <= max) {
    return value;
  } else {
    Serial.println("# command failed: Invalid argument.");
    return NULL;
  }
}

void command_set_time() {
  const long time = parse_integer<long>();
  if (time != NULL) {
    integration_time = time;
    FreqCount.end();
    FreqCount.begin(integration_time);
  }
}

void command_set_thr() {
  const byte channel = parse_integer<byte>(1, 2);
  const byte value = parse_integer<byte>(0, 255);

  mode[threshold_channel[channel-1]] = updated;
  threshold[threshold_channel[channel-1]] = value;
}

void command_set_width() {
  const byte channel = parse_integer<byte>(1, 2);
  const byte value = parse_integer<byte>(0, 255);

  mode[width_channel[channel-1]] = updated;
  threshold[width_channel[channel-1]] = value;
}

void command_scan_thr() {
  mode[CH1_THR] = scanning;
  mode[CH2_THR] = scanning;
}

void command_unrecognized(const char *c) {
  Serial.print("# Unknown command: ");
  Serial.println(c);
}
