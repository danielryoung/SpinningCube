# CLAUDE.md - SpinningCube Repository Guide

**Last Updated:** 2025-11-14
**Repository:** SpinningCube - Wireless LED Cube Light Show System

---

## Table of Contents

1. [Project Overview](#project-overview)
2. [Repository Structure](#repository-structure)
3. [Active vs Deprecated Code](#active-vs-deprecated-code)
4. [System Architecture](#system-architecture)
5. [Hardware Configuration](#hardware-configuration)
6. [Key Libraries & Dependencies](#key-libraries--dependencies)
7. [Communication Protocol](#communication-protocol)
8. [Development Workflows](#development-workflows)
9. [Code Conventions](#code-conventions)
10. [Common Tasks](#common-tasks)
11. [Troubleshooting](#troubleshooting)

---

## Project Overview

SpinningCube is a **wireless LED cube art installation** (also called "Melty Cube") that creates rotating light patterns through synchronized LED animations. The system consists of:

- **Controller Device**: Remote control with rotary encoder, button, and motor ESC
- **Cube Receiver**: Rotating LED cube with 48 APA102 LEDs
- **Communication**: ESP_NOW wireless protocol (2.4GHz)
- **Core Feature**: Frame-rate synchronization between motor rotation and LED display

### Primary Use Case
The cube rotates on a motor while LEDs display frame-by-frame animations synchronized to the rotation speed, creating persistence-of-vision (POI) effects for art installations and performances.

---

## Repository Structure

```
SpinningCube/
├── README.md                                    # Hardware requirements & setup notes
├── CLAUDE.md                                    # This file - AI assistant guide
│
├── CubeReceiverESP8266WorkingPostTOPs/         # ✅ PRODUCTION RECEIVER (Primary)
│   └── CubeReceiverESP8266WorkingPostTOPs.ino
│
├── CubeControllerESP8266/                      # ✅ PRODUCTION CONTROLLER (ESP8266)
│   └── CubeControllerESP8266.ino
│
├── CubeControllerESP32/                        # ✅ PRODUCTION CONTROLLER (ESP32)
│   └── CubeControllerESP32.ino                 # With IR speed sensing
│
├── CubeControllerESP8266OLED/                  # 🔧 VARIANT with OLED display
│   ├── CubeControllerESP8266OLED.ino
│   ├── Adafruit_SSD1306.cpp                    # Custom OLED library
│   └── Adafruit_SSD1306.h
│
├── CubeReceiverESP8266/                        # ⚠️ DEPRECATED (older receiver)
│   └── CubeReceiverESP8266.ino
│
├── CubeReceiverESP8266-MFaire/                 # ⚠️ EVENT-SPECIFIC (Maker Faire)
│   └── CubeReceiverESP8266-MFaire.ino          # Reduced LEDs (24 vs 48)
│
├── CubeControllerOLD/                          # ⚠️ DEPRECATED (3 menus vs 5)
│   └── CubeControllerOLD.ino
│
├── Fire2012WorkingTest/                        # 🧪 TEST - Fire animation
│   └── Fire2012WorkingTest.ino
│
├── FireandServoUnoWorks/                       # 🧪 TEST - Arduino Uno variant
│   └── FireandServoUnoWorks.ino
│
├── MotorTotem/                                 # 🧪 TEST - Motor + Bluetooth
│   └── MotorTotem.ino
│
├── basicopticalencoder/                        # 🧪 TEST - Encoder basics
│   └── basicopticalencoder.ino
│
├── ValueReceiver/                              # 🧪 TEST - ESP_NOW debugging
│   └── ValueReceiver.ino
│
└── esctest2/                                   # 🧪 TEST - ESC control
    └── esctest2.ino
```

### Legend
- ✅ **Production Code** - Actively used, well-tested
- 🔧 **Variant** - Alternative configuration
- ⚠️ **Deprecated** - Old versions, kept for reference
- 🧪 **Test/Debug** - Development/testing sketches

---

## Active vs Deprecated Code

### ✅ USE THESE (Production-Ready)

**Receiver:**
- **CubeReceiverESP8266WorkingPostTOPs** - Current production receiver
  - 48 APA102 LEDs (4 strips × 12 LEDs)
  - Pacifica ocean wave animation
  - 5-parameter menu system
  - Frame rate synchronization

**Controllers:**
- **CubeControllerESP8266** - Stable ESP8266 controller (recommended)
  - 5 menu items (Pattern, FrameRate, MotorSpeed, FineFrameRate, OnTime)
  - Rotary encoder + button input
  - ESC motor control

- **CubeControllerESP32** - ESP32 variant with advanced features
  - IR sensor for motor speed feedback
  - Enhanced timing precision
  - Same 5-menu system

### ⚠️ AVOID THESE (Deprecated/Limited)

- **CubeReceiverESP8266** - Older receiver, missing Pacifica pattern
- **CubeReceiverESP8266-MFaire** - Maker Faire special (24 LEDs, reduced brightness)
- **CubeControllerOLD** - Original controller with only 3 menus

### 🔧 VARIANTS (Situational Use)

- **CubeControllerESP8266OLED** - Use if OLED display needed
  - Includes custom Adafruit_SSD1306 library
  - May have compatibility issues with newer Arduino IDE

---

## System Architecture

```
┌──────────────────────────┐         ESP_NOW         ┌────────────────────────────┐
│   CONTROLLER DEVICE      │      (2.4GHz WiFi)      │    CUBE RECEIVER          │
├──────────────────────────┤  ◄──────────────────►   ├────────────────────────────┤
│                          │   5-byte packets        │                            │
│ Hardware:                │   on value change       │ Hardware:                  │
│ • Rotary Encoder (D6/D7) │                         │ • APA102 LEDs × 48         │
│ • Push Button (D5)       │                         │ • DATA_PIN: D7 (MOSI)      │
│ • ESC/Motor (D4)         │                         │ • CLOCK_PIN: D5 (SCK)      │
│ • OLED Display (opt)     │                         │ • TickTwo Timer            │
│                          │                         │                            │
│ Software:                │                         │ Software:                  │
│ • Menu navigation        │                         │ • ESP_NOW receiver         │
│ • Encoder position track │                         │ • FastLED control          │
│ • Motor speed control    │                         │ • Frame rate calculation   │
│ • ESP_NOW transmitter    │                         │ • Pattern generation       │
│ • Button events          │                         │ • Side mapping             │
│                          │                         │                            │
│ Platform:                │                         │ Platform:                  │
│ • ESP8266 / ESP32        │                         │ • ESP8266                  │
└──────────────────────────┘                         └────────────────────────────┘
```

### Data Flow

1. **User Input** → Rotary encoder turned or button pressed
2. **Menu System** → Current menu value incremented/decremented
3. **Transmission** → ESP_NOW sends 5-byte array when value changes
4. **Reception** → Cube receives packet, updates `txrxData[]` array
5. **Calculation** → Frame interval recalculated based on framerate values
6. **LED Update** → Pattern generated and mapped to physical LEDs
7. **Timer Callback** → `nextSide()` advances to next frame at calculated interval
8. **Display** → FastLED.show() updates physical LED strips

---

## Hardware Configuration

### Controller Hardware

**ESP8266 Version:**
```cpp
Pin Configuration:
- D4: SERVO_PIN (ESC/Motor control)
- D5: BUTTON_PIN (Menu navigation)
- D6: ENCODER_PIN_1 (Rotary encoder A)
- D7: ENCODER_PIN_2 (Rotary encoder B)
- D1: SCL (OLED - optional)
- D2: SDA (OLED - optional)
```

**ESP32 Version:**
```cpp
Pin Configuration:
- 13: IR_PIN (IR sensor for speed feedback)
- 14: ENCODER_PIN_1 (Rotary encoder A)
- 12: ENCODER_PIN_2 (Rotary encoder B)
- 15: SERVO_PIN (ESC/Motor control)
- 21: SDA (OLED - optional)
- 22: SCL (OLED - optional)
```

**Bill of Materials (Controller):**
- 1× ESP8266 (NodeMCU/D1 Mini) or ESP32 board
- 1× Rotary encoder with integrated switch
- 1× Drone motor (low speed, high torque)
- 1× 12A ESC (Electronic Speed Controller)
- 2× 18650 batteries + holder
- 1× 3.3V voltage regulator
- Optional: 128×32 OLED display (I2C)
- Optional: IR reflective sensor (ESP32 only)

### Receiver (Cube) Hardware

**Pin Configuration:**
```cpp
- D7: DATA_PIN (APA102 SPI MOSI)
- D5: CLOCK_PIN (APA102 SPI SCK)
```

**LED Layout:**
```
Total LEDs: 48
Layout: 4 vertical strips, 12 LEDs per strip

Strip mapping:
- Side 0: LEDs 0-11
- Side 1: LEDs 12-23
- Side 2: LEDs 24-35
- Side 3: LEDs 36-47

Physical orientation varies by installation
```

**Bill of Materials (Cube):**
- 1× ESP8266 (NodeMCU/D1 Mini)
- 4× APA102 LED strips (12 LEDs each)
- 1× Logic level shifter (3.3V → 5V for LEDs)
- 1× 3.3V-5V charge controller
- 1× 18650 battery + holder
- Wire, connectors, cube frame structure

---

## Key Libraries & Dependencies

### Required Arduino Libraries

**Communication:**
```cpp
#include <ESP8266WiFi.h>      // ESP8266 WiFi stack
// OR
#include <WiFi.h>             // ESP32 WiFi stack

#include <espnow.h>           // Espressif NOW protocol
#include <user_interface.h>   // Low-level ESP8266 control (ESP8266 only)
```

**LED Control:**
```cpp
#include <FastLED.h>          // LED strip control
// Configuration:
// - Chipset: APA102
// - Color order: RGB
// - Data rate: default SPI speed
```

**Timing:**
```cpp
#include <TickTwo.h>          // Timer library for frame control
// IMPORTANT: Must use latest version!
// See README.md for ticker library update instructions
```

**Input Handling:**
```cpp
#include <Encoder.h>          // Quadrature encoder position tracking
#include <AceButton.h>        // Button debouncing & event handling
#include <Servo.h>            // ESC control (uses PWM servo protocol)
```

**Display (Optional):**
```cpp
#include <Adafruit_SSD1306.h> // OLED display driver
#include <Adafruit_GFX.h>     // Graphics primitives
#include <Wire.h>             // I2C communication
#include <SPI.h>              // SPI communication
```

### Library Installation Notes

**Critical: Ticker Library Update**
From README.md, the ticker library MUST be updated:

1. Download latest ticker package as ZIP
2. Backup existing: `Arduino/packages/esp8266/hardware/esp8266/[version]/libraries/Ticker`
3. Replace with downloaded Ticker folder
4. Restart Arduino IDE

Without this update, frame timing may be unstable.

---

## Communication Protocol

### ESP_NOW Protocol

**Overview:**
- **Type:** Connectionless WiFi protocol (Espressif proprietary)
- **Range:** ~250 meters line-of-sight
- **Latency:** Very low (<10ms typical)
- **Payload:** Up to 250 bytes (this project uses 5 bytes)
- **Frequency:** 2.4GHz (same as WiFi, but no router needed)

### Implementation

**Controller Setup (Transmitter):**
```cpp
// MAC address of receiver (hardcoded in sketch)
uint8_t remoteMac[] = {0x5C, 0xCF, 0x7F, 0x08, 0x11, 0x8C};

// Initialize ESP_NOW
WiFi.mode(WIFI_STA);
esp_now_init();
esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
esp_now_add_peer(remoteMac, ESP_NOW_ROLE_COMBO, WIFI_CHANNEL, NULL, 0);

// Transmit when value changes
if (transmitNeededFlag) {
  esp_now_send(remoteMac, transmitData, MENU_ITEMS);
  transmitNeededFlag = false;
}
```

**Receiver Setup:**
```cpp
// Initialize ESP_NOW
WiFi.mode(WIFI_STA);
esp_now_init();
esp_now_set_self_role(ESP_NOW_ROLE_COMBO);

// Register receive callback
esp_now_register_recv_cb([](uint8_t *mac, uint8_t *data, uint8_t len) {
  memcpy(txrxData, data, len);
  changeFrameInterval();  // Update timing based on new framerate
});
```

### Data Structure

**5-Byte Menu Array:**
```cpp
#define MENU_ITEMS  5

// Array indices (menu parameters)
#define PATTERN       0   // LED hue: 0-254 (HSV color wheel)
#define FRAMERATE     1   // Frame duration milliseconds: 0-254
#define MOTORSPEED    2   // Motor speed: 45-179 (servo range)
#define FINEFRAMERATE 3   // Fine adjust: 0-254 → 0-9999 microseconds
#define ONTIME        4   // LED on-time: 0-254 (duty cycle percentage)

uint8_t transmitData[MENU_ITEMS];  // Transmitted by controller
uint8_t txrxData[MENU_ITEMS];      // Received by cube
```

**Timing Calculation:**
```cpp
// On receiver - converts framerate values to microseconds
uint32_t onDurationMicros =
  ((txrxData[FRAMERATE] * 1000) +                    // Coarse: 0-254ms
   map(txrxData[FINEFRAMERATE], 0, 254, 0, 9999));  // Fine: 0-9.999ms

frameTimer.interval(onDurationMicros);  // Update timer interval
```

### Finding Receiver MAC Address

When setting up a new receiver, you need its MAC address:

```cpp
// Add to receiver setup() to print MAC
Serial.println(WiFi.macAddress());
```

Then update `remoteMac[]` in controller sketch.

---

## Development Workflows

### Setting Up Arduino IDE

1. **Install ESP8266/ESP32 Board Support**
   - File → Preferences
   - Add to Additional Board URLs:
     - ESP8266: `http://arduino.esp8266.com/stable/package_esp8266com_index.json`
     - ESP32: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
   - Tools → Board → Boards Manager
   - Install "esp8266" and/or "esp32"

2. **Install Required Libraries**
   - Sketch → Include Library → Manage Libraries
   - Install:
     - FastLED
     - TickTwo (then update per README.md)
     - Encoder
     - AceButton
     - Adafruit SSD1306 (if using OLED)
     - Adafruit GFX (if using OLED)

3. **Board Selection**
   - Controller: Tools → Board → ESP8266/ESP32 → NodeMCU 1.0 (or your board)
   - Receiver: Tools → Board → ESP8266 → NodeMCU 1.0
   - Set upload speed: 115200 or 921600

### Typical Development Workflow

**Making Changes to Controller:**
1. Open `CubeControllerESP8266/CubeControllerESP8266.ino`
2. Make changes (e.g., adjust menu ranges, add features)
3. Verify compilation: Sketch → Verify/Compile
4. Upload: Sketch → Upload (with board connected via USB)
5. Test: Open Serial Monitor (115200 baud) for debug output
6. Commit changes to git with descriptive message

**Making Changes to Receiver:**
1. Open `CubeReceiverESP8266WorkingPostTOPs/CubeReceiverESP8266WorkingPostTOPs.ino`
2. Make changes (e.g., new LED patterns, timing adjustments)
3. Verify compilation
4. Upload to receiver ESP8266
5. Test with controller running
6. Commit changes to git

**Creating New LED Pattern:**
1. Edit receiver sketch
2. Add pattern function similar to `pacifica_loop()` (line 282)
3. Add pattern selection in menu system
4. Test pattern by setting PATTERN menu value
5. Update comments documenting pattern

### Git Workflow

**Current Branch:**
```bash
# You are on: claude/claude-md-mhzdeb75rsfb7rdl-01F8qp9DFi8orJgpM9bwQPap
# This is your development branch for this session
```

**Committing Changes:**
```bash
# Stage changes
git add CubeControllerESP8266/CubeControllerESP8266.ino

# Commit with descriptive message
git commit -m "Add new rainbow pattern to LED controller"

# Push to remote (use -u for first push of branch)
git push -u origin claude/claude-md-mhzdeb75rsfb7rdl-01F8qp9DFi8orJgpM9bwQPap
```

**Branch Naming Convention:**
- Development branches start with `claude/`
- Include session ID in branch name
- Never push to master/main without PR

---

## Code Conventions

### File Organization

**Arduino Sketch Structure:**
```cpp
// 1. Includes
#include <ESP8266WiFi.h>
#include <FastLED.h>
// ...

// 2. Definitions
#define LED_COUNT 48
#define DATA_PIN D7
// ...

// 3. Global Variables
CRGB leds[LED_COUNT];
uint8_t txrxData[MENU_ITEMS];
// ...

// 4. Function Prototypes (if needed)
void nextSide();
void pacifica_loop();
// ...

// 5. setup()
void setup() {
  Serial.begin(115200);
  // initialization code
}

// 6. loop()
void loop() {
  // main loop code
}

// 7. Helper Functions
void nextSide() { ... }
void changeFrameInterval() { ... }
// ...
```

### Naming Conventions

**Variables:**
- camelCase for variables: `frameTimer`, `motorSpeed`
- ALL_CAPS for constants: `MENU_ITEMS`, `DATA_PIN`
- Descriptive names: `transmitNeededFlag` not `tnf`

**Functions:**
- camelCase: `nextSide()`, `changeFrameInterval()`
- Action-oriented names: `motorArm()`, `toggleMotor()`

**Files:**
- Sketch name matches folder name (Arduino requirement)
- Descriptive folder names: `CubeReceiverESP8266WorkingPostTOPs`

### Commenting Style

**File Headers:**
```cpp
// SpinningCube Controller
// ESP8266 version with 5-menu system
// Last updated: 2024-XX-XX
```

**Function Comments:**
```cpp
// Advances to next cube side and updates LED display
void nextSide() {
  side++;
  if (side == 4) side = 0;
  FastLED.show();
}
```

**Inline Comments:**
```cpp
frameTimer.interval(onDurationMicros);  // Update timer interval
```

### Code Style

**Indentation:** 2 spaces (Arduino IDE default)

**Braces:**
```cpp
// Opening brace on same line
if (condition) {
  // code
} else {
  // code
}
```

**Serial Output:**
```cpp
// Use descriptive debug messages
Serial.print("Frame interval: ");
Serial.println(onDurationMicros);
```

---

## Common Tasks

### Task 1: Change LED Pattern Colors

**File:** `CubeReceiverESP8266WorkingPostTOPs.ino`

**Location:** Lines 169-180 (approximate)

```cpp
void sendCubeToStrips() {
  for (int i = 0; i < 4; i++) {
    // Change hue calculation here
    hue = txrxData[PATTERN];  // Currently uses transmitted pattern value

    // For custom colors, replace with:
    // hue = 160;  // Fixed blue
    // or
    // hue = map(i, 0, 3, 0, 255);  // Rainbow across sides
  }
}
```

### Task 2: Adjust Motor Speed Range

**File:** `CubeControllerESP8266.ino`

**Location:** Lines 122-125 (approximate)

```cpp
Menu menu3 = {
  "Motor Speed",
  45,    // minValue (change lower bound)
  179,   // maxValue (change upper bound)
  127,   // currentValue (starting speed)
  1      // step
};
```

**Note:** ESC typically accepts 0-180, but safe range is 45-179

### Task 3: Add New Menu Item

**Controller Changes:**
1. Increase `MENU_ITEMS` from 5 to 6
2. Add new menu definition array index
3. Create new Menu struct
4. Add to `menuList[]` array

**Receiver Changes:**
1. Increase `MENU_ITEMS` from 5 to 6
2. Add handling for new menu value in appropriate function
3. Update `txrxData[]` processing

### Task 4: Change Frame Rate Range

**File:** `CubeControllerESP8266.ino`

**Location:** Menu definitions

```cpp
Menu menu2 = {
  "Frame Rate",
  1,     // minValue (1ms minimum)
  250,   // maxValue (250ms maximum)
  50,    // currentValue (starting at 50ms)
  1      // step
};
```

### Task 5: Debug ESP_NOW Communication

**Add to Controller:**
```cpp
// In transmit section
result = esp_now_send(remoteMac, transmitData, MENU_ITEMS);
Serial.print("Send result: ");
Serial.println(result == 0 ? "Success" : "Failed");
```

**Add to Receiver:**
```cpp
// In receive callback
Serial.print("Received: ");
for (int i = 0; i < len; i++) {
  Serial.print(txrxData[i]);
  Serial.print(" ");
}
Serial.println();
```

### Task 6: Create New LED Animation Pattern

**File:** `CubeReceiverESP8266WorkingPostTOPs.ino`

**Template:**
```cpp
// Add new pattern function
void myCustomPattern() {
  // Your pattern code here
  // Example: pulse all LEDs
  uint8_t brightness = beatsin8(60);  // 60 BPM pulse
  for (int i = 0; i < NUM_LEDS; i++) {
    cube[i] = CHSV(txrxData[PATTERN], 255, brightness);
  }
}

// Add to loop() or timer callback
void loop() {
  // Call your pattern
  myCustomPattern();
  stripToCubeMap();  // Map cube[] to leds[]
}
```

---

## Troubleshooting

### ESP_NOW Not Connecting

**Symptoms:** Receiver not responding to controller

**Solutions:**
1. Verify MAC addresses match
   ```cpp
   // On receiver, print MAC:
   Serial.println(WiFi.macAddress());
   ```
2. Check WiFi channel matches (default: channel 1)
3. Verify both devices on same WiFi mode (WIFI_STA)
4. Check power supply - ESP modules need clean 3.3V
5. Reduce distance between devices (test at <2m first)

### LEDs Not Lighting

**Symptoms:** No LED output or flickering

**Solutions:**
1. Check power supply
   - APA102 needs 5V, ESP needs 3.3V
   - Ensure logic level shifter is working
2. Verify pin connections
   - DATA_PIN = D7 (MOSI)
   - CLOCK_PIN = D5 (SCK)
3. Check LED strip polarity (DO/CO, not DI/CI)
4. Test with simple pattern:
   ```cpp
   leds[0] = CRGB::Red;
   FastLED.show();
   ```
5. Verify `NUM_LEDS` matches physical count

### Motor Not Starting

**Symptoms:** ESC beeps but motor doesn't spin

**Solutions:**
1. Arm ESC sequence
   ```cpp
   // ESC calibration (run once)
   esc.write(0);    // Minimum
   delay(2000);
   esc.write(180);  // Maximum
   delay(2000);
   esc.write(90);   // Neutral
   ```
2. Check battery voltage (need >7.4V for most ESCs)
3. Verify ESC ground connected to ESP ground
4. Check motor connections (3-phase, any order works)
5. Ensure SERVO_PIN matches physical connection

### Encoder Not Responding

**Symptoms:** Turning encoder doesn't change values

**Solutions:**
1. Check pin connections match sketch
2. Verify encoder has pullup resistors (or enable internal)
   ```cpp
   pinMode(ENCODER_PIN_1, INPUT_PULLUP);
   pinMode(ENCODER_PIN_2, INPUT_PULLUP);
   ```
3. Test encoder separately:
   ```cpp
   Serial.println(myEnc.read());
   ```
4. Try swapping encoder pins (reverses direction)

### Frame Rate Synchronization Issues

**Symptoms:** LED pattern doesn't align with rotation

**Solutions:**
1. Verify ticker library is updated (see README.md)
2. Increase `FINEFRAMERATE` resolution for precision
3. Check motor speed stability (may need encoder feedback)
4. Adjust frame timing:
   ```cpp
   // Add offset to framerate calculation
   uint32_t onDurationMicros =
     ((txrxData[FRAMERATE] * 1000) + offset);
   ```
5. Use ESP32 controller with IR sensor for accurate speed feedback

### Compilation Errors

**"espnow.h not found":**
- Install ESP8266 board support in Arduino IDE

**"FastLED.h not found":**
- Install FastLED library via Library Manager

**"Ticker issues":**
- Update ticker library per README.md instructions
- Ensure using TickTwo, not Ticker

**"Multiple definition errors":**
- Check for duplicate includes
- Ensure only one .ino file in sketch folder

### Upload Failures

**"Timed out waiting for packet header":**
- Check USB cable connection
- Select correct COM port
- Reduce upload speed (try 115200)
- Press RESET button when "Connecting..." appears

**"Wrong boot mode":**
- Some boards need GPIO0 LOW during boot
- Hold FLASH button while pressing RESET

---

## Additional Resources

### Documentation

- **README.md** - Hardware requirements and ticker library setup
- **Git History** - `git log` for development timeline
- **Code Comments** - Inline documentation in sketches

### Key Git Commits

```
58a6655 - working arduino based version (latest)
56007b5 - Melty Cube Working Build post TOPs (stable milestone)
129575c - Adding esp32 controller version
ca79f7f - add basic encoder for melty hedrons
512146b - Added OLED Screen, doubled LEDs
```

### External References

- **FastLED Library:** http://fastled.io/
- **ESP_NOW Protocol:** https://www.espressif.com/en/products/software/esp-now
- **APA102 LEDs:** https://cpldcpu.wordpress.com/2014/08/27/apa102/
- **ESP8266 Arduino Core:** https://github.com/esp8266/Arduino

---

## AI Assistant Guidelines

When working with this codebase:

1. **Always specify which sketch you're modifying** (Controller vs Receiver, ESP8266 vs ESP32)

2. **Use production code** - Prefer `CubeReceiverESP8266WorkingPostTOPs` and `CubeControllerESP8266`

3. **Maintain 5-byte protocol** - Don't break ESP_NOW communication by changing array size without updating both sides

4. **Test incrementally** - Changes to timing/patterns should be tested on hardware

5. **Preserve working code** - When making major changes, consider creating a variant folder

6. **Document changes** - Update comments when modifying LED patterns or menu systems

7. **Check both sides** - Controller changes often require receiver updates

8. **Respect hardware limits:**
   - ESP8266: Limited RAM (~80KB free)
   - FastLED: Max ~500 LEDs recommended
   - ESP_NOW: 250-byte payload limit
   - ESC: 45-179 safe range for motor speed

9. **Follow git workflow** - Always commit to feature branches starting with `claude/`

10. **Update this file** - When adding major features, update CLAUDE.md

---

## Version History

- **2025-11-14:** Initial CLAUDE.md creation (comprehensive repository analysis)
- **Future:** Update after major feature additions

---

*This documentation is maintained for AI assistants working with the SpinningCube codebase. For questions or updates, modify this file and commit changes.*
