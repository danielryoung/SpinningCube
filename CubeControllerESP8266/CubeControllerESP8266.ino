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

#include <ESP8266WiFi.h>
extern "C" {
  #include <espnow.h>
}

#include <Servo.h>

Servo esc;


#define WIFI_CHANNEL 1

//MAC ADDRESS OF THE DEVICE YOU ARE SENDING TO
//byte remoteMac[] = {0x3C, 0x71, 0xBF, 0x29, 0x52, 0x3C};
byte remoteMac[] = {0x2C, 0x3A, 0xE8, 0x4E, 0x3C, 0xAE};

#define MENU_ITEMS 3
// we want to have a menu for each value we are sending

byte cnt=0;
byte transmitData[MENU_ITEMS];
bool transmitNeededFlag = true;
byte result;

bool motorRunning = false;
 
// End Transmitter and Servo

// BUTTON MENU STUFF
#include <AceButton.h>
using namespace ace_button;

// The pin number attached to the button.
const int BUTTON_PIN = D5;

// ENCODER STUFF

#include <Encoder.h>

// Change these two numbers to the pins connected to your encoder.
//   Best Performance: both pins have interrupt capability
//   Good Performance: only the first pin has interrupt capability
//   Low Performance:  neither pin has interrupt capability
Encoder myEnc(D6, D7);

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
  //Menu twiddlerMenu;
  //Menu cubeMultiplier;

  //http://www.cplusplus.com/doc/tutorial/structures/
  Menu * pCurrentMenu;
void readEncoder(Menu& currMenu);
// DEBUG
int LED_PIN = LED_BUILTIN;


// LED states. Some microcontrollers wire their built-in LED the reverse.
const int LED_ON = HIGH;
const int LED_OFF = LOW;

// END BUTTON DEBUG

//MOTOR and SERVO Stuff
// this is actually D2 on the d1 knock offs
#define SERVO_PIN D4
#define MOTOR_START_SPEED 60

// One button wired to the pin at BUTTON_PIN. Automatically uses the default
// ButtonConfig. The alternative is to call the AceButton::init() method in
// setup() below.
AceButton button(BUTTON_PIN, LOW, 0);

void handleEvent(AceButton*, uint8_t, uint8_t);

void setup() {
  // initialize built-in LED as an output
  pinMode(LED_PIN, OUTPUT);

  // Button is connected to 3.3 V so it runs revers logic from default config.  set this at constructor.
  pinMode(BUTTON_PIN, INPUT);

  
  ButtonConfig* buttonConfig = button.getButtonConfig();
  buttonConfig->setEventHandler(handleEvent);
  buttonConfig->setFeature(ButtonConfig::kFeatureDoubleClick);
  buttonConfig->setFeature(
  ButtonConfig::kFeatureSuppressClickBeforeDoubleClick);
  buttonConfig->setFeature(ButtonConfig::kFeatureSuppressAfterClick);
  buttonConfig->setFeature(ButtonConfig::kFeatureSuppressAfterDoubleClick);

  //setup default menu params
                      //menuItem, init,current, min, max, ledhue
  patternColorMenu  =   {0,0,10,0,254,100};
  frameRateMenu     =   {1,0,15,0,255,50};
  motorSpeedMenu    =   {2,60,60,5,180,0}; // TO DO Need initial value after spin up, then make adjustments to that.  NOT ZERO
  
  // set up the pattern menu as init menu
  // later we reassign pCurrentMenu to each menu when we change
  // then all functions will act on the pCurrentMenu reference and act on whatever menu is there.
  pCurrentMenu = &patternColorMenu;

  // ESP NOW STUFF make a function 
  delay(10);
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin();
  Serial.print("\r\n\r\nDevice MAC: ");
  Serial.println(WiFi.macAddress());
  Serial.println("\r\nESP_Now Controller.");
  esp_now_init();
  delay(10);
  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_add_peer(remoteMac, ESP_NOW_ROLE_COMBO, WIFI_CHANNEL, NULL, 0);

  // SERVO
  esc.attach(SERVO_PIN);
  motorArm(MOTOR_START_SPEED);
}

void loop() {
  // Should be called every 20ms or faster for the default debouncing time
  // of ~50ms.
  button.check();
  readEncoder(*pCurrentMenu);

  // if transmitNeeded flat is true then we need to transmit the data.  would be nice to get OK on receipt here.
  // make this a function instead of calling it in loop all sloppy like.
  if(transmitNeededFlag){
    // set all the current values before sending them to the cube.
    
    transmitData[0] = patternColorMenu.currentValue;
    transmitData[1] = frameRateMenu.currentValue;
    transmitData[2] = motorSpeedMenu.currentValue;
    
    result = esp_now_send(remoteMac, transmitData, MENU_ITEMS);
    
    transmitNeededFlag = false;
  }
  // we want to send the cube three values, the pattern, the cube servo speed and the frame rate.  
    // pattern will be as many patterns as we have available, maybe just a couple
    // servo speed will be set here and the motor is controlled on this device, however we want the cube to know its speed setting
    // we also want to send the frame rate, which should generally match the speed, but we may also fiddle with this in small increments/
    // for now we are using bytes of 0-254 but we may need to concatenate bytes on the cube side for more resolution.
    
  //servoCRate = map(potCState[i],0,1023,0,179); 
  esc.write(motorSpeedMenu.currentValue);
  //Serial.println(motorSpeedMenu.currentValue);

}

// The event handler for the button.
void handleEvent(AceButton* /* button */, uint8_t eventType,
    uint8_t /* buttonState */) {
  switch (eventType) {
    case AceButton::kEventClicked:
    case AceButton::kEventReleased:
      Serial.println("Single Click");
      // click switches menu item so increment menuSelection++
      nextMenu();
      
      break;
    case AceButton::kEventDoubleClicked:
      Serial.println("DoubleClick");
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
    Serial.print("Value Changed to: ");
    Serial.println(currMenu.currentValue);
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
    Serial.println("Stopping Motor...");
    // get the motor running.  
    motorRunning = false;
   }  else {
    Serial.println("Starting Motor...");
    // stop the motor!
    motorRunning = true;
   }
}
void motorArm(int start_speed)
{
    int i;
    for (i=0; i < start_speed; i+=5) {
      motorSetSpeed(i);
      //Serial.println(i);
      delay(100);
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
