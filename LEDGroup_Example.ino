/**
 * LEDGroup Integration Example
 *
 * This example shows how to integrate the LEDGroup class into your spinning cube project.
 * The LEDGroup class provides:
 * - Virtual LED arrays mapped to physical strips
 * - Frame buffering for smooth animations
 * - Square wave timing control for multiplexed displays
 * - Custom effect functions per group
 */

#include <FastLED.h>
#include "LEDGroup.h"

// LED Strip Configuration
#define DATA_PIN 7
#define CLOCK_PIN 5
#define NUM_LEDS 48
#define LEDS_PER_SIDE 12
#define NUM_SIDES 4

// Physical LED array (shared by all groups)
CRGB leds[NUM_LEDS];

// Create LED groups for each side of the cube
LEDGroup* side1;
LEDGroup* side2;
LEDGroup* side3;
LEDGroup* side4;

// Custom effect function examples
void rainbowEffect(CRGB* leds, uint16_t count, uint8_t groupNum) {
    static uint8_t hue = 0;
    fill_rainbow(leds, count, hue + (groupNum * 60), 255 / count);
    hue++;
}

void pulseEffect(CRGB* leds, uint16_t count, uint8_t groupNum) {
    static uint8_t brightness = 0;
    static int8_t direction = 1;

    CRGB color;
    switch(groupNum) {
        case 1: color = CRGB::Red; break;
        case 2: color = CRGB::Green; break;
        case 3: color = CRGB::Blue; break;
        case 4: color = CRGB::Purple; break;
        default: color = CRGB::White;
    }

    fill_solid(leds, count, color);
    for(int i = 0; i < count; i++) {
        leds[i].nscale8(brightness);
    }

    brightness += direction * 5;
    if(brightness >= 250 || brightness <= 5) {
        direction *= -1;
    }
}

void sparkleEffect(CRGB* leds, uint16_t count, uint8_t groupNum) {
    // Fade all LEDs
    for(int i = 0; i < count; i++) {
        leds[i].nscale8(200);
    }

    // Add random sparkles
    if(random8() < 30) {
        leds[random16(count)] = CRGB::White;
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("LEDGroup Example Starting...");

    // Initialize FastLED
    FastLED.addLeds<APA102, DATA_PIN, CLOCK_PIN, RGB>(leds, NUM_LEDS);
    FastLED.setBrightness(100);

    // Create LED groups for each side of the cube
    // Parameters: (physical array, physical size, group number, virtual size)
    side1 = new LEDGroup(leds, NUM_LEDS, 1, LEDS_PER_SIDE);
    side2 = new LEDGroup(leds, NUM_LEDS, 2, LEDS_PER_SIDE);
    side3 = new LEDGroup(leds, NUM_LEDS, 3, LEDS_PER_SIDE);
    side4 = new LEDGroup(leds, NUM_LEDS, 4, LEDS_PER_SIDE);

    // Configure timing for each side (for spinning cube POV)
    // Example: 2000 RPM = 30ms per rotation, ~7.5ms per side
    unsigned long framePeriod = 7500; // microseconds

    side1->setDutyCycle(0.9, framePeriod); // 90% on time
    side2->setDutyCycle(0.9, framePeriod);
    side3->setDutyCycle(0.9, framePeriod);
    side4->setDutyCycle(0.9, framePeriod);

    // Set custom effects for each side
    side1->setEffectFunction(rainbowEffect);
    side2->setEffectFunction(pulseEffect);
    side3->setEffectFunction(sparkleEffect);
    // side4 will use default color effect

    Serial.println("Setup complete!");
}

void loop() {
    // Update all groups
    updateGroup(side1);
    updateGroup(side2);
    updateGroup(side3);
    updateGroup(side4);

    // Show the LEDs
    FastLED.show();
}

void updateGroup(LEDGroup* group) {
    // Update square wave timing
    group->updateSquareWave();

    // Process state changes (handles frame transitions)
    group->processStateChanges();

    // Keep buffer filled with frames
    group->fillBuffer();

    // Map virtual LEDs to physical strip
    group->mapVirtualToPhysical();
}

/**
 * INTEGRATION NOTES:
 *
 * 1. Basic Setup:
 *    - Include LEDGroup.h and LEDGroup.cpp in your project
 *    - Create LEDGroup objects for each section you want to control
 *    - Configure timing based on your rotation speed
 *
 * 2. For Spinning Cube (POV):
 *    - Calculate frame period based on RPM
 *    - Set duty cycle to control ON/OFF ratio
 *    - Use frame buffer to pre-render animations
 *
 * 3. Custom Effects:
 *    - Create effect functions with signature: void func(CRGB*, uint16_t, uint8_t)
 *    - Use setEffectFunction() to assign them
 *    - Effects are automatically called when generating frames
 *
 * 4. Integration with Existing Code:
 *
 *    In your main .ino file:
 *
 *    #include "LEDGroup.h"
 *
 *    // Create groups in setup()
 *    LEDGroup* groups[4];
 *    for(int i = 0; i < 4; i++) {
 *        groups[i] = new LEDGroup(leds, NUM_LEDS, i+1, LEDS_PER_SIDE);
 *    }
 *
 *    // In loop(), update based on encoder/timing
 *    void loop() {
 *        // Update rotation timing based on encoder
 *        unsigned long framePeriod = calculateFramePeriod();
 *
 *        for(int i = 0; i < 4; i++) {
 *            groups[i]->setDutyCycle(0.95, framePeriod);
 *            groups[i]->updateSquareWave();
 *            groups[i]->processStateChanges();
 *            groups[i]->fillBuffer();
 *            groups[i]->mapVirtualToPhysical();
 *        }
 *
 *        FastLED.show();
 *    }
 *
 * 5. Advanced: ESP-NOW Integration
 *    - Receive pattern/timing updates via ESP-NOW
 *    - Update effect functions dynamically
 *    - Adjust duty cycle based on motor speed
 *
 *    Example:
 *    void onDataReceived(uint8_t *data, uint8_t len) {
 *        uint8_t pattern = data[0];
 *        uint8_t frameRate = data[1];
 *
 *        // Update timing
 *        unsigned long period = frameRate * 1000;
 *        for(auto group : groups) {
 *            group->setDutyCycle(0.95, period);
 *        }
 *
 *        // Change effects based on pattern
 *        switch(pattern) {
 *            case 1: side1->setEffectFunction(rainbowEffect); break;
 *            case 2: side1->setEffectFunction(pulseEffect); break;
 *            case 3: side1->setEffectFunction(sparkleEffect); break;
 *        }
 *    }
 */
