#include "LEDGroup.h"

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
}

// Destructor
LEDGroup::~LEDGroup() {
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
    onDuration = onTime;
}

void LEDGroup::setOffDuration(unsigned long offTime) {
    offDuration = offTime;
}

void LEDGroup::setDutyCycle(float percentage, unsigned long totalPeriod) {
    if(percentage < 0.0) percentage = 0.0;
    if(percentage > 1.0) percentage = 1.0;

    onDuration = totalPeriod * percentage;
    offDuration = totalPeriod * (1.0 - percentage);
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
