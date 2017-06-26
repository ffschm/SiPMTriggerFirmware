# SiPM Trigger Controller&Counter
This firmware allows an arduino to be used as a controller of the digital potentiometer on the SiPMTrigger board
as well as a simple scaler.
Please note: The combination of slow control and data aquisition has several drawbacks but can be choosen if handled carefully.

# Installation

This sketch depends on the following libraries:
- [FreqCount](https://github.com/PaulStoffregen/FreqCount)
- [RTClib](http://www.tinkbox.ph/sites/tinkbox.ph/files/downloads/RTClib.zip)
- [LiquidCrystal](https://www.arduino.cc/en/Reference/LiquidCrystal) (included in the Arduino IDE)
- [OneWire](https://github.com/PaulStoffregen/OneWire)
- [DallasTemperature](https://github.com/milesburton/Arduino-Temperature-Control-Library)
- [AnotherSerialCommand](https://github.com/ffschm/Arduino-AnotherSerialCommand)
- [SPI](https://www.arduino.cc/en/Reference/SPI) (included in the Arduino IDE)

# Commands

| Comand | Description |
| ------ | ----------- |
| `SET THR $CH, $THR`| Set the threshold of signal channel `$CH` to `$THR`. |
| `SET WIDTH $CH, $VAL`| Set the voltage divider controlling the latch of signal channel `$CH` to `$VAL`. |
| `SCAN THR`| Scan the threshold of both signal channels simultaneously by increasing them one step at each tick. |
| `SET PE GAIN $CH, $VAL`| Set the gain of signal channel `$CH` to `$VAL`. |
| `SET PE THR $CH, $PETHR`| Set the threshold of signal channel `$CH` to `$PETHR * $GAIN`. |
| `SCAN THR $STEPWIDTH`| Scan the thresholds of both signal channels simultaneously by increasing them by `$STEPWIDTH` at each tick. |
| `SET TIME $INTTIME`| Set the integrating time interval to `$INTTIME` (in milliseconds). |
| `GET TEMP`| Measure the temperature (&humidity&pressure) and print it via the serial console. |

# Output
Example:
```
# SiPMTrigger v4 Control v0.5
# CH1(p_width) CH1(THR) CH2(THR) CH1(p_width) CH1(THR/pe) CH2(THR/pe) counts
128 128 128 128 3.20 3.20 0
128 128 128 128 3.20 3.20 0
128 128 128 128 3.20 3.20 0
128 128 128 128 3.20 3.20 0
128 128 128 128 3.20 3.20 0
128 128 128 128 3.20 3.20 0
```

The output contains the threshold settings for channel 1-4 of the digital potentiometer as well as the trigger rate with error of the signal on pin 2 of the arduino in the last integration time interval.
Channel 2 and 3 of the digital potentiometer correspond to the threshold setting of signal channel 1 and to 2 on the SiPMTrigger board.

| Potentiometer channel | Function |
| --------------------- | -------- |
| channel 0             | signal channel 1 pulse width |
| channel 1             | signal channel 2 threshold |
| channel 2             | signal channel 1 threshold |
| channel 3             | signal channel 2 pulse width |

## License

Licensed under the [GPLv3](LICENSE) or later.
