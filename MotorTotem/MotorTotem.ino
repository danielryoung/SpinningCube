#include <FastLED.h>
#include <Servo.h>

#define HWSERIAL Serial1
 
Servo esc;
//SoftwareSerial BTSerial(5,6);

#define DATA_PIN     11
#define CLOCK_PIN     14
#define COLOR_ORDER BGR
#define CHIPSET     APA102
#define NUM_LEDS    24
#define LED_GROUP 6
#define ESC_PIN 20
#define RX_PIN 0
#define TX_PIN 1


#define BRIGHTNESS  100
#define FRAMES_PER_SECOND 10


//this is the pot value diff that needs to exist before we make change to speed.
#define SPEEDDELTA 10
#define MTRSPDINTERVAL 50


// Fire2012 by Mark Kriegsman, July 2012
// as part of "Five Elements" shown here: http://youtu.be/knWiGsmgycY
//// 
// This basic one-dimensional 'fire' simulation works roughly as follows:
// There's a underlying array of 'heat' cells, that model the temperature
// at each point along the line.  Every cycle through the simulation, 
// four steps are performed:
//  1) All cells cool down a little bit, losing heat to the air
//  2) The heat from each cell drifts 'up' and diffuses a little
//  3) Sometimes randomly new 'sparks' of heat are added at the bottom
//  4) The heat from each cell is rendered as a color into the leds array
//     The heat-to-color mapping uses a black-body radiation approximation.
//
// Temperature is in arbitrary units from 0 (cold black) to 255 (white hot).
//
// This simulation scales it self a bit depending on NUM_LEDS; it should look
// "OK" on anywhere from 20 to 100 LEDs without too much tweaking. 
//
// I recommend running this simulation at anywhere from 30-100 frames per second,
// meaning an interframe delay of about 10-35 milliseconds.
//
// Looks best on a high-density LED setup (60+ pixels/meter).
//
//
// There are two main parameters you can play with to control the look and
// feel of your fire: COOLING (used in step 1 above), and SPARKING (used
// in step 3 above).
//
// COOLING: How much does the air cool as it rises?
// Less cooling = taller flames.  More cooling = shorter flames.
// Default 50, suggested range 20-100 
#define COOLING  55

// SPARKING: What chance (out of 255) is there that a new spark will be lit?
// Higher chance = more roaring fire.  Lower chance = more flickery fire.
// Default 120, suggested range 50-200.
#define SPARKING 80
bool gReverseDirection = false;



// attach stuff
elapsedMillis sinceMotorAdj;
elapsedMillis sinceFlash;
elapsedMicros offFlash;

CRGB leds[NUM_LEDS];
//globals
unsigned int flashTime = 10000;

//this is the Motor speed that motor is moving at, will change
int currentMotorSpeed = 40;

//apply some smoothing by taking what we want speed to be and moving to it.
int desiredMotorSpeed = 0; // pot1 value

unsigned int flashRate = 200; //pot2 value

byte reading = 0;

void setup() {
  delay(3000); // sanity delay
  FastLED.addLeds<CHIPSET,DATA_PIN, CLOCK_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness( BRIGHTNESS );
  //add something to black out leds here
  esc.attach(ESC_PIN);
  HWSERIAL.setRX(RX_PIN);
  HWSERIAL.setTX(TX_PIN);
  HWSERIAL.begin(38400);
 // BTSerial.begin(38400);
}
void Fire2012()
{
// Array of temperature readings at each simulation cell
  static byte heat[NUM_LEDS];

  // Step 1.  Cool down every cell a little
    for( int i = 0; i < NUM_LEDS; i++) {
      heat[i] = qsub8( heat[i],  random8(0, ((COOLING * 10) / NUM_LEDS) + 2));
    }
  
    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for( int k= NUM_LEDS - 1; k >= 2; k--) {
      heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
    }
    
    // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
    if( random8() < SPARKING ) {
      int y = random8(7);
      heat[y] = qadd8( heat[y], random8(160,255) );
    }

    // Step 4.  Map from heat cells to LED colors
    for( int j = 0; j < NUM_LEDS; j++) {
      CRGB color = HeatColor( heat[j]);
      int pixelnumber;
      if( gReverseDirection ) {
        pixelnumber = (NUM_LEDS-1) - j;
      } else {
        pixelnumber = j;
      }
      leds[pixelnumber] = color;
    }
}

void parseReading(){
       
   if(HWSERIAL.available()>0) {
    reading = HWSERIAL.read();
 
    desiredMotorSpeed = reading; //pot1 first val comma sep
    flashRate = map(reading,0,180,10,60);
    flashTime = map(reading,0,180,8000,20000);
   // flashTime = 12000;
    //TODO Calculate flash time from flashrate in microseconds
    
   }
  esc.write(reading);
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

  
void loop() {
  // put your main code here, to run repeatedly:

 //maybe timer here also. get readings and parse only occasionally
  parseReading();

  if (sinceMotorAdj >= MTRSPDINTERVAL ) {
  adjustMotorSpeed();
  }

//see if its time to turn on then wait to turn off again

if (reading > 0) {
  
  if(sinceFlash > flashRate){
    CRGB color1 = CRGB::Red;
    CRGB color2 = CRGB::Green;
    for( int i = 0; i < NUM_LEDS; i++) {
       int g = ((LED_GROUP + i) - (i%LED_GROUP))/ LED_GROUP;

       switch (g) {
        case 1: leds[i] = color1;
        break;
        case 2: leds[i] = color2;
        break;
        case 3: leds[i] = color1;
        break;
        case 4: leds[i] = color2;
        break;
        }
 
    }
    sinceFlash = 0;
    offFlash = 0;
  }
  if (offFlash > flashTime){
    CRGB color = CRGB::Blue;
   for( int i = 0; i < NUM_LEDS; i++) {
   
    leds[i] = color;
      }
    offFlash = 0;
  } 
}else {
  Fire2012();
  FastLED.delay(1000 / FRAMES_PER_SECOND);
}
  FastLED.show(); // display this frame
  //FastLED.delay(1000 / FRAMES_PER_SECOND);
}



//void startupMotor(){}

//void shutdownMotor(){}

//void setMotorSpeed(spd){


  // if pot is different by SPEEDDELTA then transition to new speed
 //return yes to move, return no to not chagne
 
 // if (compareReading(spd)) {
 //transitionSpeed(spd);





