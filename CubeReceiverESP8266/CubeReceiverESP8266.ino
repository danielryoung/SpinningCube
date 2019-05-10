/// LIBS
#include <ESP8266WiFi.h>
extern "C"
{
#include <espnow.h>
#include "user_interface.h"
}

#include <FastLED.h>
#include <SPI.h>

/// GLOBALS

//  We are using APA102 Strips, 4 strips of 6 on each side of the cube
// SPI Pins designated here for DATA_PIN and CLOCK_PIN
#define DATA_PIN D7
#define CLOCK_PIN D5
#define MENU_ITEMS 3
// Menu Items needs to match the transmitted data size, which is also menu items in the controller code.
// How many leds in your strip?
#define NUM_LEDS 24

// Define the array of leds
CRGB leds[NUM_LEDS];
CRGB cube[NUM_LEDS];

// This the ESP NOW Data that will be transmitted.
// this is length of data sent in byte array txrxData.  this can be up to 100ish bytes

byte txrxData[MENU_ITEMS];

int side = 0;
///SETUP

void setup()
{
  // DEBUG SETUP prints MAC Address from WIFI Stack.  Only necessary to know receiver mac.
  Serial.begin(115200);
  Serial.println("\r\nESP_Now MASTER CONTROLLER\r\n");
  WiFi.mode(WIFI_STA);
  WiFi.begin();
  Serial.print("\r\n\r\nDevice MAC: ");
  Serial.println(WiFi.macAddress());
  //  Comment out above dbug and serial for prod code.  slows down everything.

  // this is a setup of our LED array for FASTLED lib
  LEDS.addLeds<APA102, DATA_PIN, CLOCK_PIN, RGB>(leds, NUM_LEDS);
  LEDS.setBrightness(84);
  // ideally we would do some power calc and set max current here  TODO

  // ESP NOW Setup
  esp_now_init();
  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);

  // this is a register callback function set up that copies data to txrxData from received data.
  //  TODO is to make this a explicit function instead of an anonymous one in the declaration.
  esp_now_register_recv_cb([](uint8_t *mac, uint8_t *data, uint8_t len) {
    //Serial.printf("Got Something length =\t%i\n\r", len);
    memcpy(txrxData, data, len);
  });
}

//  esp now will receive bytes without doing anything in loop.
void loop()
{
 // ledTest();
 stripToCubeMap();
 EVERY_N_SECONDS(2){ledTest();}
 //setSide();
 FastLED.show();
  EVERY_N_SECONDS(3){nextSide();}

}

void ledTest()
{
  static uint8_t hue = 0;
  // static means the value will be maintained outside the function!
  
    for (int i = (NUM_LEDS)-1; i >= 0; i--)
  {
    // Set the i'th led to red
   // leds[i] = CHSV(100, 255, 255);
   // cube[6] = CHSV(hue++,255,255);
    fill_solid(cube,6,CHSV(hue++,255,255));
   // leds[i] = cube[1];
    // Show the leds
    //FastLED.show();

    // now that we've shown the leds, reset the i'th led to black
    // leds[i] = CRGB::Black;
    //fadeall();
    // Wait a little bit before we loop around and do it again
    //delay(10);
  }
};

void setSide(){
  
  //fadeall();
  for (int i = 0; i < 6; i++)
  {
    cube[i] = CHSV(150,255,255);
  }
  

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
     
    int n = (i + 6 * side)%24;
    leds[n] = cube[i];
    Serial.print("N: ");
    Serial.println(n);
    Serial.print("I: ");
    Serial.println(i);
    Serial.print("Side: ");
    Serial.println(side);
  }
  
}

void nextSide(){
  side++;
  if (side == 4) {side = 0;}
   
}

void cylon()
{
  static uint8_t hue = 0;
  Serial.println(txrxData[0]);
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
  Serial.print("x");

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
