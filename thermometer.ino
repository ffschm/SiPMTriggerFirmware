#include <OneWire.h>
#include <DallasTemperature.h>

// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 42

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

// arrays to hold device address
DeviceAddress insideThermometer;

const int temp_resolution = 12;


void setup_thermometer() {
    sensors.begin();
    if(sensors.getDeviceCount() != 1) {
        Serial.println("# No temperature sensor found.");
    }

    DeviceAddress tempDeviceAddress;
    sensors.getAddress(tempDeviceAddress, 0);
    sensors.setResolution(tempDeviceAddress, temp_resolution);
}

void update_temperature() {
    sensors.requestTemperatures();
    temperature = sensors.getTempCByIndex(0);
}
