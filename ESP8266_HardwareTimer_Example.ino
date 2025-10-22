/**
 * ESP8266 Hardware Timer Example for LED Square Wave Control
 *
 * Demonstrates using Timer1 with a state machine to control
 * multiple LED groups with precise timing and duty cycle control.
 *
 * Perfect for: Spinning LED cubes with POV effects
 */

#include <FastLED.h>

// LED Configuration
#define DATA_PIN D7
#define CLOCK_PIN D5
#define NUM_LEDS 48
#define LEDS_PER_SIDE 12
#define NUM_SIDES 4

CRGB leds[NUM_LEDS];

// Timer configuration
#define TIMER_PRESCALER 80  // 80MHz / 80 = 1MHz (1μs per tick)

// State machine for cycling through sides
volatile uint8_t currentSide = 0;
volatile bool displayOn = false;

// Timing values (in microseconds, converted to timer ticks)
volatile uint32_t onTicks = 7500;   // Default: 7.5ms on
volatile uint32_t offTicks = 500;   // Default: 0.5ms off

// Double buffering for safe updates from main loop
volatile bool updatePending = false;
volatile uint32_t newOnTicks = 7500;
volatile uint32_t newOffTicks = 500;

// Frame buffer for each side
CRGB sideBuffers[NUM_SIDES][LEDS_PER_SIDE];

/**
 * Convert microseconds to timer ticks
 * With prescaler=80: 80MHz / 80 = 1MHz = 1 tick per microsecond
 */
inline uint32_t usToTicks(uint32_t microseconds) {
    return microseconds; // 1:1 ratio with prescaler 80
}

/**
 * Timer1 ISR - Handles LED timing state machine
 * CRITICAL: Keep this FAST! No Serial.print, no delays
 */
void ICACHE_RAM_ATTR onTimer() {
    // Toggle display state
    displayOn = !displayOn;

    if (displayOn) {
        // TURNING ON - Show current side
        mapSideToStrip(currentSide);
        timer1_write(onTicks);
    } else {
        // TURNING OFF - Clear display and advance to next side
        FastLED.clear();
        currentSide = (currentSide + 1) % NUM_SIDES;

        // Check for timing updates (do this during OFF to avoid glitches)
        if (updatePending) {
            onTicks = newOnTicks;
            offTicks = newOffTicks;
            updatePending = false;
        }

        timer1_write(offTicks);
    }

    // Update physical LEDs (FastLED.show() can be called from ISR if needed)
    // Note: This adds ~50-100μs latency. For best performance, use
    // direct port manipulation or call from main loop with DMA
    FastLED.show();
}

/**
 * Map a virtual side buffer to the physical LED strip
 */
void ICACHE_RAM_ATTR mapSideToStrip(uint8_t side) {
    for (int i = 0; i < LEDS_PER_SIDE; i++) {
        int physicalIndex = (i + (side * LEDS_PER_SIDE)) % NUM_LEDS;
        leds[physicalIndex] = sideBuffers[side][i];
    }
}

/**
 * Initialize Timer1
 */
void setupTimer() {
    // Disable timer
    timer1_disable();

    // Set prescaler to 80 (80MHz / 80 = 1MHz = 1μs resolution)
    timer1_isr_init();
    timer1_attachInterrupt(onTimer);
    timer1_enable(TIM_DIV80, TIM_EDGE, TIM_LOOP);

    // Set initial interval
    timer1_write(onTicks);
}

/**
 * Safely update timing from main loop
 * Uses double buffering to avoid race conditions
 */
void setTiming(uint32_t periodUs, float dutyCycle) {
    if (dutyCycle < 0.0) dutyCycle = 0.0;
    if (dutyCycle > 1.0) dutyCycle = 1.0;

    newOnTicks = usToTicks(periodUs * dutyCycle);
    newOffTicks = usToTicks(periodUs * (1.0 - dutyCycle));

    // Signal ISR to update (will happen during next OFF period)
    updatePending = true;
}

/**
 * Calculate timing based on RPM
 */
void setTimingFromRPM(float rpm) {
    // Period for one full rotation in microseconds
    float rotationPeriodUs = (60.0 / rpm) * 1000000.0;

    // Each of 4 sides gets 1/4 of rotation
    float sidePeriodUs = rotationPeriodUs / NUM_SIDES;

    // Use 95% duty cycle (5% blank for separation)
    setTiming(sidePeriodUs, 0.95);
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

void chaseEffect() {
    static uint8_t position = 0;

    for (int side = 0; side < NUM_SIDES; side++) {
        fill_solid(sideBuffers[side], LEDS_PER_SIDE, CRGB::Black);
        sideBuffers[side][(position + side * 3) % LEDS_PER_SIDE] = CRGB::White;
    }

    position++;
}

// ===== SETUP & LOOP =====

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\nESP8266 Hardware Timer LED Control");

    // Initialize FastLED
    FastLED.addLeds<APA102, DATA_PIN, CLOCK_PIN, RGB>(leds, NUM_LEDS);
    FastLED.setBrightness(100);
    FastLED.clear();
    FastLED.show();

    // Initialize side buffers
    for (int i = 0; i < NUM_SIDES; i++) {
        fill_solid(sideBuffers[i], LEDS_PER_SIDE, CRGB::Black);
    }

    // Setup initial animation
    solidColors();

    // Start with default timing: 8ms per side @ 95% duty
    setTiming(8000, 0.95);

    // Initialize and start timer
    setupTimer();

    Serial.println("Timer started!");
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
        // chaseEffect();
    }

    // Simulate RPM changes for testing
    if (millis() - lastRpmChange > 5000) {
        lastRpmChange = millis();

        // Cycle through different RPMs
        testRpm += 500;
        if (testRpm > 3000) testRpm = 500;

        setTimingFromRPM(testRpm);

        Serial.print("RPM: ");
        Serial.print(testRpm);
        Serial.print(" -> Period per side: ");
        Serial.print((60.0 / testRpm) * 1000000.0 / NUM_SIDES);
        Serial.println("μs");
    }

    // Main loop can do other work - timer runs independently!
    // Handle ESP-NOW, update displays, process inputs, etc.

    yield(); // Keep WiFi stack happy
}

/**
 * INTEGRATION NOTES:
 *
 * 1. ESP-NOW Integration:
 *    - Receive timing updates via ESP-NOW
 *    - Call setTiming() or setTimingFromRPM() with new values
 *    - Updates happen safely during OFF period
 *
 * 2. Encoder Integration:
 *    - Read actual RPM from encoder
 *    - Update timing dynamically based on real rotation speed
 *    - Example: attachInterrupt(ENCODER_PIN, encoderISR, RISING);
 *
 * 3. Performance:
 *    - Timer ISR takes ~50-100μs (including FastLED.show)
 *    - For better performance, move FastLED.show() to main loop
 *    - Use flag in ISR, update in loop
 *
 * 4. Optimization:
 *    - To avoid FastLED.show() in ISR:
 *
 *      volatile bool needsUpdate = false;
 *
 *      void ICACHE_RAM_ATTR onTimer() {
 *          // Toggle state and update buffers
 *          displayOn = !displayOn;
 *          if(displayOn) mapSideToStrip(currentSide);
 *          else currentSide = (currentSide + 1) % NUM_SIDES;
 *
 *          needsUpdate = true;  // Signal main loop
 *          timer1_write(displayOn ? onTicks : offTicks);
 *      }
 *
 *      void loop() {
 *          if(needsUpdate) {
 *              FastLED.show();
 *              needsUpdate = false;
 *          }
 *      }
 *
 * 5. Troubleshooting:
 *    - If LEDs flicker: Increase OFF time (lower duty cycle)
 *    - If timing seems off: Check prescaler calculation
 *    - If ESP crashes: Make sure ISR is ICACHE_RAM_ATTR
 *    - If updates lag: Remove Serial.print from fast paths
 */
