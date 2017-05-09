# SiPM Trigger Controller&Counter
This firmware allows an arduino to be used as a controller of the digital potentiometer on the SiPMTrigger board
as well as a simple scaler.
Please note: The combination of slow control and data aquisition has several drawbacks but can be choosen if handled carefully.

# Commands

| Comand | Description |
| ------ | ----------- |
| `SET $CH, $THR`| Set the potentiometer value of channel `$CH` to threshold `$THR`. |
| `SCAN $CH`| Scan the potentiometer value of channel `$CH` by increasing one step at each tick. |

# Output
Example:
```
128,128,128,128,0
128,128,128,128,0
128,128,128,128,0
```

The output contains the threshold settings for channel 1-4 of the digital potentiometer as well as the trigger rate of the signal on pin 2 of the arduino in the last integration time interval.
Channel 2 and 3 of the digital potentiometer correspond to the threshold setting of signal channel 1 and to 2 on the SiPMTrigger board (to be confirmed!).