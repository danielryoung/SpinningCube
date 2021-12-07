/*
 * A demo that combines the techniques of ClickVersusDoubleClickUsingReleased
 * and ClickVersusDoubleClickUsingSuppression by using both event types to
 * trigger a "Clicked". I think this is the best of both worlds. If someone does
 * a simple Press/Release, the Release gets triggered. If someone does a quick
 * click, then a Click gets triggers (after a delay to wait for the potential
 * DoubleClick).
 *
 * See Also:
 *    examples/ClickVersusDoubleClickUsingReleased/
 *    examples/ClickVersusDoubleClickUsingSuppression/
 */
// Transmitter and Servo Stuff
#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <Ticker.h>
#include <Servo.h>

// i2c oled display setup
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

//Lib Object decs
Adafruit_SSD1306 display = Adafruit_SSD1306(128, 32, &Wire);

Servo esc;

//PIN DESIGNATIONS:

// The pin number attached to the button.
const int BUTTON_PIN = 36;
// DEBUG
int LED_PIN = LED_BUILTIN;
#define IR_PIN 13

//https://lastminuteengineers.com/handling-esp32-gpio-interrupts-tutorial/
#define ENCODER_PIN_1 39
#define ENCODER_PIN_2 32

// TODO this should probably be analog?
#define SERVO_PIN 32

// other const
#define WIFI_CHANNEL 1

//MAC ADDRESS OF THE DEVICE YOU ARE SENDING TO
//byte remoteMac[] = {0x3C, 0x71, 0xBF, 0x29, 0x52, 0x3C};
byte remoteMac[] = {0x2C, 0x3A, 0xE8, 0x4E, 0x3C, 0xAE};


#define MENU_ITEMS 5
// we want to have a menu for each value we are sending

#define PATTERN         0
#define FRAMERATE       1
#define MOTORSPEED      2
#define FINEFRAMERATE   3
#define ONTIME          4


byte cnt=0;
uint8_t transmitData[MENU_ITEMS];
bool transmitNeededFlag = true;
byte result;

bool motorRunning = false;

#define STARTUP_SPEED 110 
#define SLOTS_IN_DISK   4
#define SLOTS_PER_SIDE  1

// End Transmitter and Servo

// BUTTON MENU STUFF
#include <AceButton.h>
using namespace ace_button;
//IR Interupter STUFF

//unsigned long lastflash;
uint16_t SlotsCounted;
// this is the callback that gets called
// on interrupt at ir sensor for SlotsCounted
// its placed in ram for faster exec. 
//TODO Place other encoder interrupt there
void ICACHE_RAM_ATTR sens() {
  SlotsCounted++;
}

//HOW MANY MICROS WE SEE BEFORE COUNTING SLOTS
#define MICROS_PER_SAMPLE 1000

//this will be set to the number of microseconds per tick or slot on the wheel
uint16_t microsPerTick = 1; // gets set by timer cb function to micros elapsed between slots in disk
uint16_t microsPerSide = 1; // gets set by timer cb function to micros for side to rotate one place forward
uint16_t microsPerRevolution = 1; // gets set ... micros for one full revolution of all slots  - might be weird since it will be between 4 and 5 count for a revolution?

uint16_t millisPerSide = 1;

//END IR INTERUPTER


// ENCODER STUFF

#include <Encoder.h>

// Change these two numbers to the pins connected to your encoder.
//   Best Performance: both pins have interrupt capability
//   Good Performance: only the first pin has interrupt capability
//   Low Performance:  neither pin has interrupt capability
Encoder myEnc(ENCODER_PIN_1, ENCODER_PIN_2);

long oldPosition  = -999; 


// END ENCODER

// MENU DETAILS


int menuSelection = 0;
struct menu
{
   int menuItem;
   int initialValue;
   int currentValue;
   int minValue;
   int maxValue;
   int ledHue;
   //void increment() {currentValue++;}
};

typedef struct menu Menu;

  Menu patternColorMenu;
  Menu frameRateMenu;
  Menu motorSpeedMenu;  
  Menu fineFrameRateMenu;
  Menu onTimeMenu;
  //Menu twiddlerMenu;
  //Menu cubeMultiplier;

  //http://www.cplusplus.com/doc/tutorial/structures/
  Menu * pCurrentMenu;
void readEncoder(Menu& currMenu);

// LED states. Some microcontrollers wire their built-in LED the reverse.
const int LED_ON = HIGH;
const int LED_OFF = LOW;

// END BUTTON DEBUG

//MOTOR and SERVO Stuff

#define MOTOR_START_SPEED 60

// One button wired to the pin at BUTTON_PIN. Automatically uses the default
// ButtonConfig. The alternative is to call the AceButton::init() method in
// setup() below.
AceButton button(BUTTON_PIN, LOW, 0);

void handleEvent(AceButton*, uint8_t, uint8_t);
Ticker rpmTimer(calcMicrosPerTick, MICROS_PER_SAMPLE,0, MICROS_MICROS); // 1 second ==1 mil micros == 1000 millis

void setup() {
  // initialize built-in LED as an output
  pinMode(LED_PIN, OUTPUT);

  // Button is connected to 3.3 V so it runs revers logic from default config.  set this at constructor.
  pinMode(BUTTON_PIN, INPUT);
  
  setupIRInterrupt();
  
  setupOLED();
  
  ButtonConfig* buttonConfig = button.getButtonConfig();
  buttonConfig->setEventHandler(handleEvent);
  buttonConfig->setFeature(ButtonConfig::kFeatureDoubleClick);
  buttonConfig->setFeature(
  ButtonConfig::kFeatureSuppressClickBeforeDoubleClick);
  buttonConfig->setFeature(ButtonConfig::kFeatureSuppressAfterClick);
  buttonConfig->setFeature(ButtonConfig::kFeatureSuppressAfterDoubleClick);

  //setup default menu params
                      //menuItem, init,current, min, max, ledhue
  patternColorMenu  =   {0,1,10,1,254,100};
  frameRateMenu     =   {1,254,250,1,254,50};
  motorSpeedMenu    =   {2,60,60,45,179,0}; // TO DO Need initial value after spin up, then make adjustments to that.  NOT ZERO
  fineFrameRateMenu =   {3,3,3,3,254,150};
  onTimeMenu        =   {4,1,10,1,254,100};
  // set up the pattern menu as init menu
  // later we reassign pCurrentMenu to each menu when we change
  // then all functions will act on the pCurrentMenu reference and act on whatever menu is there.
  pCurrentMenu = &frameRateMenu;

  // ESP NOW STUFF make a function 
  delay(10);
  //Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin();
  //Serial.print("\r\n\r\nDevice MAC: ");
 // Serial.println(WiFi.macAddress());
  //Serial.println("\r\nESP_Now Controller.");
  
  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  //esp_now_register_send_cb(OnDataSent); not used yet but can chekc status of send
  
  delay(10);
  // Register peer
  esp_now_peer_info_t peerInfo;
  
  memcpy(peerInfo.peer_addr, remoteMac, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
  // SERVO
  esc.attach(SERVO_PIN);
  motorArm(MOTOR_START_SPEED);
}

void loop() {
  
  //rpmTimer.update();
  //noInterrupts(); // no interrupting this loop with IR or encoder stuff for a sec
  
  // Should be called every 20ms or faster for the default debouncing time
  // of ~50ms.
  button.check();
  readEncoder(*pCurrentMenu);
  

  // if transmitNeeded flat is true then we need to transmit the data.  would be nice to get OK on receipt here.
  // make this a function instead of calling it in loop all sloppy like.
  if(transmitNeededFlag){
    // set all the current values before sending them to the cube.
    
    transmitData[PATTERN] = patternColorMenu.currentValue;
    //transmitData[FRAMERATE] = frameRateMenu.currentValue; //TODO Lets just send it micros
    transmitData[FRAMERATE] = 250;
    transmitData[MOTORSPEED] = motorSpeedMenu.currentValue;
    transmitData[FINEFRAMERATE] = fineFrameRateMenu.currentValue;
    transmitData[ONTIME] = onTimeMenu.currentValue; 
       
    result = esp_now_send(remoteMac, transmitData, MENU_ITEMS);
    
    //transmitNeededFlag = false;  COMMENTED FOR  TESTING ONLY
  }
  // we want to send the cube three values, the pattern, the cube servo speed and the frame rate.  
    // pattern will be as many patterns as we have available, maybe just a couple
    // servo speed will be set here and the motor is controlled on this device, however we want the cube to know its speed setting
    // we also want to send the frame rate, which should generally match the speed, but we may also fiddle with this in small increments/
    // for now we are using bytes of 0-254 but we may need to concatenate bytes on the cube side for more resolution.
    
  //servoCRate = map(potCState[i],0,1023,0,179); 
 //  esc.write(motorSpeedMenu.currentValue); TODO IMPLEMENT MOTOR CONTROLS ON BRUSHED MOTOR
  //Serial.println(motorSpeedMenu.currentValue);
  //delay(1000);
  //interrupts();  // turns interrupts back on
 

}

void calcMicrosPerTick(){
  //micros divided by slots counted = microsPerSlot
  // probably need to worry about overflow for slow speed 
  //so just set to zero if large or rolling over
  //microsPerTick = MICROS_PER_SAMPLE/SlotsCounted;
 // microsPerRevolution = (MICROS_PER_SAMPLE/SlotsCounted)/SLOTS_IN_DISK;
  //microsPerSide = (MICROS_PER_SAMPLE/SlotsCounted)/(SLOTS_IN_DISK/SLOTS_PER_SIDE);

  // for now we will get a rough millis and sen that to frame rate. it gets mulitplied by 1000 in receiver so we divide here and send millis
 

  //if(microsPerSide > 100000) {
    millisPerSide  = 50;
    
  //} 
  //else {
    //millisPerSide = (microsPerSide / 1000);
  //};
  //map fine frame rate to 0-254 val from 0-9999 in remainder. TODO probably a bug here.

  //we want this to go fast so we should just record vals and do this elswhere
  // set the counter back to zero to start another sample form MICROS_PER_SAMPLE
  SlotsCounted = 0;
  
}

void setupIRInterrupt() {
  // IR Infrared sensor
  
pinMode(IR_PIN, INPUT_PULLUP); 
attachInterrupt(digitalPinToInterrupt(13), sens, RISING);
rpmTimer.start();
}

void setupOLED () {
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // Address 0x3C for 128x32

  Serial.println("OLED begun");

  // Show image buffer on the display hardware.
  // Since the buffer is intialized with an Adafruit splashscreen
  // internally, this will display the splashscreen.
  display.display();
  delay(1000);

  // Clear the buffer.
  display.clearDisplay();
  display.display();

   // text display tests
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.print("Connecting to SSID\n'adafruit':");
  display.print("connected!");
  display.println("IP: 10.0.1.23");
  display.println("Sending val #0");
  display.setCursor(0,0);
  display.display(); // actually display all of the above

}

void displaySomething (){
  display.print("C");
  delay(10);
  yield();
  display.display();
}
// The event handler for the button.
void handleEvent(AceButton* /* button */, uint8_t eventType,
    uint8_t /* buttonState */) {
  switch (eventType) {
    case AceButton::kEventClicked:
    case AceButton::kEventReleased:
      //Serial.println("Single Click");
      // click switches menu item so increment menuSelection++
      nextMenu();
      
      break;
    case AceButton::kEventDoubleClicked:
      //Serial.println("DoubleClick");
      toggleMotor();
      // double click turns on motor and turns it off again. toggle 0 and 1
      // toggleMotor() start stop
      break;
  }
}

// read position on encoder

void readEncoder(Menu& currMenu){
  // take a reference to the curr menu and increment or decrement.  we dont care about old position, just if its a positive or negative change. 
  // TODO need to ensure it doesnt overflow and do some additional checking here to loop around
  // create blink when min max limit is reached.
  long newPosition = myEnc.read();
  if (newPosition != oldPosition) {
    if (oldPosition > newPosition) {
      if(currMenu.currentValue > currMenu.minValue){
        currMenu.currentValue--;
      }
    }
      else {
         if(currMenu.currentValue < currMenu.maxValue){
          currMenu.currentValue++;
          }
      }
    oldPosition = newPosition;

    //OLED 
    displaySomething();
    
    //Serial.print("Value Changed to: ");
    //Serial.println(currMenu.currentValue);
    transmitNeededFlag = true;
    
  }
}

void nextMenu(){
  // convert to turnary
  if (menuSelection < MENU_ITEMS){menuSelection++;}
  if (menuSelection == MENU_ITEMS){menuSelection = 0;}

  switch(menuSelection){
    case 0: pCurrentMenu = &patternColorMenu;
      break;
    case 1: pCurrentMenu = &frameRateMenu;
      break;
    case 2: pCurrentMenu = &motorSpeedMenu;
      break;
    case 3: pCurrentMenu = &fineFrameRateMenu;
      break;
    case 4: pCurrentMenu = &onTimeMenu;
      break;
    default: pCurrentMenu = &patternColorMenu;
  }
  Serial.print("Menu is: ");
  Serial.println(menuSelection);
  Serial.print("CurrValue in Menu is: ");
  Serial.println(pCurrentMenu->currentValue);
  // set LED to hue of pCurrentMenu
  // set starting encoder value to whatever the value was when we were last at this menu
}

void toggleMotor(){
// This will start up the motor or shut it down smoothly
  if (motorRunning){
    //if motor running is true, we stop...
    //Serial.println("Stopping Motor...");
    motorDisarm(motorSpeedMenu.currentValue);
    // stop the motor! 
    motorRunning = false;
   }  else {
    //Serial.println("Starting Motor...");
    //start the motor
    motorArm(STARTUP_SPEED);
    motorRunning = true;
   }
}
void motorArm(int start_speed)
{
    int i;
    for (i=0; i < start_speed; i++) {
      motorSetSpeed(i);
      //Serial.println(i);
      delay(100);
      // TODO get rid of delay use timer?
    }
}

void motorDisarm( int currSpeed)
{
      int i;
    for (i=currSpeed; i > 50; i--) {
      //increment currspeed down until its at 50
      motorSetSpeed(i);
      //Serial.println(i);
      delay(80);
      // TODO get rid of delay use timer?
    }
  
}
void motorSetSpeed(int motorSpeed)
{
    esc.write(motorSpeed);
    motorSpeedMenu.currentValue = motorSpeed;     
}
// pass by reference. send in a menu and it will be acted upon rather than copied
//void PassByReference(Student& who); //prototype
