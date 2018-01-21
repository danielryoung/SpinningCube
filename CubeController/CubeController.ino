#include <Servo.h>
 
//Servo esc;
int throttlePin = 0;
int flashPin = 1;
 
void setup()
{
  delay(10);
esc.attach(9);
Serial.begin(38400);
  Serial.println("PowerOn");
}

//2000 rpm = 1 rotaion per 30 ms per flash 
//100 rpm = 600 ms per flash
// flash for 3 degrees, so (360/rotationtime)= 
//  timeperdegree * 3 = time for threedegrees microsecond 
void loop()
{
int throttle = analogRead(throttlePin);
int flash = analogRead(flashPin);

throttle = map(throttle, 0, 1023, 0, 179);
flash = map(flash,0,1023, 650, 20);

Serial.println(throttle ',' flash);
//esc.write(throttle);
}
