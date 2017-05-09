String inputString = "";
boolean stringComplete = false;

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
    if (inputString.startsWith("SETTIME")) {
      const long time = getValue(inputString.substring(8), ',', 0).toInt();
      integration_time = time;
    } else if (inputString.startsWith("SET")) {
      // set potentiometer values
      const byte channel = (byte) (getValue(inputString.substring(4), ',', 0).toInt());
      const byte value = (byte) (getValue(inputString.substring(4), ',', 1).toInt());
      if (0 <= channel && channel <= 4) {
        if (0 <= value  && value <= 255) {
          mode[channel] = updated;
          threshold[channel] = value;
        } else {
          Serial.print("# SET command failed: Invalid value.");
        }
      } else {
        Serial.print("# SET command failed: Invalid channel.");
      }
    } else if (inputString.startsWith("SCAN")) {
      const byte channel = (byte) (getValue(inputString.substring(4), ',', 0).toInt());
      if (0 <= channel && channel <= 4) {
        mode[channel] = scanning;
      } else {
        Serial.print("# SCAN command failed: Invalid channel.");
      }
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
