#include <Servo.h>
 
Servo esc;
int throttlePin = 0;
 
void setup()
{
  delay(10);
esc.attach(9);
Serial.begin(9600);
  Serial.println("PowerOn");
}
 
void loop()
{
int throttle = analogRead(throttlePin);
throttle = map(throttle, 0, 1023, 0, 179);
Serial.println(throttle);
esc.write(throttle);
}
