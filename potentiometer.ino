#include <SPI.h>


const int slaveSelectPin = 53;
const SPISettings spi_settings = SPISettings(1e6, MSBFIRST, SPI_MODE1);

uint16_t sendCommand(byte control, byte address, byte data) {
  uint16_t dataToSend = control << 12 | address << 8 | data;

  SPI.beginTransaction(spi_settings);
  digitalWrite(slaveSelectPin, LOW);
  uint16_t result = SPI.transfer16(dataToSend);
  digitalWrite(slaveSelectPin, HIGH);
  SPI.endTransaction();
  delayMicroseconds(40);

  return result;
}

void setup_potentiometer() {
  // initialize SPI
  SPI.begin();
  pinMode (slaveSelectPin, OUTPUT);
}
