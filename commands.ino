String inputString = "";
boolean stringComplete = false;

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
  // Reserve bytes for the input string
  inputString.reserve(20);
}

// Source: https://arduino.stackexchange.com/a/1237
String getValue(String data, char separator, int index)
{
    int found = 0;
    int strIndex[] = { 0, -1 };
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void handle_commands() {
  if (stringComplete) {
    if (inputString.startsWith("SET TIME")) {
      const long time = getValue(inputString.substring(8), ',', 0).toInt();
      integration_time = time;
      FreqCount.end();
      FreqCount.begin(integration_time);
    } else if (inputString.startsWith("SET THR")) {
      const byte channel = (byte) (getValue(inputString.substring(8), ',', 0).toInt());
      const byte value = (byte) (getValue(inputString.substring(8), ',', 1).toInt());
      if (1 <= channel && channel <= 2) {
        if (0 <= value  && value <= 255) {
          mode[threshold_channel[channel-1]] = updated;
          threshold[threshold_channel[channel-1]] = value;
        } else {
          Serial.print("# SET command failed: Invalid value.");
        }
      } else {
        Serial.print("# SET command failed: Invalid channel.");
      }
    } else if (inputString.startsWith("SET WIDTH")) {
      const byte channel = (byte) (getValue(inputString.substring(10), ',', 0).toInt());
      const byte value = (byte) (getValue(inputString.substring(10), ',', 1).toInt());
      if (1 <= channel && channel <= 2) {
        if (0 <= value  && value <= 255) {
          mode[width_channel[channel-1]] = updated;
          threshold[width_channel[channel-1]] = value;
        } else {
          Serial.print("# SET command failed: Invalid value.");
        }
      } else {
        Serial.print("# SET command failed: Invalid channel.");
      }
    } else if (inputString.startsWith("SCAN THR")) {
        mode[CH1_THR] = scanning;
        mode[CH2_THR] = scanning;
    }
    inputString = "";
    stringComplete = false;
  }
}

void serialEvent() {
  while (Serial.available()) {
    char inChar = (char) Serial.read();
    inputString += inChar;

    if (inChar == '\n') {
      stringComplete = true;
    }
  }
}
