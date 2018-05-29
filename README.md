# SiPM Trigger Controller&Counter
This firmware allows an arduino to be used as a controller of the digital potentiometer on the SiPMTrigger board
as well as a simple scaler.
Please note: The combination of slow control and data aquisition by the same hardware has several drawbacks, but can be choosen if handled carefully.

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

The SiPMTrigger Controller can be controlled interactively via a serial terminal (e.g. minicom/Cutecom).
The required serial settings are:

Baud rate | 9600
Data bits | 8
Parity | None
Stop bits | 1
Handshake | None

The following table lists all available commands. For command execution send a newline character.

| Comand | Description |
| ------ | ----------- |
| `SET THR $CH, $THR`| Set the threshold of signal channel `$CH` to `$THR`. |
| `SET OFFSET $CH, $VAL`| Set the offset of signal channel `$CH` to `$VAL`. |
| `SET GAIN $CH, $VAL`| Set the gain of signal channel `$CH` to `$VAL`. |
| `SET PE THR $CH, $PETHR`| Set the threshold of signal channel `$CH` to `$PETHR * $GAIN`. |
| `SCAN THR $CH`| Scan the threshold of signal channel `CH` by increasing the threshold by 1LSB per step. |
| `SCAN PE THR`| Scan the thresholds of both signal channels simultaneously from 0p.e. up to the highest possible treshold by increasing them by 0.1p.e. at each tick. |
| `SET TIME $INTTIME`| Set the integrating time interval to `$INTTIME` (in milliseconds). |
| `GET TEMP`| Measure the temperature, humidity and pressure and print the results on the serial console. |

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

The output contains the threshold settings of the discriminator for both signal channels, as well as the trigger rate with error of the signal on the frequency input pin in the last integration time interval.

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

## Scripting

The SiPMTrigger Controller is controlled with the python scripts provided in the folter `contrib`.
To be able to use these scripts, a capacitor has to be added between the `RESET` and `GND` pin of the Arduino
to prevent a reset of the microcontroller after each command execution. The dependencies of the scripts
can be installed with
```
  sudo pip install -r contrib/requirements.txt
```

The following commands are available:
```
$ ./contrib/scan_thr.py --help
usage: scan_thr.py [-h] [--port PORT] [--channel CHANNEL]

Run a treshold scan on the discriminator of the SiPM Trigger Controller.

optional arguments:
  -h, --help         show this help message and exit
  --port PORT
  --channel CHANNEL

$ ./contrib/scan_pe_thr.py --help
usage: scan_pe_thr.py [-h] [--port PORT] [--gain1 GAIN1] [--offset1 OFFSET1]
                      [--gain2 GAIN2] [--offset2 OFFSET2]

Run a treshold scan on the discriminator of the SiPM Trigger Controller.

optional arguments:
  -h, --help         show this help message and exit
  --port PORT
  --gain1 GAIN1
  --offset1 OFFSET1
  --gain2 GAIN2
  --offset2 OFFSET2

$ ./contrib/get_calibration.py --help
usage: get_calibration.py [-h] [-c CHANNEL] thr_1pe thr_2pe

Calculate offset and gain from the first two photopeaks.

positional arguments:
  thr_1pe
  thr_2pe

optional arguments:
  -h, --help            show this help message and exit
  -c CHANNEL, --channel CHANNEL

```

### Usage: Measuring the gain & dark count rate of both channels

1. Connect the trigger output of channel 1 with the frequency counter input.
2. Start a treshold scan of channel 1 and save the result in a file `ch1_dark.dat`:

   ```
   $ ./contrib/scan_thr.py --port /dev/ttyACM0 --channel 1 > ch1_dark.dat
   ```
   NOTE: The arduino may be available on a different port on your system, adjust accordingly.
3. Optional: Abort the threshold scan with Ctrl-C as soon as you think you've measured at least
   three steps in the rate displayed (corresponding to a threshold of more than three photoelectrons)
4. Plot the data, e.g. with the provided [gnuplot-Script](contrib/thr_scan_example.plot).
   - Copy the script to your path:
     ```
     $ cp ./contrib/thr_scan_example.plot ./thr_scan_ch1.plot
     ```
   - Open it with a text editor and adjust it to plot your data. Replace `example_data/ch1.dat`
     with the name of your datafile and remove the trailing backslash and all following lines,
     unless you want to plot multiple channels:
     ```
     plot "example_data/ch1.dat" w errorlines lt 1 pt 5, \
     "example_data/ch2.dat" w errorlines lt 1 pt 4, \
     ```

   - Execute it in the the terminal.
     ```
     $ ./thr_scan_ch1.plot
     ```
   The resulting eps-file is now available as specified in the gnuplot-script.

There are example plots for the [single channel threshold scans](contrib/thr_scan_example.eps) and for the [coincidence spectrum](contrib/pe_thr_scan_example.eps) available.

## License

Licensed under the [GPLv3](LICENSE) or later.
