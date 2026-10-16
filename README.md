# Lan to Bluetooth WoL for the PS3
Receive WoL packets using the RP PICO 2040 (or newer) + w5500 Ethernet and send them as Bluetooth packets emulating a real DS3 controller
<br>
To use configure platformio.ino with the data obtained from your PS3 controller using SixaxisPairTool (for PS3 bt board) + any Bluetooth app that shows MAC addresses (for the controller's).
<br>
You might also want to configure a different MAC address for the Ethernet port in the platformio file if there is a collision!
<br>
<br>
This WoL requires an external device compared to the DEX settings. Use in case you don't have access to QA/DEX flags or don't like the fan always running.
