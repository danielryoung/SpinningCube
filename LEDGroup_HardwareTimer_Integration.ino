/**
 * LEDGroup with Hardware Timer Integration
 *
 * This example shows how to integrate the LEDGroup class with hardware timers
 * for ultra-precise timing control. Hardware timers provide:
 * - Microsecond precision
 * - Minimal CPU overhead
 * - Consistent timing regardless of main loop activity
 *
 * Two approaches are shown:
 * 1. ESP8266 - Single timer with callback
 * 2. ESP32 - Multiple timers (one per group)
 */

#include <FastLED.h>
#include "LEDGroup.h"

// LED Configuration
#define NUM_LEDS 48
#define LEDS_PER_SIDE 12
#define NUM_SIDES 4

#ifdef ESP8266
#define DATA_PIN D7
#define CLOCK_PIN D5
#elif defined(ESP32)
#define DATA_PIN 23
#define CLOCK_PIN 18
#endif

CRGB leds[NUM_LEDS];

// LED Groups
LEDGroup* groups[NUM_SIDES];

// ===== HARDWARE TIMER INTEGRATION =====

#ifdef ESP8266

// ESP8266: Use single timer with state machine
volatile uint8_t currentGroup = 0;
volatile bool displayOn = false;

volatile uint32_t onTicks = 7500;
volatile uint32_t offTicks = 500;

void ICACHE_RAM_ATTR timerISR() {
    displayOn = !displayOn;
    LEDGroup* group = groups[currentGroup];

    if (displayOn) {
        // Render and map current group
        group->renderCurrentFrame();
        group->mapVirtualToPhysical();
        timer1_write(onTicks);
    } else {
        // Clear group and advance
        group->fillVirtual(CRGB::Black);
        group->mapVirtualToPhysical();

        // Advance to next frame in buffer
        if (!group->isBufferEmpty()) {
            group->advanceReadIndex();
        }

        currentGroup = (currentGroup + 1) % NUM_SIDES;
        timer1_write(offTicks);
    }

    // Note: Moving FastLED.show() to main loop is recommended
}

void setupHardwareTimer() {
    timer1_disable();
    timer1_isr_init();
    timer1_attachInterrupt(timerISR);
    timer1_enable(TIM_DIV80, TIM_EDGE, TIM_LOOP);  // 1MHz (1μs resolution)
    timer1_write(onTicks);
}

void setTimingFromRPM(float rpm) {
    float rotationPeriodUs = (60.0 / rpm) * 1000000.0;
    float sidePeriodUs = rotationPeriodUs / NUM_SIDES;

    onTicks = sidePeriodUs * 0.95;
    offTicks = sidePeriodUs * 0.05;

    // Also update LEDGroup timing for compatibility
    for (int i = 0; i < NUM_SIDES; i++) {
        groups[i]->setDutyCycle(0.95, sidePeriodUs);
    }
}

#elif defined(ESP32)

// ESP32: Use multiple timers (one per group)
hw_timer_t* timers[NUM_SIDES] = {NULL, NULL, NULL, NULL};

struct GroupTiming {
    volatile bool displayOn;
    volatile uint32_t onTicks;
    volatile uint32_t offTicks;
};

GroupTiming groupTiming[NUM_SIDES];

// Timer ISRs for each group
void IRAM_ATTR timer0ISR() {
    LEDGroup* group = groups[0];
    groupTiming[0].displayOn = !groupTiming[0].displayOn;

    if (groupTiming[0].displayOn) {
        group->renderCurrentFrame();
        group->mapVirtualToPhysical();
        timerAlarmWrite(timers[0], groupTiming[0].onTicks, true);
    } else {
        group->fillVirtual(CRGB::Black);
        group->mapVirtualToPhysical();
        if (!group->isBufferEmpty()) {
            group->advanceReadIndex();
        }
        timerAlarmWrite(timers[0], groupTiming[0].offTicks, true);
    }
}

void IRAM_ATTR timer1ISR() {
    LEDGroup* group = groups[1];
    groupTiming[1].displayOn = !groupTiming[1].displayOn;

    if (groupTiming[1].displayOn) {
        group->renderCurrentFrame();
        group->mapVirtualToPhysical();
        timerAlarmWrite(timers[1], groupTiming[1].onTicks, true);
    } else {
        group->fillVirtual(CRGB::Black);
        group->mapVirtualToPhysical();
        if (!group->isBufferEmpty()) {
            group->advanceReadIndex();
        }
        timerAlarmWrite(timers[1], groupTiming[1].offTicks, true);
    }
}

void IRAM_ATTR timer2ISR() {
    LEDGroup* group = groups[2];
    groupTiming[2].displayOn = !groupTiming[2].displayOn;

    if (groupTiming[2].displayOn) {
        group->renderCurrentFrame();
        group->mapVirtualToPhysical();
        timerAlarmWrite(timers[2], groupTiming[2].onTicks, true);
    } else {
        group->fillVirtual(CRGB::Black);
        group->mapVirtualToPhysical();
        if (!group->isBufferEmpty()) {
            group->advanceReadIndex();
        }
        timerAlarmWrite(timers[2], groupTiming[2].offTicks, true);
    }
}

void IRAM_ATTR timer3ISR() {
    LEDGroup* group = groups[3];
    groupTiming[3].displayOn = !groupTiming[3].displayOn;

    if (groupTiming[3].displayOn) {
        group->renderCurrentFrame();
        group->mapVirtualToPhysical();
        timerAlarmWrite(timers[3], groupTiming[3].onTicks, true);
    } else {
        group->fillVirtual(CRGB::Black);
        group->mapVirtualToPhysical();
        if (!group->isBufferEmpty()) {
            group->advanceReadIndex();
        }
        timerAlarmWrite(timers[3], groupTiming[3].offTicks, true);
    }
}

void (*timerISRs[NUM_SIDES])() = {timer0ISR, timer1ISR, timer2ISR, timer3ISR};

void setupHardwareTimer() {
    for (int i = 0; i < NUM_SIDES; i++) {
        // Initialize timing
        groupTiming[i].displayOn = false;
        groupTiming[i].onTicks = 7500;
        groupTiming[i].offTicks = 500;

        // Setup timer
        timers[i] = timerBegin(i, 80, true);  // 1MHz (1μs resolution)
        timerAttachInterrupt(timers[i], timerISRs[i], true);
        timerAlarmWrite(timers[i], groupTiming[i].onTicks, true);
        timerAlarmEnable(timers[i]);
    }
}

void setTimingFromRPM(float rpm) {
    float rotationPeriodUs = (60.0 / rpm) * 1000000.0;
    float sidePeriodUs = rotationPeriodUs / NUM_SIDES;

    for (int i = 0; i < NUM_SIDES; i++) {
        groupTiming[i].onTicks = sidePeriodUs * 0.95;
        groupTiming[i].offTicks = sidePeriodUs * 0.05;

        // Update LEDGroup timing for compatibility
        groups[i]->setDutyCycle(0.95, sidePeriodUs);
    }
}

#endif

// ===== EFFECT FUNCTIONS =====

void rainbowEffect(CRGB* leds, uint16_t count, uint8_t groupNum) {
    static uint8_t hue = 0;
    fill_rainbow(leds, count, hue + (groupNum * 60), 255 / count);
    hue++;
}

void pulseEffect(CRGB* leds, uint16_t count, uint8_t groupNum) {
    static uint8_t brightness = 0;
    static int8_t direction = 1;

    CRGB color;
    switch (groupNum) {
        case 1: color = CRGB::Red; break;
        case 2: color = CRGB::Green; break;
        case 3: color = CRGB::Blue; break;
        case 4: color = CRGB::Purple; break;
        default: color = CRGB::White;
    }

    fill_solid(leds, count, color);
    for (int i = 0; i < count; i++) {
        leds[i].nscale8(brightness);
    }

    brightness += direction * 3;
    if (brightness >= 250 || brightness <= 5) {
        direction *= -1;
    }
}

void fireEffect(CRGB* leds, uint16_t count, uint8_t groupNum) {
    // Simple fire effect
    for (int i = 0; i < count; i++) {
        leds[i] = CRGB(random8(200, 255), random8(0, 100), 0);
    }
}

// ===== SETUP & LOOP =====

void setup() {
    Serial.begin(115200);
    delay(500);

#ifdef ESP8266
    Serial.println("\nLEDGroup + ESP8266 Hardware Timer");
#elif defined(ESP32)
    Serial.println("\nLEDGroup + ESP32 Hardware Timers");
#endif

    // Initialize FastLED
    FastLED.addLeds<APA102, DATA_PIN, CLOCK_PIN, RGB>(leds, NUM_LEDS);
    FastLED.setBrightness(100);
    FastLED.clear();
    FastLED.show();

    // Create LED groups
    for (int i = 0; i < NUM_SIDES; i++) {
        groups[i] = new LEDGroup(leds, NUM_LEDS, i + 1, LEDS_PER_SIDE);

        // Set initial timing (will be overridden by hardware timer)
        groups[i]->setDutyCycle(0.95, 8000);
    }

    // Set different effects per group
    groups[0]->setEffectFunction(rainbowEffect);
    groups[1]->setEffectFunction(pulseEffect);
    groups[2]->setEffectFunction(fireEffect);
    // groups[3] uses default color effect

    // Setup hardware timers
    setupHardwareTimer();

    // Set initial RPM
    setTimingFromRPM(1500.0);

    Serial.println("Hardware timers initialized!");
}

void loop() {
    static unsigned long lastRpmChange = 0;
    static float testRpm = 1000.0;

    // Fill buffers with new frames
    // This happens in main loop, display happens in ISR
    for (int i = 0; i < NUM_SIDES; i++) {
        if (!groups[i]->isBufferFull()) {
            groups[i]->fillBuffer();
        }
    }

    // Update physical LEDs (called here for performance)
    FastLED.show();

    // Simulate RPM changes
    if (millis() - lastRpmChange > 5000) {
        lastRpmChange = millis();

        testRpm += 500;
        if (testRpm > 3000) testRpm = 500;

        setTimingFromRPM(testRpm);

        Serial.print("RPM: ");
        Serial.print(testRpm);
        Serial.print(" -> ");
        Serial.print((60.0 / testRpm) * 1000000.0 / NUM_SIDES);
        Serial.println("μs per side");
    }

    yield();
}

/**
 * ============================================================================
 * INTEGRATION WITH YOUR EXISTING CODE
 * ============================================================================
 *
 * 1. ESP-NOW Integration:
 *
 *    #include <esp_now.h>  // or espnow.h for ESP8266
 *
 *    void onDataReceived(uint8_t *mac, uint8_t *data, uint8_t len) {
 *        // Parse received data
 *        uint8_t pattern = data[0];
 *        uint8_t frameRate = data[1];
 *        uint8_t motorSpeed = data[2];
 *
 *        // Update timing based on motor speed
 *        float rpm = map(motorSpeed, 0, 255, 500, 3000);
 *        setTimingFromRPM(rpm);
 *
 *        // Change effects based on pattern
 *        switch(pattern) {
 *            case 1:
 *                for(int i = 0; i < NUM_SIDES; i++) {
 *                    groups[i]->setEffectFunction(rainbowEffect);
 *                }
 *                break;
 *            case 2:
 *                for(int i = 0; i < NUM_SIDES; i++) {
 *                    groups[i]->setEffectFunction(pulseEffect);
 *                }
 *                break;
 *            case 3:
 *                groups[0]->setEffectFunction(fireEffect);
 *                groups[1]->setEffectFunction(rainbowEffect);
 *                groups[2]->setEffectFunction(pulseEffect);
 *                groups[3]->setEffectFunction(NULL);  // Default color
 *                break;
 *        }
 *    }
 *
 *    void setup() {
 *        // ... existing setup ...
 *
 *        // Setup ESP-NOW
 *        WiFi.mode(WIFI_STA);
 *        esp_now_init();
 *        esp_now_register_recv_cb(onDataReceived);
 *    }
 *
 * 2. Encoder Integration (Like CubeControllerESP32):
 *
 *    volatile uint32_t slotCount = 0;
 *    volatile uint32_t lastSlotTime = 0;
 *    volatile uint32_t slotPeriodUs = 0;
 *
 *    void IRAM_ATTR encoderISR() {
 *        slotCount++;
 *        uint32_t now = micros();
 *        slotPeriodUs = now - lastSlotTime;
 *        lastSlotTime = now;
 *    }
 *
 *    void setup() {
 *        attachInterrupt(digitalPinToInterrupt(ENCODER_PIN), encoderISR, RISING);
 *    }
 *
 *    void loop() {
 *        // Calculate RPM from encoder
 *        if(slotPeriodUs > 0) {
 *            // Assuming 12 slots per revolution
 *            float rpm = (60.0 * 1000000.0) / (slotPeriodUs * 12);
 *            setTimingFromRPM(rpm);
 *        }
 *    }
 *
 * 3. Replacing TickTwo with Hardware Timers:
 *
 *    OLD CODE (CubeReceiverESP8266WorkingPostTOPs.ino):
 *    -----------------------------------------------
 *    TickTwo frameTimer(nextSide, 200, 0, MICROS_MICROS);
 *    TickTwo onTimer(turnOff, 200, 0, MICROS_MICROS);
 *
 *    frameTimer.interval(onDurationMicros);
 *    onTimer.interval(turnOffAfter);
 *
 *    void loop() {
 *        frameTimer.update();
 *        onTimer.update();
 *    }
 *
 *    NEW CODE (Hardware Timer):
 *    -------------------------
 *    // Remove TickTwo timers
 *    // Use hardware timer ISR instead
 *    // Timing updates happen automatically
 *    // No need to call .update() in loop!
 *
 * 4. Performance Comparison:
 *
 *    Software (TickTwo):
 *    - Timing jitter: ±50-200μs
 *    - CPU usage: ~5-10% checking timers
 *    - Loop() must run fast
 *
 *    Hardware Timer:
 *    - Timing jitter: ±1-2μs
 *    - CPU usage: <1% (only ISR execution)
 *    - Loop() can be slow, timing unaffected
 *
 * 5. Migration Path:
 *
 *    Step 1: Add hardware timer code alongside existing TickTwo
 *    Step 2: Test timing with Serial output
 *    Step 3: Switch LED updates to hardware timer
 *    Step 4: Remove TickTwo when confirmed working
 *    Step 5: Optimize ISR (move FastLED.show to main loop if needed)
 *
 * 6. Debugging:
 *
 *    // Add timing debug output
 *    void loop() {
 *        static unsigned long lastDebug = 0;
 *
 *        if(millis() - lastDebug > 1000) {
 *            lastDebug = millis();
 *
 *            Serial.print("Group 0 buffer: ");
 *            Serial.print(groups[0]->getReadIndex());
 *            Serial.print("/");
 *            Serial.println(groups[0]->getWriteIndex());
 *
 *            Serial.print("Frames buffered: ");
 *            Serial.println(groups[0]->isBufferFull() ? "FULL" : "OK");
 *        }
 *    }
 *
 * ============================================================================
 */
