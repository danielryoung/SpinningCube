/// LIBS
#include <ESP8266WiFi.h>
#include <TickTwo.h>
#include "Arduino.h"
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
void turnOff();
TickTwo frameTimer(nextSide, 200,0, MICROS_MICROS);
TickTwo onTimer(turnOff,200,0, MICROS_MICROS);
//Ticker 
int side = 0;

bool cubeIsOn = true;
//bool debugOn = true;
///SETUP

void setup()
{
  // DEBUG SETUP prints MAC Address from WIFI Stack.  Only necessary to know receiver mac.
  //Serial.begin(115200);
 //Serial.println("\r\nESP_Now MASTER CONTROLLER\r\n");
  WiFi.mode(WIFI_STA);
  WiFi.begin();
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
  //LEDS.setBrightness(100);
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

  //Serial.printf("Got hue menu =\t%i\n\r", txrxData[PATTERN]);
  //Serial.printf("Got frame menu =\t%i\n\r", txrxData[FRAMERATE]);
  //Serial.printf("Got motorspeed menu =\t%i\n\r", txrxData[MOTORSPEED]);
  //Serial.printf("Got fineframe menu =\t%i\n\r", txrxData[FINEFRAMERATE]);
  //Serial.printf("Got Ontime menu =\t%i\n\r", txrxData[ONTIME]);

  });
}

void changeFrameInterval () {
  
  uint32_t onDurationMicros = ((txrxData[FRAMERATE] * 1000) + map(txrxData[FINEFRAMERATE],0,254,0,9999));
  //take our rough frame and change milli to micro times 1000
  //then take our fine frame rate and add a fractional millisecond up to 999
  
  uint32_t turnOffAfter = map(txrxData[ONTIME],0,254,0,(onDurationMicros +1 /20) );

  // this sets the time the frame will be on. 
  //for now a menu choice, but should be done with math
  onTimer.interval( turnOffAfter );
  frameTimer.interval(onDurationMicros);
}

//  esp now will receive bytes without doing anything in loop.
void loop() {
  frameTimer.update();
  onTimer.update();
  
  // we are always mapping cube to leds
  // but we periodically call next side at the framerate
  if (cubeIsOn) {

        switch (txrxData[MOTORSPEED]){
        
          case 1 ... 40:
            LEDS.setBrightness(0);
            break;
          case 41 ...50:
            LEDS.setBrightness(50);
            pacifica_loop();
            break;
          case 51 ... 60:
            frameTimer.interval(15000);
            onTimer.interval(14999);
            LEDS.setBrightness(80);
            pacifica_loop();
            break;
            default:
            ledPattern();
        }
 } else {

 }

 FastLED.show(); 
 stripToCubeMap();
}

void ledPattern() {
  static uint8_t globalHue = txrxData[PATTERN];
  // static means the value will be maintained outside the function!
  
   // this just fills our cube with four increasing colors
   // in theory this will shift around with the frame rate.
   setSide(0,txrxData[PATTERN]);
   setSide(1,txrxData[PATTERN] + 50);
   setSide(2,txrxData[PATTERN] + 100);
   setSide(3,txrxData[PATTERN] + 150);
}

void setSide(int side, int hue) {

  side = side % 4;
  for (int i = 0 + (side * L_P_SIDE); i < (L_P_SIDE + (side * L_P_SIDE)); i++) {
    cube[i] = CHSV(hue,255,255);
  }
}

void fadeall() {
  for (int i = 0; i < NUM_LEDS; i++) {
    cube[i].nscale8(250);
  }
}

void stripToCubeMap() {
  for (int i = 0; i < NUM_LEDS; i++) {
     //remap each led from cube position to correct strip position
    int n = (i + L_P_SIDE * side)%NUM_LEDS;
    leds[n] = cube[i];
  }
  
}

void nextSide() {
  side++;
  if (side == 4) {
    side = 0;
  }
  onTimer.start(); 
  cubeIsOn = true;
}


void turnOff() {
  fill_solid( leds, NUM_LEDS, CRGB(0,0,0));
  cubeIsOn = false;
  FastLED.show();
}


// pacifica pattern stuff. do this different.

CRGBPalette16 pacifica_palette_1 = 
    { 0x000507, 0x000409, 0x00030B, 0x00030D, 0x000210, 0x000212, 0x000114, 0x000117, 
      0x000019, 0x00001C, 0x000026, 0x000031, 0x00003B, 0x000046, 0x14554B, 0x28AA50 };
CRGBPalette16 pacifica_palette_2 = 
    { 0x000507, 0x000409, 0x00030B, 0x00030D, 0x000210, 0x000212, 0x000114, 0x000117, 
      0x000019, 0x00001C, 0x000026, 0x000031, 0x00003B, 0x000046, 0x0C5F52, 0x19BE5F };
CRGBPalette16 pacifica_palette_3 = 
    { 0x000208, 0x00030E, 0x000514, 0x00061A, 0x000820, 0x000927, 0x000B2D, 0x000C33, 
      0x000E39, 0x001040, 0x001450, 0x001860, 0x001C70, 0x002080, 0x1040BF, 0x2060FF };
// Gradient palette "purplefly_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/rc/tn/purplefly.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 16 bytes of program space.

CRGBPalette16 purplefly_palette ={
    0,   0,  0,  0,
   63, 239,  0,122,
  191, 252,255, 78,
  255,   0,  0,  0
  };


void pacifica_loop()
{
  // Increment the four "color index start" counters, one for each wave layer.
  // Each is incremented at a different speed, and the speeds vary over time.
  static uint16_t sCIStart1, sCIStart2, sCIStart3, sCIStart4;
  static uint32_t sLastms = 0;
  uint32_t ms = GET_MILLIS();
  uint32_t deltams = ms - sLastms;
  sLastms = ms;
  uint16_t speedfactor1 = beatsin16(3, 179, 269);
  uint16_t speedfactor2 = beatsin16(4, 179, 269);
  uint32_t deltams1 = (deltams * speedfactor1) / 256;
  uint32_t deltams2 = (deltams * speedfactor2) / 256;
  uint32_t deltams21 = (deltams1 + deltams2) / 2;
  sCIStart1 += (deltams1 * beatsin88(1011,10,13));
  sCIStart2 -= (deltams21 * beatsin88(777,8,11));
  sCIStart3 -= (deltams1 * beatsin88(501,5,7));
  sCIStart4 -= (deltams2 * beatsin88(257,4,6));

  // Clear out the LED array to a dim background blue-green
  fill_solid( leds, NUM_LEDS, CRGB( 2, 6, 10));

  // Render each of four layers, with different scales and speeds, that vary over time
  pacifica_one_layer( purplefly_palette, sCIStart1, beatsin16( 3, 11 * 256, 14 * 256), beatsin8( 10, 70, 130), 0-beat16( 301) );
  pacifica_one_layer( purplefly_palette, sCIStart2, beatsin16( 4,  6 * 256,  9 * 256), beatsin8( 17, 40,  80), beat16( 401) );
  pacifica_one_layer( pacifica_palette_3, sCIStart3, 6 * 256, beatsin8( 9, 10,38), 0-beat16(503));
  pacifica_one_layer( purplefly_palette, sCIStart4, 5 * 256, beatsin8( 8, 10,28), beat16(601));

  // Add brighter 'whitecaps' where the waves lines up more
  pacifica_add_whitecaps();

  // Deepen the blues and greens a bit
  pacifica_deepen_colors();
}

// Add one layer of waves into the led array
void pacifica_one_layer( CRGBPalette16& p, uint16_t cistart, uint16_t wavescale, uint8_t bri, uint16_t ioff)
{
  uint16_t ci = cistart;
  uint16_t waveangle = ioff;
  uint16_t wavescale_half = (wavescale / 2) + 20;
  for( uint16_t i = 0; i < NUM_LEDS; i++) {
    waveangle += 250;
    uint16_t s16 = sin16( waveangle ) + 32768;
    uint16_t cs = scale16( s16 , wavescale_half ) + wavescale_half;
    ci += cs;
    uint16_t sindex16 = sin16( ci) + 32768;
    uint8_t sindex8 = scale16( sindex16, 240);
    CRGB c = ColorFromPalette( p, sindex8, bri, LINEARBLEND);
    leds[i] += c;
  }
}

// Add extra 'white' to areas where the four layers of light have lined up brightly
void pacifica_add_whitecaps()
{
  uint8_t basethreshold = beatsin8( 9, 55, 65);
  uint8_t wave = beat8( 7 );
  
  for( uint16_t i = 0; i < NUM_LEDS; i++) {
    uint8_t threshold = scale8( sin8( wave), 20) + basethreshold;
    wave += 7;
    uint8_t l = leds[i].getAverageLight();
    if( l > threshold) {
      uint8_t overage = l - threshold;
      uint8_t overage2 = qadd8( overage, overage);
      leds[i] += CRGB( overage, overage2, qadd8( overage2, overage2));
    }
  }
}

// Deepen the blues and greens
void pacifica_deepen_colors()
{
  for( uint16_t i = 0; i < NUM_LEDS; i++) {
    leds[i].blue = scale8( leds[i].blue,  145); 
    leds[i].green= scale8( leds[i].green, 200); 
    leds[i] |= CRGB( 2, 5, 7);
  }
}
