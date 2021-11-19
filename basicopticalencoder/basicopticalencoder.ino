/*
-Arduino Position Encoder
-Using a generic photo-interrupter
-Basic Test Sketch 1 / June 2014
-Tested at TechNode Protolabz
-www.electroschematics.com/
*/
const int encoderIn = 8; // input pin for the interrupter 
const int statusLED = 13; // Output pin for Status indicator
const int pulseOutput = 12; // Pulse output pin for external interfacing
int detectState=0; // Variable for reading the encoder status
void setup()
{
   pinMode(encoderIn, INPUT); //Set pin 8 as input
   pinMode(statusLED, OUTPUT); //Set pin 13 as output
   pinMode(pulseOutput, OUTPUT); // Set Pin 12 as output
}
void loop() {
   detectState=digitalRead(encoderIn);
   if (detectState == HIGH) { //If encoder output is high
      digitalWrite(statusLED, HIGH); //Turn on the status LED
      digitalWrite(pulseOutput,HIGH); // Give a logic-High level output
   }
   else {
      digitalWrite(statusLED, LOW); //Turn off the status LED
      digitalWrite(pulseOutput,LOW); // Give a logic-Low level output
   }
}
