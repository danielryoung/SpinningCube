#ifndef LEDGROUP_H
#define LEDGROUP_H

#include <FastLED.h>

#ifdef ESP32
#include <esp_timer.h>
// ESP32 has 4 hardware timers available
#endif

// Frame buffer size for smooth animations
#define FRAME_BUFFER_SIZE 4

/**
 * LEDGroup - Manages a group of LEDs with virtual buffering and timing control
 *
 * ESP32 HARDWARE TIMER SUPPORT:
 * - Automatically uses hardware timers on ESP32 when enabled
 * - 4 timers available (can support 4 independent groups)
 * - Microsecond precision with minimal CPU overhead
 * - Falls back to software timing if hardware timers unavailable
 *
 * This class allows you to:
 * - Map virtual LED arrays to physical LED strips
 * - Buffer multiple frames for smooth animations
 * - Control display timing with hardware or software square waves
 * - Apply custom effects to LED groups
 * - Manage multiple independent LED groups
 */
class LEDGroup {
public:
    /**
     * Constructor
     * @param physicalArray Pointer to the physical LED array (shared)
     * @param physicalArraySize Total size of physical LED array
     * @param group Group number (used for positioning and default colors)
     * @param virtualSectionSize Number of LEDs in this virtual section
     */
    LEDGroup(CRGB* physicalArray, uint16_t physicalArraySize, uint8_t group, uint16_t virtualSectionSize);

    /**
     * Destructor - cleans up allocated memory and timers
     */
    ~LEDGroup();

    // Core display functions
    /**
     * Maps the virtual LED array to the physical LED strip
     * Call this after updating virtual LEDs to show changes
     */
    void mapVirtualToPhysical();

    /**
     * Renders the current frame from buffer to virtual LEDs
     */
    void renderCurrentFrame();

    /**
     * Updates the square wave timing state (software mode)
     * Call this frequently (e.g., in loop()) if not using hardware timer
     */
    void updateSquareWave();

    /**
     * Processes state changes and manages frame transitions
     * Call this after updateSquareWave() or use with hardware timer
     */
    void processStateChanges();

    /**
     * Fills the frame buffer with new frames
     * Call this to keep buffer full of content
     */
    void fillBuffer();

    /**
     * Generates the next frame using the effect function
     */
    void generateNextFrame();

    // Hardware timer control (ESP32 only)
#ifdef ESP32
    /**
     * Enable hardware timer for this group
     * @param timerNumber Timer to use (0-3 for ESP32)
     * @return true if successful, false if timer unavailable
     */
    bool enableHardwareTimer(uint8_t timerNumber);

    /**
     * Start the hardware timer
     * Must call enableHardwareTimer() first
     */
    void startHardwareTimer();

    /**
     * Stop the hardware timer
     */
    void stopHardwareTimer();

    /**
     * Check if hardware timer is enabled
     */
    bool isHardwareTimerEnabled();

    /**
     * Hardware timer ISR callback (internal use)
     * This is called automatically by the timer
     */
    void timerCallback();
#endif

    // Ring buffer management
    void advanceReadIndex();
    void advanceWriteIndex();
    CRGB* getWriteFrame();
    CRGB* getReadFrame();
    bool isBufferFull();
    bool isBufferEmpty();

    // Effect management
    /**
     * Set a custom effect function
     * @param func Function pointer: void func(CRGB* leds, uint16_t count, uint8_t groupNum)
     */
    void setEffectFunction(void (*func)(CRGB*, uint16_t, uint8_t));

    /**
     * Apply default color effect to virtual LEDs
     */
    void defaultColorEffect();

    /**
     * Apply default color effect to target array
     * @param targetArray Array to fill with default color
     */
    void defaultColorEffect(CRGB* targetArray);

    /**
     * Apply the current effect function
     */
    void applyEffect();

    // Square wave timing control
    /**
     * Set the ON duration in microseconds
     */
    void setOnDuration(unsigned long onTime);

    /**
     * Set the OFF duration in microseconds
     */
    void setOffDuration(unsigned long offTime);

    /**
     * Set duty cycle as percentage
     * @param percentage 0.0 to 1.0 (0% to 100%)
     * @param totalPeriod Total period in microseconds
     */
    void setDutyCycle(float percentage, unsigned long totalPeriod);

    /**
     * Get current square wave state
     * @return true if ON, false if OFF
     */
    bool getSquareWaveState();

    // Utility functions
    /**
     * Set all LEDs in group to a specific color
     */
    void setGroupColor(CRGB color);

    /**
     * Fill virtual LED array with a color
     */
    void fillVirtual(CRGB color);

    /**
     * Get the default color for this group
     * @return CRGB color based on group number
     */
    CRGB getDefaultGroupColor();

    // Getters
    uint8_t getGroupNumber() const;
    uint16_t getVirtualSize() const;
    uint8_t getReadIndex() const;
    uint8_t getWriteIndex() const;
    CRGB* getVirtualLEDs();

private:
    // Physical LED array reference
    CRGB* physicalLEDs;
    uint16_t physicalSize;

    // Virtual LED array for this group
    CRGB* virtualLEDs;
    uint16_t virtualSize;

    // Group identifier
    uint8_t groupNumber;

    // Frame buffer for smooth animations
    CRGB* frameBuffer[FRAME_BUFFER_SIZE];
    uint8_t readIndex;
    uint8_t writeIndex;
    uint8_t bufferedFrames;

    // Square wave timing
    volatile unsigned long onDuration;
    volatile unsigned long offDuration;
    volatile bool squareWaveState;
    unsigned long lastSquareWaveUpdate;

    // Effect function pointer
    void (*effectFunction)(CRGB*, uint16_t, uint8_t);

#ifdef ESP32
    // Hardware timer support (ESP32)
    hw_timer_t* hwTimer;
    uint8_t timerNum;
    bool hardwareTimerEnabled;
    volatile bool timingUpdatePending;
    volatile unsigned long newOnDuration;
    volatile unsigned long newOffDuration;
#endif
};

#endif // LEDGROUP_H
