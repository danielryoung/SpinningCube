#include <Servo.h>
#include <FastLED.h>


#define DATAPIN MOSI //D7
#define CLOCKPIN SCK //D5
#define ESCPIN D1
// BTSerial on RT TX

//FASTLEDSETUP
#define NUM_LEDS 24
#define CHIPSET APA102
#define COLOR_ORDER BGR
#define BRIGHTNESS 100


//this is the pot value diff that needs to exist before we make change to speed.
#define SPEEDDELTA 5
#define MTRSPDINTERVAL 50

// attach stuff
Servo esc;
elapsedMillis sinceMotorAdj;
elapsedMicros sinceFlash;

//globals


//this is the Motor speed that motor is moving at, will change
int currentMotorSpeed = 0;

//apply some smoothing by taking what we want speed to be and moving to it.
int desiredMotorSpeed = 0; // pot1 value

int flashRate = 0; //pot2 value

String reading;

void setup(){
  delay(10);
  esc.attach(ESCPIN);
  Serial.begin(38400);

  delay(3000);
 FastLED.addLeds<CHIPSET, DATAPIN, CLOCKPIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
 FastLED.setBrightness(BRIGHTNESS);
 
 //Metro motorTransition = Metro(MTRSPDINTERVAL);
 
}

void loop() {
  // put your main code here, to run repeatedly:

 //maybe timer here also. get readings and parse only occasionally
  parseReading();

  if (sinceMotorAdjust >= MTRSPDINTERVAL ) {
  adjustMotorSpeed();
  }

//see if its time to turn on then wait to turn off again
  if(sinceFlash > flashRate){
    leds[2] = CRGB:Green;
    sinceFlash = 0;
    offFlash = 0;
    };
  if (offFlash > flashTime){
    leds[2] = CRGB;Red;
    sinceFlash = 0;
  } 

}


//void startupMotor(){}

//void shutdownMotor(){}

//void setMotorSpeed(spd){


  // if pot is different by SPEEDDELTA then transition to new speed
 //return yes to move, return no to not chagne
 
 // if (compareReading(spd)) {
 //transitionSpeed(spd);
};

 
  
void parseReading(){

   if(Serial.available() > 0) {
    reading = Serial.read();
 
    desiredMotorSpeed = strtok(reading, ','); //pot1 first val comma sep
    flashRate = strtok();
    flashTime = 3000;
    //TODO Calculate flash time from flashrate in microseconds
    
   }
}

void adjustMotorSpeed() {
//this gets called by timer callback
  if(desiredMotorSpeed > currentMotorSpeed){
      currentMotorSpeed++;
  }
    else if(desiredMotorSpeed < currentMotorSpeed){
      currentMotorSpeed--;
    }
    else{}
} 
  


