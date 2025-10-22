/**
 * ESP32 Hardware Timer Example for LED Square Wave Control
 *
 * Demonstrates using ESP32's hardware timers (4 available!) to control
 * independent LED groups with precise timing and duty cycle control.
 *
 * This example shows TWO approaches:
 * 1. Multiple hardware timers (one per LED group)
 * 2. Single timer with state machine (like ESP8266)
 *
 * Choose based on your needs!
 */

#include <FastLED.h>

// LED Configuration
#define DATA_PIN 23
#define CLOCK_PIN 18
#define NUM_LEDS 48
#define LEDS_PER_SIDE 12
#define NUM_SIDES 4

CRGB leds[NUM_LEDS];

// ===== APPROACH 1: MULTIPLE TIMERS (Best for independent control) =====

#ifdef USE_MULTIPLE_TIMERS

// Timer handles (ESP32 has 4 hardware timers)
hw_timer_t *timer0 = NULL;
hw_timer_t *timer1 = NULL;
hw_timer_t *timer2 = NULL;
hw_timer_t *timer3 = NULL;

// State for each side
volatile bool sideState[NUM_SIDES] = {false, false, false, false};

// Timing configuration per side (in microseconds)
struct SideTiming {
    uint32_t periodUs;
    float dutyCycle;
    uint32_t onTicks;
    uint32_t offTicks;
    volatile bool updatePending;
};

SideTiming sideTiming[NUM_SIDES];

// Frame buffers
CRGB sideBuffers[NUM_SIDES][LEDS_PER_SIDE];

/**
 * Timer ISRs - One per side
 */
void IRAM_ATTR timer0ISR() {
    sideState[0] = !sideState[0];

    if (sideState[0]) {
        // Display side 0
        for (int i = 0; i < LEDS_PER_SIDE; i++) {
            leds[i] = sideBuffers[0][i];
        }
        timerAlarmWrite(timer0, sideTiming[0].onTicks, true);
    } else {
        // Clear side 0
        for (int i = 0; i < LEDS_PER_SIDE; i++) {
            leds[i] = CRGB::Black;
        }
        timerAlarmWrite(timer0, sideTiming[0].offTicks, true);

        // Apply pending updates during OFF period
        if (sideTiming[0].updatePending) {
            // Updates already written by main loop
            sideTiming[0].updatePending = false;
        }
    }
}

void IRAM_ATTR timer1ISR() {
    sideState[1] = !sideState[1];
    int offset = LEDS_PER_SIDE;

    if (sideState[1]) {
        for (int i = 0; i < LEDS_PER_SIDE; i++) {
            leds[offset + i] = sideBuffers[1][i];
        }
        timerAlarmWrite(timer1, sideTiming[1].onTicks, true);
    } else {
        for (int i = 0; i < LEDS_PER_SIDE; i++) {
            leds[offset + i] = CRGB::Black;
        }
        timerAlarmWrite(timer1, sideTiming[1].offTicks, true);

        if (sideTiming[1].updatePending) {
            sideTiming[1].updatePending = false;
        }
    }
}

void IRAM_ATTR timer2ISR() {
    sideState[2] = !sideState[2];
    int offset = LEDS_PER_SIDE * 2;

    if (sideState[2]) {
        for (int i = 0; i < LEDS_PER_SIDE; i++) {
            leds[offset + i] = sideBuffers[2][i];
        }
        timerAlarmWrite(timer2, sideTiming[2].onTicks, true);
    } else {
        for (int i = 0; i < LEDS_PER_SIDE; i++) {
            leds[offset + i] = CRGB::Black;
        }
        timerAlarmWrite(timer2, sideTiming[2].offTicks, true);

        if (sideTiming[2].updatePending) {
            sideTiming[2].updatePending = false;
        }
    }
}

void IRAM_ATTR timer3ISR() {
    sideState[3] = !sideState[3];
    int offset = LEDS_PER_SIDE * 3;

    if (sideState[3]) {
        for (int i = 0; i < LEDS_PER_SIDE; i++) {
            leds[offset + i] = sideBuffers[3][i];
        }
        timerAlarmWrite(timer3, sideTiming[3].onTicks, true);
    } else {
        for (int i = 0; i < LEDS_PER_SIDE; i++) {
            leds[offset + i] = CRGB::Black;
        }
        timerAlarmWrite(timer3, sideTiming[3].offTicks, true);

        if (sideTiming[3].updatePending) {
            sideTiming[3].updatePending = false;
        }
    }
}

/**
 * Setup hardware timers
 * ESP32 timer parameters:
 * - Prescaler: 80 (80MHz / 80 = 1MHz = 1μs resolution)
 * - Count up mode
 * - Auto-reload enabled
 */
void setupTimers() {
    // Timer 0
    timer0 = timerBegin(0, 80, true);  // Timer 0, prescaler 80, count up
    timerAttachInterrupt(timer0, &timer0ISR, true);  // Attach ISR, edge mode
    timerAlarmWrite(timer0, sideTiming[0].onTicks, true);  // Set alarm, auto-reload
    timerAlarmEnable(timer0);  // Enable alarm

    // Timer 1
    timer1 = timerBegin(1, 80, true);
    timerAttachInterrupt(timer1, &timer1ISR, true);
    timerAlarmWrite(timer1, sideTiming[1].onTicks, true);
    timerAlarmEnable(timer1);

    // Timer 2
    timer2 = timerBegin(2, 80, true);
    timerAttachInterrupt(timer2, &timer2ISR, true);
    timerAlarmWrite(timer2, sideTiming[2].onTicks, true);
    timerAlarmEnable(timer2);

    // Timer 3
    timer3 = timerBegin(3, 80, true);
    timerAttachInterrupt(timer3, &timer3ISR, true);
    timerAlarmWrite(timer3, sideTiming[3].onTicks, true);
    timerAlarmEnable(timer3);
}

/**
 * Set timing for a specific side
 */
void setSideTiming(uint8_t side, uint32_t periodUs, float dutyCycle) {
    if (side >= NUM_SIDES) return;
    if (dutyCycle < 0.0) dutyCycle = 0.0;
    if (dutyCycle > 1.0) dutyCycle = 1.0;

    sideTiming[side].periodUs = periodUs;
    sideTiming[side].dutyCycle = dutyCycle;
    sideTiming[side].onTicks = periodUs * dutyCycle;
    sideTiming[side].offTicks = periodUs * (1.0 - dutyCycle);
    sideTiming[side].updatePending = true;
}

/**
 * Set timing for all sides based on RPM
 */
void setTimingFromRPM(float rpm) {
    float rotationPeriodUs = (60.0 / rpm) * 1000000.0;
    float sidePeriodUs = rotationPeriodUs / NUM_SIDES;

    for (int i = 0; i < NUM_SIDES; i++) {
        setSideTiming(i, sidePeriodUs, 0.95);
    }
}

#else  // USE_SINGLE_TIMER

// ===== APPROACH 2: SINGLE TIMER (Like ESP8266) =====

hw_timer_t *timer = NULL;

volatile uint8_t currentSide = 0;
volatile bool displayOn = false;

volatile uint32_t onTicks = 7500;
volatile uint32_t offTicks = 500;

volatile bool updatePending = false;
volatile uint32_t newOnTicks = 7500;
volatile uint32_t newOffTicks = 500;

CRGB sideBuffers[NUM_SIDES][LEDS_PER_SIDE];

void IRAM_ATTR onTimer() {
    displayOn = !displayOn;

    if (displayOn) {
        // Show current side
        int offset = currentSide * LEDS_PER_SIDE;
        for (int i = 0; i < LEDS_PER_SIDE; i++) {
            leds[offset + i] = sideBuffers[currentSide][i];
        }
        timerAlarmWrite(timer, onTicks, true);
    } else {
        // Clear current side
        int offset = currentSide * LEDS_PER_SIDE;
        for (int i = 0; i < LEDS_PER_SIDE; i++) {
            leds[offset + i] = CRGB::Black;
        }

        // Advance to next side
        currentSide = (currentSide + 1) % NUM_SIDES;

        // Update timing if pending
        if (updatePending) {
            onTicks = newOnTicks;
            offTicks = newOffTicks;
            updatePending = false;
        }

        timerAlarmWrite(timer, offTicks, true);
    }
}

void setupTimers() {
    timer = timerBegin(0, 80, true);  // Timer 0, prescaler 80, count up
    timerAttachInterrupt(timer, &onTimer, true);
    timerAlarmWrite(timer, onTicks, true);
    timerAlarmEnable(timer);
}

void setTiming(uint32_t periodUs, float dutyCycle) {
    if (dutyCycle < 0.0) dutyCycle = 0.0;
    if (dutyCycle > 1.0) dutyCycle = 1.0;

    newOnTicks = periodUs * dutyCycle;
    newOffTicks = periodUs * (1.0 - dutyCycle);
    updatePending = true;
}

void setTimingFromRPM(float rpm) {
    float rotationPeriodUs = (60.0 / rpm) * 1000000.0;
    float sidePeriodUs = rotationPeriodUs / NUM_SIDES;
    setTiming(sidePeriodUs, 0.95);
}

#endif  // USE_MULTIPLE_TIMERS

// ===== COMMON CODE =====

// Flag for updating LEDs from main loop (recommended for performance)
volatile bool needsFastLEDUpdate = false;

void IRAM_ATTR setUpdateFlag() {
    needsFastLEDUpdate = true;
}

// ===== ANIMATION FUNCTIONS =====

void rainbowAnimation() {
    static uint8_t hue = 0;

    for (int side = 0; side < NUM_SIDES; side++) {
        fill_rainbow(sideBuffers[side], LEDS_PER_SIDE, hue + (side * 60), 255 / LEDS_PER_SIDE);
    }

    hue += 2;
}

void solidColors() {
    CRGB colors[] = {CRGB::Red, CRGB::Green, CRGB::Blue, CRGB::Yellow};

    for (int side = 0; side < NUM_SIDES; side++) {
        fill_solid(sideBuffers[side], LEDS_PER_SIDE, colors[side]);
    }
}

void sparkleEffect() {
    for (int side = 0; side < NUM_SIDES; side++) {
        // Fade existing
        for (int i = 0; i < LEDS_PER_SIDE; i++) {
            sideBuffers[side][i].nscale8(200);
        }
        // Add sparkle
        if (random8() < 50) {
            sideBuffers[side][random8(LEDS_PER_SIDE)] = CRGB::White;
        }
    }
}

// ===== SETUP & LOOP =====

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\nESP32 Hardware Timer LED Control");

#ifdef USE_MULTIPLE_TIMERS
    Serial.println("Mode: Multiple Timers (4 independent timers)");

    // Initialize timing structures
    for (int i = 0; i < NUM_SIDES; i++) {
        sideTiming[i].periodUs = 8000;
        sideTiming[i].dutyCycle = 0.95;
        sideTiming[i].onTicks = 7600;
        sideTiming[i].offTicks = 400;
        sideTiming[i].updatePending = false;
    }
#else
    Serial.println("Mode: Single Timer (state machine)");
#endif

    // Initialize FastLED
    FastLED.addLeds<APA102, DATA_PIN, CLOCK_PIN, RGB>(leds, NUM_LEDS);
    FastLED.setBrightness(100);
    FastLED.clear();
    FastLED.show();

    // Initialize buffers
    for (int i = 0; i < NUM_SIDES; i++) {
        fill_solid(sideBuffers[i], LEDS_PER_SIDE, CRGB::Black);
    }

    // Setup initial animation
    solidColors();

    // Setup timers
    setupTimers();

    Serial.println("Timers started!");
}

void loop() {
    static unsigned long lastUpdate = 0;
    static unsigned long lastRpmChange = 0;
    static float testRpm = 1000.0;

    // Update animations at 60 FPS
    if (millis() - lastUpdate > 16) {
        lastUpdate = millis();

        // Choose your animation
        rainbowAnimation();
        // solidColors();
        // sparkleEffect();
    }

    // Update FastLED in main loop (recommended for performance)
    // Comment this out if you prefer to call FastLED.show() in ISR
    FastLED.show();

    // Simulate RPM changes for testing
    if (millis() - lastRpmChange > 5000) {
        lastRpmChange = millis();

        testRpm += 500;
        if (testRpm > 3000) testRpm = 500;

        setTimingFromRPM(testRpm);

        Serial.print("RPM: ");
        Serial.print(testRpm);
        Serial.print(" -> Period per side: ");
        Serial.print((60.0 / testRpm) * 1000000.0 / NUM_SIDES);
        Serial.println("μs");
    }

    // Main loop can do other work!
    yield();
}

/**
 * INTEGRATION NOTES:
 *
 * 1. Choosing Between Approaches:
 *
 *    MULTIPLE TIMERS (#define USE_MULTIPLE_TIMERS):
 *    ✅ Independent control per side
 *    ✅ Different timing per group possible
 *    ✅ More complex animations
 *    ⚠️ Uses all 4 timers
 *    ⚠️ More code/complexity
 *
 *    SINGLE TIMER (default):
 *    ✅ Simple state machine
 *    ✅ Saves 3 timers for other uses
 *    ✅ Easier to understand
 *    ⚠️ All sides share same timing
 *
 * 2. ESP-NOW Integration:
 *
 *    void onDataReceived(uint8_t *data, uint8_t len) {
 *        uint8_t frameRate = data[1];
 *        uint8_t motorSpeed = data[2];
 *
 *        // Calculate RPM from motor speed
 *        float rpm = map(motorSpeed, 0, 255, 0, 3000);
 *        setTimingFromRPM(rpm);
 *    }
 *
 * 3. Encoder Integration:
 *
 *    volatile uint32_t lastSlotTime = 0;
 *    volatile uint32_t slotPeriodUs = 0;
 *
 *    void IRAM_ATTR encoderISR() {
 *        uint32_t now = micros();
 *        slotPeriodUs = now - lastSlotTime;
 *        lastSlotTime = now;
 *    }
 *
 *    void loop() {
 *        // Calculate RPM from slot timing
 *        if(slotPeriodUs > 0) {
 *            float rpm = calculateRPM(slotPeriodUs);
 *            setTimingFromRPM(rpm);
 *        }
 *    }
 *
 * 4. Performance Optimization:
 *
 *    - Move FastLED.show() to main loop (already done above)
 *    - Use DMA for SPI LEDs (APA102 supports this)
 *    - Reduce buffer copies with pointer swapping
 *    - Use IRAM_ATTR on all ISR functions
 *
 * 5. Advanced: Phase Offset Between Sides:
 *
 *    // Start timers with phase offset for smoother overall display
 *    void setupTimersWithPhase() {
 *        // Timer 0 starts immediately
 *        setupTimer0();
 *
 *        // Timer 1 starts 1/4 phase offset
 *        delay(sidePeriod / 4 / 1000);
 *        setupTimer1();
 *
 *        // Timer 2 starts 1/2 phase offset
 *        delay(sidePeriod / 4 / 1000);
 *        setupTimer2();
 *
 *        // Timer 3 starts 3/4 phase offset
 *        delay(sidePeriod / 4 / 1000);
 *        setupTimer3();
 *    }
 */
