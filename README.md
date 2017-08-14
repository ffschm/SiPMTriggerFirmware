# SiPM Trigger Controller&Counter
This firmware allows an arduino to be used as a controller of the digital potentiometer on the SiPMTrigger board
as well as a simple scaler.
Please note: The combination of slow control and data aquisition has several drawbacks but can be choosen if handled carefully.

Currently there are two versions of the SiPMTrigger board (`Scinti_Readout v1` and `Scinti_Readout v2`). Due to different channel mappings,
each board requires its own firmware. Please edit the line `#define SCINTI_READOUT_HW_VERSION 2` according to your needs.

## Installation

This sketch depends on the following libraries:
- [FreqCount](https://github.com/PaulStoffregen/FreqCount)
- [RTClib](http://www.tinkbox.ph/sites/tinkbox.ph/files/downloads/RTClib.zip)
- [LiquidCrystal](https://www.arduino.cc/en/Reference/LiquidCrystal) (included in the Arduino IDE)
- [OneWire](https://github.com/PaulStoffregen/OneWire)
- [DallasTemperature](https://github.com/milesburton/Arduino-Temperature-Control-Library)
- [AnotherSerialCommand](https://github.com/ffschm/Arduino-AnotherSerialCommand)
- [SPI](https://www.arduino.cc/en/Reference/SPI) (included in the Arduino IDE)
- [SparkFun BME280](https://github.com/sparkfun/SparkFun_BME280_Arduino_Library)
- [AD5144](https://github.com/ffschm/Arduino-AD5144-library)

## Pin configuration

In the follwing table the arduino pins used by this sketch are listed.

| Arduino pin | Pin Mode | Function |
| ----------- | ---------| -------- |
| 47 | input | Frequency counter input |
| 53 | output | Chip Select to SiPM Trigger board (potentiometer IC) |
| MISO, MOSI, SCK, +5V, GND (ICSP header) | both | SPI to SiPM Trigger board |
| SDA, SCL, +3.3V, GND | both | I2C to BME280 environmental sensor |

## Commands

| Comand | Description |
| ------ | ----------- |
| `SET THR $CH, $THR`| Set the threshold of signal channel `$CH` to `$THR`. |
| `SET OFFSET $CH, $VAL`| Set the offset of signal channel `$CH` to `$VAL`. |
| `SET GAIN $CH, $VAL`| Set the gain of signal channel `$CH` to `$VAL`. |
| `SET PE THR $CH, $PETHR`| Set the threshold of signal channel `$CH` to `$PETHR * $GAIN`. |
| `SCAN THR $CH`| Scan the threshold of signal channel `CH` by increasing the threshold by 1LSB per step. |
| `SCAN PE THR`| Scan the thresholds of both signal channels simultaneously from 0p.e. up to the highest possible treshold by increasing them by 0.1p.e. at each tick. |
| `SET TIME $INTTIME`| Set the integrating time interval to `$INTTIME` (in milliseconds). |
| `GET TEMP`| Measure the temperature (&humidity&pressure) and print it via the serial console. |

## Output

Example:
```
# SiPMTrigger v4 Control v0.7pre
# RTC is NOT running!
# CH1(THR) CH2(THR)  CH1(THR/pe) CH2(THR/pe)  counts sqrt(counts)
128 128  128.00 128.00  0.00 0.00
128 128  128.00 128.00  0.00 0.00
128 128  128.00 128.00  0.00 0.00
128 128  128.00 128.00  0.00 0.00
8 0  8.00 0.00  2273990.00 1507.98
8 0  8.00 0.00  2263700.00 1504.56
```

The output contains the threshold settings of the discriminator for both signal channels, as well as the trigger rate with error of the signal on pin 47 of the arduino in the last integration time interval.

## Hardware

The following table shows the mapping of internal potentiometer channels to their function on the different boards.

### `Scinit_Readout v1`

| Potentiometer channel | Function |
| --------------------- | -------- |
| channel 0             | signal channel 1 pulse width |
| channel 1             | signal channel 2 threshold |
| channel 2             | signal channel 1 threshold |
| channel 3             | signal channel 2 pulse width |

### `Scinit_Readout v2`

| Potentiometer channel | Function |
| --------------------- | -------- |
| channel 0             | signal channel 1 threshold |
| channel 1             | signal channel 2 threshold |

## License

Licensed under the [GPLv3](LICENSE) or later.
