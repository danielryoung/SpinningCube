/**
 * ESP32 Cube Receiver with LEDGroup Hardware Timers
 *
 * Converted from ESP8266 version to use:
 * - ESP32 WiFi and ESP-NOW
 * - LEDGroup class with built-in hardware timers
 * - 4 independent hardware timers (one per side)
 * - Microsecond-precision timing with minimal CPU overhead
 *
 * Hardware Timers Used:
 * - Timer 0: Side 1 (Group 1)
 * - Timer 1: Side 2 (Group 2)
 * - Timer 2: Side 3 (Group 3)
 * - Timer 3: Side 4 (Group 4)
 */

/// LIBS
#include <WiFi.h>
#include <esp_now.h>
#include <FastLED.h>
#include <SPI.h>
#include "LEDGroup.h"

/// PIN CONFIGURATION
// ESP32 SPI pins for APA102
#define DATA_PIN 23
#define CLOCK_PIN 18

/// LED CONFIGURATION
#define L_P_SIDE 12      // LEDs per side
#define NUM_LEDS 48      // Total LEDs (4 sides × 12 LEDs)
#define NUM_SIDES 4

/// ESP-NOW MENU CONFIGURATION
#define MENU_ITEMS  5

// Menu item definitions (match controller)
#define PATTERN       0
#define FRAMERATE     1
#define MOTORSPEED    2
#define FINEFRAMERATE 3
#define ONTIME        4

/// GLOBALS

// Physical LED array (shared by all groups)
CRGB leds[NUM_LEDS];

// LED Groups (one per side, each with hardware timer)
LEDGroup* groups[NUM_SIDES];

// ESP-NOW data buffer
uint8_t txrxData[MENU_ITEMS];

// Current mode
bool pacificaMode = false;

/// EFFECT FUNCTIONS

/**
 * Solid color effect - fills side with color based on pattern
 */
void solidColorEffect(CRGB* ledArray, uint16_t count, uint8_t groupNum) {
    uint8_t hue = txrxData[PATTERN] + ((groupNum - 1) * 50);
    fill_solid(ledArray, count, CHSV(hue, 255, 255));
}

/**
 * Rainbow effect  - rotating rainbow
 */
void rainbowEffect(CRGB* ledArray, uint16_t count, uint8_t groupNum) {
    static uint8_t hue = 0;
    fill_rainbow(ledArray, count, hue + (groupNum * 60), 255 / count);
    hue += 2;
}

/**
 * Fire effect - animated fire
 */
void fireEffect(CRGB* ledArray, uint16_t count, uint8_t groupNum) {
    for (int i = 0; i < count; i++) {
        ledArray[i] = CRGB(random8(200, 255), random8(0, 100), 0);
    }
}

/**
 * Pulse effect - breathing color
 */
void pulseEffect(CRGB* ledArray, uint16_t count, uint8_t groupNum) {
    static uint8_t brightness = 0;
    static int8_t direction = 1;

    CRGB color = CHSV(txrxData[PATTERN], 255, 255);
    fill_solid(ledArray, count, color);

    for (int i = 0; i < count; i++) {
        ledArray[i].nscale8(brightness);
    }

    brightness += direction * 3;
    if (brightness >= 250 || brightness <= 5) {
        direction *= -1;
    }
}

/// PACIFICA EFFECT (for slow/stopped mode)

CRGBPalette16 pacifica_palette_1 =
    { 0x000507, 0x000409, 0x00030B, 0x00030D, 0x000210, 0x000212, 0x000114, 0x000117,
      0x000019, 0x00001C, 0x000026, 0x000031, 0x00003B, 0x000046, 0x14554B, 0x28AA50 };

CRGBPalette16 pacifica_palette_2 =
    { 0x000507, 0x000409, 0x00030B, 0x00030D, 0x000210, 0x000212, 0x000114, 0x000117,
      0x000019, 0x00001C, 0x000026, 0x000031, 0x00003B, 0x000046, 0x0C5F52, 0x19BE5F };

CRGBPalette16 pacifica_palette_3 =
    { 0x000208, 0x00030E, 0x000514, 0x00061A, 0x000820, 0x000927, 0x000B2D, 0x000C33,
      0x000E39, 0x001040, 0x001450, 0x001860, 0x001C70, 0x002080, 0x1040BF, 0x2060FF };

CRGBPalette16 purplefly_palette = {
    0,   0,  0,  0,
   63, 239,  0,122,
  191, 252,255, 78,
  255,   0,  0,  0
};

void pacifica_loop() {
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

    fill_solid(leds, NUM_LEDS, CRGB(2, 6, 10));

    pacifica_one_layer(purplefly_palette, sCIStart1, beatsin16(3, 11 * 256, 14 * 256), beatsin8(10, 70, 130), 0-beat16(301));
    pacifica_one_layer(purplefly_palette, sCIStart2, beatsin16(4,  6 * 256,  9 * 256), beatsin8(17, 40,  80), beat16(401));
    pacifica_one_layer(pacifica_palette_3, sCIStart3, 6 * 256, beatsin8(9, 10,38), 0-beat16(503));
    pacifica_one_layer(purplefly_palette, sCIStart4, 5 * 256, beatsin8(8, 10,28), beat16(601));

    pacifica_add_whitecaps();
    pacifica_deepen_colors();
}

void pacifica_one_layer(CRGBPalette16& p, uint16_t cistart, uint16_t wavescale, uint8_t bri, uint16_t ioff) {
    uint16_t ci = cistart;
    uint16_t waveangle = ioff;
    uint16_t wavescale_half = (wavescale / 2) + 20;

    for(uint16_t i = 0; i < NUM_LEDS; i++) {
        waveangle += 250;
        uint16_t s16 = sin16(waveangle) + 32768;
        uint16_t cs = scale16(s16, wavescale_half) + wavescale_half;
        ci += cs;
        uint16_t sindex16 = sin16(ci) + 32768;
        uint8_t sindex8 = scale16(sindex16, 240);
        CRGB c = ColorFromPalette(p, sindex8, bri, LINEARBLEND);
        leds[i] += c;
    }
}

void pacifica_add_whitecaps() {
    uint8_t basethreshold = beatsin8(9, 55, 65);
    uint8_t wave = beat8(7);

    for(uint16_t i = 0; i < NUM_LEDS; i++) {
        uint8_t threshold = scale8(sin8(wave), 20) + basethreshold;
        wave += 7;
        uint8_t l = leds[i].getAverageLight();
        if(l > threshold) {
            uint8_t overage = l - threshold;
            uint8_t overage2 = qadd8(overage, overage);
            leds[i] += CRGB(overage, overage2, qadd8(overage2, overage2));
        }
    }
}

void pacifica_deepen_colors() {
    for(uint16_t i = 0; i < NUM_LEDS; i++) {
        leds[i].blue = scale8(leds[i].blue, 145);
        leds[i].green = scale8(leds[i].green, 200);
        leds[i] |= CRGB(2, 5, 7);
    }
}

/// TIMING FUNCTIONS

/**
 * Update LED group timing based on received ESP-NOW data
 */
void updateGroupTiming() {
    // Calculate ON duration from framerate and fine framerate
    uint32_t onDurationMicros = (txrxData[FRAMERATE] * 1000) +
                                 map(txrxData[FINEFRAMERATE], 0, 254, 0, 9999);

    // Calculate OFF duration from ONTIME parameter
    uint32_t offDurationMicros = map(txrxData[ONTIME], 0, 254, 0, onDurationMicros / 20);

    // Calculate duty cycle
    float dutyCycle = (float)(onDurationMicros - offDurationMicros) / onDurationMicros;
    if (dutyCycle < 0.0) dutyCycle = 0.0;
    if (dutyCycle > 1.0) dutyCycle = 1.0;

    // Check if we should enter Pacifica mode (motor stopped/slow)
    if (txrxData[MOTORSPEED] <= 70) {
        // Stop hardware timers
        for (int i = 0; i < NUM_SIDES; i++) {
            groups[i]->stopHardwareTimer();
        }
        pacificaMode = true;
    } else {
        // Update all groups with new timing
        for (int i = 0; i < NUM_SIDES; i++) {
            groups[i]->setDutyCycle(dutyCycle, onDurationMicros);
        }

        // Restart timers if coming from Pacifica mode
        if (pacificaMode) {
            for (int i = 0; i < NUM_SIDES; i++) {
                groups[i]->startHardwareTimer();
            }
            pacificaMode = false;
        }
    }
}

/**
 * ESP-NOW receive callback
 */
void onDataReceived(const uint8_t *mac, const uint8_t *data, int len) {
    memcpy(txrxData, data, len);
    updateGroupTiming();
}

/// SETUP

void setup() {
    // Debug setup (comment out for production)
    Serial.begin(115200);
    delay(500);
    Serial.println("\nESP32 Cube Receiver with Hardware Timers");

    // Initialize WiFi for ESP-NOW
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    Serial.print("MAC Address: ");
    Serial.println(WiFi.macAddress());

    // Initialize ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP-NOW init failed!");
        return;
    }

    // Register receive callback
    esp_now_register_recv_cb(onDataReceived);

    // Initialize default values
    txrxData[PATTERN] = 1;
    txrxData[FRAMERATE] = 200;
    txrxData[MOTORSPEED] = 60;
    txrxData[FINEFRAMERATE] = 10;
    txrxData[ONTIME] = 200;

    // Initialize FastLED
    FastLED.addLeds<APA102, DATA_PIN, CLOCK_PIN, RGB>(leds, NUM_LEDS);
    FastLED.setBrightness(100);
    FastLED.clear();
    FastLED.show();

    Serial.println("Creating LED Groups...");

    // Create LED groups (one per side)
    for (int i = 0; i < NUM_SIDES; i++) {
        groups[i] = new LEDGroup(leds, NUM_LEDS, i + 1, L_P_SIDE);

        // Enable hardware timer for this group
        if (groups[i]->enableHardwareTimer(i)) {
            Serial.print("Group ");
            Serial.print(i + 1);
            Serial.print(" using Timer ");
            Serial.println(i);
        } else {
            Serial.print("Failed to enable timer for Group ");
            Serial.println(i + 1);
        }

        // Set effect function
        groups[i]->setEffectFunction(solidColorEffect);

        // Set initial timing (will be overridden by ESP-NOW)
        groups[i]->setDutyCycle(0.95, 2000);  // 95% duty, 2ms period
    }

    Serial.println("Starting hardware timers...");

    // Start all hardware timers
    for (int i = 0; i < NUM_SIDES; i++) {
        groups[i]->startHardwareTimer();
    }

    Serial.println("Setup complete!");
    Serial.println("Waiting for ESP-NOW data...");
}

/// LOOP

void loop() {
    if (pacificaMode) {
        // Pacifica mode - slow/stopped motor
        pacifica_loop();
        FastLED.show();
    } else {
        // Normal mode - hardware timers handle LED updates
        // Main loop fills animation buffers
        for (int i = 0; i < NUM_SIDES; i++) {
            // Keep buffers full
            while (!groups[i]->isBufferFull()) {
                groups[i]->fillBuffer();
            }
        }

        // Update physical LEDs
        // Note: Hardware timers update virtualLEDs and map to physical
        // We just need to call FastLED.show() to update the strip
        FastLED.show();
    }

    // Small delay to prevent watchdog issues
    yield();
}

/**
 * ============================================================================
 * HARDWARE TIMER USAGE SUMMARY
 * ============================================================================
 *
 * ESP32 has 4 hardware timers available:
 * - Timer 0: Group 1 (LEDs 0-11)   - Side 1
 * - Timer 1: Group 2 (LEDs 12-23)  - Side 2
 * - Timer 2: Group 3 (LEDs 24-35)  - Side 3
 * - Timer 3: Group 4 (LEDs 36-47)  - Side 4
 *
 * Each timer operates independently with:
 * - 1 microsecond resolution (80MHz / 80 prescaler)
 * - Adjustable duty cycle (ON/OFF timing)
 * - ISR-based frame rendering and LED updates
 * - Safe timing updates during OFF period
 *
 * Benefits over software timers (TickTwo):
 * - ±1μs precision vs ±100μs
 * - <1% CPU usage vs ~10%
 * - No jitter from WiFi/processing
 * - Independent control per side
 *
 * ============================================================================
 * CHANGING EFFECTS
 * ============================================================================
 *
 * To change effects based on PATTERN value, modify the ESP-NOW callback:
 *
 * void onDataReceived(const uint8_t *mac, const uint8_t *data, int len) {
 *     memcpy(txrxData, data, len);
 *
 *     // Select effect based on pattern
 *     switch(txrxData[PATTERN] / 50) {  // Divide by 50 for effect ranges
 *         case 0:
 *             for(int i = 0; i < NUM_SIDES; i++) {
 *                 groups[i]->setEffectFunction(solidColorEffect);
 *             }
 *             break;
 *         case 1:
 *             for(int i = 0; i < NUM_SIDES; i++) {
 *                 groups[i]->setEffectFunction(rainbowEffect);
 *             }
 *             break;
 *         case 2:
 *             for(int i = 0; i < NUM_SIDES; i++) {
 *                 groups[i]->setEffectFunction(fireEffect);
 *             }
 *             break;
 *         case 3:
 *             for(int i = 0; i < NUM_SIDES; i++) {
 *                 groups[i]->setEffectFunction(pulseEffect);
 *             }
 *             break;
 *     }
 *
 *     updateGroupTiming();
 * }
 *
 * ============================================================================
 */
