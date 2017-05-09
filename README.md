# SiPM Trigger Controller&Counter
This firmware allows an arduino to be used as a controller of the digital potentiometer on the SiPMTrigger board
as well as a simple scaler.
Please note: The combination of slow control and data aquisition has several drawbacks but can be choosen if handled carefully.

# Commands
SET $CH, $THR
Set the potentiometer value of channel $CH to threshold $THR.

SCAN $CH
Scan the potentiometer value of channel $CH by increasing one step at each tick.
