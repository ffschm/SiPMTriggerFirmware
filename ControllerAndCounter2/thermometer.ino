#include "SparkFunBME280.h"

BME280 BME;



void setup_thermometer() {
  //commInterface can be I2C_MODE or SPI_MODE
  BME.settings.commInterface = I2C_MODE;
  BME.settings.I2CAddress = 0x76; //77 for Bosch, 76 for Bosch-Fake
  BME.settings.runMode = 3; //  3, Normal mode
  BME.settings.tStandby = 0; //  0, 0.5ms
  BME.settings.filter = 0; //  0, filter off

  //tempOverSample can be:
  //  0, skipped
  //  1 through 5, oversampling *1, *2, *4, *8, *16 respectively
  BME.settings.tempOverSample = 1;

  //pressOverSample can be:
  //  0, skipped
  //  1 through 5, oversampling *1, *2, *4, *8, *16 respectively
  BME.settings.pressOverSample = 1;

  //humidOverSample can be:
  //  0, skipped
  //  1 through 5, oversampling *1, *2, *4, *8, *16 respectively
  BME.settings.humidOverSample = 1;

  BME.begin();
}

void update_temperature() {
  humidity = BME.readFloatHumidity();
  temperature = BME.readTempC();
  pressure = BME.readFloatPressure();
}

void print_temperature() {
  Serial.print("# T = ");
  Serial.print(temperature);
  Serial.println(" degC");

  Serial.print("# hum = ");
  Serial.print(humidity);
  Serial.println(" %");

  Serial.print("# pressure = ");
  Serial.print(pressure / 100.0);
  Serial.println(" hPa");
}
