#ifndef LEDGROUP_H
#define LEDGROUP_H

#include <FastLED.h>

// Frame buffer size for smooth animations
#define FRAME_BUFFER_SIZE 4

/**
 * LEDGroup - Manages a group of LEDs with virtual buffering and timing control
 *
 * This class allows you to:
 * - Map virtual LED arrays to physical LED strips
 * - Buffer multiple frames for smooth animations
 * - Control display timing with square wave patterns
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
     * Destructor - cleans up allocated memory
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
     * Updates the square wave timing state
     * Call this frequently (e.g., in loop())
     */
    void updateSquareWave();

    /**
     * Processes state changes and manages frame transitions
     * Call this after updateSquareWave()
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
    unsigned long onDuration;
    unsigned long offDuration;
    bool squareWaveState;
    unsigned long lastSquareWaveUpdate;

    // Effect function pointer
    void (*effectFunction)(CRGB*, uint16_t, uint8_t);
};

#endif // LEDGROUP_H
