#include "LEDGroup.h"

// Static array to track which timers are in use (ESP32 only)
#ifdef ESP32
static bool timerInUse[4] = {false, false, false, false};

// Static wrapper functions for timer ISRs
// These are needed because C function pointers can't directly call member functions
static LEDGroup* timerGroups[4] = {nullptr, nullptr, nullptr, nullptr};

void IRAM_ATTR timer0Wrapper() {
    if (timerGroups[0]) timerGroups[0]->timerCallback();
}

void IRAM_ATTR timer1Wrapper() {
    if (timerGroups[1]) timerGroups[1]->timerCallback();
}

void IRAM_ATTR timer2Wrapper() {
    if (timerGroups[2]) timerGroups[2]->timerCallback();
}

void IRAM_ATTR timer3Wrapper() {
    if (timerGroups[3]) timerGroups[3]->timerCallback();
}

void (*timerWrappers[4])() = {timer0Wrapper, timer1Wrapper, timer2Wrapper, timer3Wrapper};
#endif

// Constructor
LEDGroup::LEDGroup(CRGB* physicalArray, uint16_t physicalArraySize, uint8_t group, uint16_t virtualSectionSize) {
    // Store references
    physicalLEDs = physicalArray;
    physicalSize = physicalArraySize;
    groupNumber = group;
    virtualSize = virtualSectionSize;

    // Allocate virtual LED array
    virtualLEDs = new CRGB[virtualSize];

    // Initialize virtual LEDs to black
    for(int i = 0; i < virtualSize; i++) {
        virtualLEDs[i] = CRGB::Black;
    }

    // Allocate frame buffer
    for(int i = 0; i < FRAME_BUFFER_SIZE; i++) {
        frameBuffer[i] = new CRGB[virtualSize];
        // Initialize frames to black
        for(int j = 0; j < virtualSize; j++) {
            frameBuffer[i][j] = CRGB::Black;
        }
    }

    // Initialize ring buffer indices
    readIndex = 0;
    writeIndex = 0;
    bufferedFrames = 0;

    // Initialize square wave properties
    onDuration = 500000;  // Default 500ms on
    offDuration = 500000; // Default 500ms off
    squareWaveState = false;
    lastSquareWaveUpdate = micros();

    // Initialize effect function to default
    effectFunction = nullptr;

#ifdef ESP32
    // Initialize hardware timer support
    hwTimer = nullptr;
    timerNum = 255;  // Invalid timer number
    hardwareTimerEnabled = false;
    timingUpdatePending = false;
    newOnDuration = onDuration;
    newOffDuration = offDuration;
#endif
}

// Destructor
LEDGroup::~LEDGroup() {
#ifdef ESP32
    // Stop and free hardware timer if enabled
    if (hardwareTimerEnabled && hwTimer != nullptr) {
        timerAlarmDisable(hwTimer);
        timerEnd(hwTimer);
        timerInUse[timerNum] = false;
        timerGroups[timerNum] = nullptr;
    }
#endif

    delete[] virtualLEDs;
    for(int i = 0; i < FRAME_BUFFER_SIZE; i++) {
        delete[] frameBuffer[i];
    }
}

// Core functions
void LEDGroup::mapVirtualToPhysical() {
    // Calculate starting position in physical array based on group number
    // This is a simple mapping - adjust based on your physical layout
    uint16_t startPos = (groupNumber - 1) * virtualSize;

    // Ensure we don't exceed physical array bounds
    if(startPos + virtualSize <= physicalSize) {
        memcpy(&physicalLEDs[startPos], virtualLEDs, virtualSize * sizeof(CRGB));
    }
}

void LEDGroup::renderCurrentFrame() {
    if(!isBufferEmpty()) {
        memcpy(virtualLEDs, getReadFrame(), virtualSize * sizeof(CRGB));
    }
}

void LEDGroup::updateSquareWave() {
    // Software timing mode (used when hardware timer not enabled)
    unsigned long currentTime = micros();
    unsigned long targetDuration = squareWaveState ? onDuration : offDuration;

    if(currentTime - lastSquareWaveUpdate >= targetDuration) {
        squareWaveState = !squareWaveState;
        lastSquareWaveUpdate = currentTime;
    }
}

void LEDGroup::processStateChanges() {
    static bool lastState = false;
    bool currentState = squareWaveState;

    // When transitioning to ON - render current frame
    if(currentState && !lastState) {
        renderCurrentFrame();
    }

    // When transitioning to OFF - advance to next frame and clear virtual LEDs
    if(!currentState && lastState) {
        if(!isBufferEmpty()) {
            advanceReadIndex();
        }
        // Clear virtual LEDs when OFF
        fillVirtual(CRGB::Black);
    }

    lastState = currentState;
}

void LEDGroup::fillBuffer() {
    // Only generate if buffer isn't full
    if(!isBufferFull()) {
        generateNextFrame();
        advanceWriteIndex();
    }
}

void LEDGroup::generateNextFrame() {
    CRGB* nextFrame = getWriteFrame();

    if(effectFunction != nullptr) {
        effectFunction(nextFrame, virtualSize, groupNumber);
    } else {
        defaultColorEffect(nextFrame);
    }
}

#ifdef ESP32
// Hardware timer support (ESP32 only)
bool LEDGroup::enableHardwareTimer(uint8_t timerNumber) {
    if (timerNumber >= 4) {
        return false;  // Invalid timer number
    }

    if (timerInUse[timerNumber]) {
        return false;  // Timer already in use
    }

    // Initialize hardware timer
    hwTimer = timerBegin(timerNumber, 80, true);  // 80MHz / 80 = 1MHz (1μs resolution)
    if (hwTimer == nullptr) {
        return false;
    }

    timerNum = timerNumber;
    timerInUse[timerNumber] = true;
    timerGroups[timerNumber] = this;

    // Attach interrupt
    timerAttachInterrupt(hwTimer, timerWrappers[timerNumber], true);

    // Set initial alarm value (will be updated when started)
    timerAlarmWrite(hwTimer, onDuration, true);

    hardwareTimerEnabled = true;
    return true;
}

void LEDGroup::startHardwareTimer() {
    if (hardwareTimerEnabled && hwTimer != nullptr) {
        squareWaveState = false;  // Start in OFF state
        timerAlarmWrite(hwTimer, onDuration, true);
        timerAlarmEnable(hwTimer);
    }
}

void LEDGroup::stopHardwareTimer() {
    if (hardwareTimerEnabled && hwTimer != nullptr) {
        timerAlarmDisable(hwTimer);
    }
}

bool LEDGroup::isHardwareTimerEnabled() {
    return hardwareTimerEnabled;
}

void IRAM_ATTR LEDGroup::timerCallback() {
    // Toggle state
    squareWaveState = !squareWaveState;

    if (squareWaveState) {
        // Transitioning to ON - render current frame
        if (!isBufferEmpty()) {
            memcpy(virtualLEDs, getReadFrame(), virtualSize * sizeof(CRGB));
        }
        mapVirtualToPhysical();

        // Set alarm for ON duration
        timerAlarmWrite(hwTimer, onDuration, true);
    } else {
        // Transitioning to OFF - advance to next frame and clear
        if (!isBufferEmpty()) {
            readIndex = (readIndex + 1) % FRAME_BUFFER_SIZE;
            bufferedFrames--;
        }

        // Clear virtual LEDs
        for (int i = 0; i < virtualSize; i++) {
            virtualLEDs[i] = CRGB::Black;
        }
        mapVirtualToPhysical();

        // Apply timing updates during OFF period (safe time to update)
        if (timingUpdatePending) {
            onDuration = newOnDuration;
            offDuration = newOffDuration;
            timingUpdatePending = false;
        }

        // Set alarm for OFF duration
        timerAlarmWrite(hwTimer, offDuration, true);
    }
}
#endif

// Ring buffer management
void LEDGroup::advanceReadIndex() {
    if(!isBufferEmpty()) {
        readIndex = (readIndex + 1) % FRAME_BUFFER_SIZE;
        bufferedFrames--;
    }
}

void LEDGroup::advanceWriteIndex() {
    if(!isBufferFull()) {
        writeIndex = (writeIndex + 1) % FRAME_BUFFER_SIZE;
        bufferedFrames++;
    }
}

CRGB* LEDGroup::getWriteFrame() {
    return frameBuffer[writeIndex];
}

CRGB* LEDGroup::getReadFrame() {
    return frameBuffer[readIndex];
}

bool LEDGroup::isBufferFull() {
    return bufferedFrames >= FRAME_BUFFER_SIZE;
}

bool LEDGroup::isBufferEmpty() {
    return bufferedFrames == 0;
}

// Effect management
void LEDGroup::setEffectFunction(void (*func)(CRGB*, uint16_t, uint8_t)) {
    effectFunction = func;
}

void LEDGroup::defaultColorEffect() {
    defaultColorEffect(virtualLEDs);
}

void LEDGroup::defaultColorEffect(CRGB* targetArray) {
    CRGB color = getDefaultGroupColor();
    for(int i = 0; i < virtualSize; i++) {
        targetArray[i] = color;
    }
}

void LEDGroup::applyEffect() {
    if(effectFunction != nullptr) {
        effectFunction(virtualLEDs, virtualSize, groupNumber);
    } else {
        defaultColorEffect();
    }
}

// Square wave control
void LEDGroup::setOnDuration(unsigned long onTime) {
#ifdef ESP32
    if (hardwareTimerEnabled) {
        // Queue update for next OFF period
        newOnDuration = onTime;
        timingUpdatePending = true;
    } else {
        onDuration = onTime;
    }
#else
    onDuration = onTime;
#endif
}

void LEDGroup::setOffDuration(unsigned long offTime) {
#ifdef ESP32
    if (hardwareTimerEnabled) {
        // Queue update for next OFF period
        newOffDuration = offTime;
        timingUpdatePending = true;
    } else {
        offDuration = offTime;
    }
#else
    offDuration = offTime;
#endif
}

void LEDGroup::setDutyCycle(float percentage, unsigned long totalPeriod) {
    if(percentage < 0.0) percentage = 0.0;
    if(percentage > 1.0) percentage = 1.0;

    unsigned long newOn = totalPeriod * percentage;
    unsigned long newOff = totalPeriod * (1.0 - percentage);

#ifdef ESP32
    if (hardwareTimerEnabled) {
        // Queue update for next OFF period
        newOnDuration = newOn;
        newOffDuration = newOff;
        timingUpdatePending = true;
    } else {
        onDuration = newOn;
        offDuration = newOff;
    }
#else
    onDuration = newOn;
    offDuration = newOff;
#endif
}

bool LEDGroup::getSquareWaveState() {
    return squareWaveState;
}

// Utility functions
void LEDGroup::setGroupColor(CRGB color) {
    fillVirtual(color);
}

void LEDGroup::fillVirtual(CRGB color) {
    for(int i = 0; i < virtualSize; i++) {
        virtualLEDs[i] = color;
    }
}

CRGB LEDGroup::getDefaultGroupColor() {
    // Simple color mapping based on group number
    switch(groupNumber % 12) {
        case 1: return CRGB::Red;
        case 2: return CRGB::Green;
        case 3: return CRGB::Blue;
        case 4: return CRGB::Yellow;
        case 5: return CRGB::Cyan;
        case 6: return CRGB::Magenta;
        case 7: return CRGB::Orange;
        case 8: return CRGB::Purple;
        case 9: return CRGB::Pink;
        case 10: return CRGB::Lime;
        case 11: return CRGB::Aqua;
        case 0: return CRGB::White;
        default: return CRGB::Gray;
    }
}

// Getters
uint8_t LEDGroup::getGroupNumber() const {
    return groupNumber;
}

uint16_t LEDGroup::getVirtualSize() const {
    return virtualSize;
}

uint8_t LEDGroup::getReadIndex() const {
    return readIndex;
}

uint8_t LEDGroup::getWriteIndex() const {
    return writeIndex;
}

CRGB* LEDGroup::getVirtualLEDs() {
    return virtualLEDs;
}
