# SpinningCube


This is a redo of the original Spinning Cube code.  I have switched to a ESP8266 for wireless communication on board and have implemented the ESP_NOW Protocol for Controller to Cube communication.

Hardware Required:

Cube:
ESP 8266
APA102 Pixels
Logic Level Shifter
3.3v-5v charge controller
18650 Battery

Controller:
Low Speed, high torque Drone Motor
Drone 12A ESC
2 18650 Batteries
Rotary Encoder w Switch


notes on timer lib:
Download the latest ticker package as a zip file
Unzip the package from point 1
Made a back up of C:\Users\john\Documents\ArduinoData\packages\esp8266\hardware\esp8266\2.5.0-beta2\libraries\Ticker
Replaced the folder mentioned in point 3 with the Ticker folder in point 2.
Restarted my Arduino IDE
Compiled and ran the code
Opened up the Serial Monitor