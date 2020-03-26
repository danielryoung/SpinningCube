/// LIBS
#include <ESP8266WiFi.h>
#include "Ticker.h"
extern "C"
{
#include <espnow.h>
#include "user_interface.h"
}

#include <FastLED.h>
#include <SPI.h>

//#include <stdint.h>
/// GLOBALS

//  We are using APA102 Strips, 4 strips of 6 on each side of the cube
// SPI Pins designated here for DATA_PIN and CLOCK_PIN
#define DATA_PIN D7
#define CLOCK_PIN D5

#define L_P_SIDE 12

#define MENU_ITEMS  5
// Menu Items needs to match the transmitted data size, which is also menu items in the controller code.

// going to define each menu number so it has a name
#define PATTERN       0
#define FRAMERATE     1
#define MOTORSPEED    2
#define FINEFRAMERATE 3
#define ONTIME        4

// How many leds in your strip?
#define NUM_LEDS    48

// Define the array of leds
CRGB leds[NUM_LEDS];
CRGB cube[NUM_LEDS];

// This the ESP NOW Data that will be transmitted.
// this is length of data sent in byte array txrxData.  this can be up to 100ish bytes


uint8_t txrxData[MENU_ITEMS];
bool frameRateChange = 1;

// initiate a tracker to only change frame rate when we get a new value
byte oldFrame = 0;

//void printMessage();

//Ticker timer1(printMessage, 5000,0, MICROS_MICROS);

//txrxData[0] = 1;
//txrxData[FRAMERATE] = 60;
//txrxData[MOTORSPEED] = 60;
void nextSide();
Ticker frameTimer(nextSide, 200,0, MICROS_MICROS);
Ticker onTimer(turnOff,200,0,MICROS_MICROS);
//Ticker 
int side = 0;

bool cubeIsOn = true;
//bool debugOn = true;
///SETUP

void setup()
{
  // DEBUG SETUP prints MAC Address from WIFI Stack.  Only necessary to know receiver mac.
  //Serial.begin(115200);
 // Serial.println("\r\nESP_Now MASTER CONTROLLER\r\n");
  //WiFi.mode(WIFI_STA);
  //WiFi.begin();
  //Serial.print("\r\n\r\nDevice MAC: ");
  //Serial.println(WiFi.macAddress());
  //  Comment out above dbug and serial for prod code.  slows down everything.

  // init values for each menu:
  txrxData[PATTERN] =       1;
  txrxData[FRAMERATE] =     200;
  txrxData[MOTORSPEED] =    60;
  txrxData[FINEFRAMERATE] = 10;
  txrxData[ONTIME] =        200; // this is a percent of the time the next side swithc is black for
  
  // this is a setup of our LED array for FASTLED lib
  LEDS.addLeds<APA102, DATA_PIN, CLOCK_PIN, RGB>(leds, NUM_LEDS);
  LEDS.setBrightness(5);
  // ideally we would do some power calc and set max current here  TODO

  // ESP NOW Setup
  esp_now_init();
  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
delay(2000);
 
 onTimer.interval( (2000 * .95) );
 frameTimer.interval( 2000 ); 
  frameTimer.start();
  onTimer.start();

  // this is a register callback function set up that copies data to txrxData from received data.
  //  TODO is to make this a explicit function instead of an anonymous one in the declaration.
  esp_now_register_recv_cb([](uint8_t *mac, uint8_t *data, uint8_t len) {
  
    memcpy(txrxData, data, len);
   changeFrameInterval ();

 // Serial.printf("Got hue menu =\t%i\n\r", txrxData[PATTERN]);
 // Serial.printf("Got frame menu =\t%i\n\r", txrxData[FRAMERATE]);
 // Serial.printf("Got motorspeed menu =\t%i\n\r", txrxData[MOTORSPEED]);
  //Serial.printf("Got fineframe menu =\t%i\n\r", txrxData[FINEFRAMERATE]);
  //Serial.printf("Got Ontime menu =\t%i\n\r", txrxData[ONTIME]);
     
   
     // need a map here
  });
}

void changeFrameInterval (){
  
  uint32_t onDurationMicros = ((txrxData[FRAMERATE] * 1000) + map(txrxData[FINEFRAMERATE],0,254,0,999));
  //(txrxData[FRAMERATE] * 100 );
  //((txrxData[FRAMERATE] * 1000) + map(txrxData[FINEFRAMERATE],0,254,0,999));
  //take our rough frame and change milli to micro times 1000
  //then take our fine frame rate and add a fractional millisecond up to 999
  //Serial.print("onfor: ");
  //Serial.println(onDurationMicros);
 
  uint32_t turnOffAfter = map(txrxData[ONTIME],0,254,0,onDurationMicros );
 // Serial.print("offafter: ");
 // Serial.println(turnOffAfter);
  //(onDurationMicros - txrxData[ONTIME]) ;
  //map(txrxData[ONTIME],0,255,onDurationMicros,1);

      // this sets the time the frame will be on. 
      //for now a menu choice, but should be done with math
  onTimer.interval( turnOffAfter );
       //Serial.printf("Change Frame Interval");  
   frameTimer.interval(onDurationMicros);
}

//  esp now will receive bytes without doing anything in loop.
void loop()
{
  frameTimer.update();
  onTimer.update();

  // updates timer
  //  timer1.update();
  //changeFrameInterval();
  
  //ledPattern();
  // we are always mapping cube to leds
  // but we periodically call next side at the framerate
  if (cubeIsOn){
  ledPattern();
 
 } else {
  turnOff();
 }

 //if (debugIsOn){
  
 //cube[1] = CHSV(map(txrx, 255, 255);
 //}
 FastLED.show(); 
 stripToCubeMap();
 //change pattern every ten sec.  for now pattern is just HUE
 //EVERY_N_SECONDS(10){txrxData[PATTERN]++;}
 //setSide();
 //FastLED.show();


 // this should change frameduration with the incoming framerate from controller
// EVERY_N_MILLISECONDS_I(thisTimer,100){
 // uint8_t timeval = txrxData[FRAMERATE];
  //thisTimer.setPeriod(timeval);
  //nextSide();
  //}
  //Serial.print ("speed should be ");
 // Serial.println(txrxData[1]);
}

void ledPattern()
{
  static uint8_t globalHue = txrxData[PATTERN];
  // static means the value will be maintained outside the function!
  
//    for (int i = (NUM_LEDS)-1; i >= 0; i--)
 // {
    // Set the i'th led to red
   // leds[i] = CHSV(100, 255, 255);
   // cube[6] = CHSV(hue++,255,255);
   // fill_solid(cube,6,CHSV(txrxData[PATTERN],255,255));
   // this just fills our cube with four increasing colors
   // in theory this will shift around with the frame rate.
   setSide(0,txrxData[PATTERN]);
   setSide(1,txrxData[PATTERN] + 50);
   setSide(2,txrxData[PATTERN] + 100);
   setSide(3,txrxData[PATTERN] + 150);
   // leds[i] = cube[1];
    // Show the leds
    //FastLED.show();

    // now that we've shown the leds, reset the i'th led to black
    // leds[i] = CRGB::Black;
    //fadeall();
    // Wait a little bit before we loop around and do it again
    //delay(10);
 // }
}

void setSide(int side, int hue){

  side = side % 4;
  //fadeall();
  for (int i = 0 + (side * L_P_SIDE); i < (L_P_SIDE + (side * L_P_SIDE)); i++)
  {
    cube[i] = CHSV(hue,255,255);
  }
//Serial.printf("SettheSide");
}

void fadeall()
{
  for (int i = 0; i < NUM_LEDS; i++)
  {
    cube[i].nscale8(250);
  }
}

void stripToCubeMap(){
  for (int i = 0; i < NUM_LEDS; i++)
  {
     //remap each led from cube position to correct strip position
     
    int n = (i + L_P_SIDE * side)%NUM_LEDS;
    leds[n] = cube[i];
   // Serial.print("N: ");
   //Serial.println(n);
    //Serial.print("I: ");
    //Serial.println(i);
    //Serial.print("Side: ");
    //Serial.println(side);
  }
  
}


void nextSide(){
  side++;
  if (side == 4) {side = 0;}
  //Serial.printf("next Side");
  //if(txrxData[FRAMERATE] <= 220){fill_solid( leds, NUM_LEDS, CRGB(50,0,200));}
  onTimer.start(); 
  cubeIsOn = true;
}


void turnOff(){
  fill_solid( leds, NUM_LEDS, CRGB(0,0,0));
  cubeIsOn = false;
 // FastLED.show();
  //Serial.printf("TurnOff Fired =\t%i\n\r", txrxData[ONTIME]);
}

/**
void cylon()
{
  static uint8_t hue = 0;
  //Serial.println(txrxData[0]);
  // First slide the led in one direction
  for (int i = 0; i < NUM_LEDS; i++)
  {
    // Set the i'th led to red
    cube[i] = CHSV(hue++, 255, 255);
    // Show the leds
    FastLED.show();
    // now that we've shown the leds, reset the i'th led to black
    // leds[i] = CRGB::Black;
    fadeall();
    // Wait a little bit before we loop around and do it again
    delay(txrxData[0]);
  }
  //Serial.print("x");

  // Now go in the other direction.
  for (int i = (NUM_LEDS)-1; i >= 0; i--)
  {
    // Set the i'th led to red
    cube[i] = CHSV(hue++, 255, 255);
    // Show the leds
    FastLED.show();

    // now that we've shown the leds, reset the i'th led to black
    // leds[i] = CRGB::Black;
    fadeall();
    // Wait a little bit before we loop around and do it again
    delay(10);
  }
}
**/
