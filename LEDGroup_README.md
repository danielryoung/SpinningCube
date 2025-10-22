# LEDGroup Library

A flexible LED group management library for Arduino/ESP projects using FastLED. Designed for spinning LED displays (POV - Persistence of Vision) and multiplexed LED control.

## Features

- **Virtual LED Arrays**: Create virtual LED groups that map to physical strips
- **Frame Buffering**: 4-frame circular buffer for smooth animations
- **Square Wave Timing**: Built-in timing control for multiplexed/POV displays
- **Custom Effects**: Assign custom effect functions to each group
- **Memory Efficient**: Share one physical LED array across multiple groups

## Use Cases

- Spinning LED cubes/displays (POV effects)
- Multiplexed LED matrices
- Multi-zone LED control
- Pre-rendered animation playback
- Synchronized LED groups with different timing

## Installation

1. Copy `LEDGroup.h` and `LEDGroup.cpp` to your project directory
2. Include in your sketch:
   ```cpp
   #include "LEDGroup.h"
   ```

## Quick Start

```cpp
#include <FastLED.h>
#include "LEDGroup.h"

#define NUM_LEDS 48
CRGB leds[NUM_LEDS];

LEDGroup* side1;

void setup() {
    FastLED.addLeds<APA102, DATA_PIN, CLOCK_PIN>(leds, NUM_LEDS);

    // Create group: (physical array, size, group number, virtual size)
    side1 = new LEDGroup(leds, NUM_LEDS, 1, 12);

    // Configure timing (90% duty cycle, 10ms period)
    side1->setDutyCycle(0.9, 10000);
}

void loop() {
    side1->updateSquareWave();
    side1->processStateChanges();
    side1->fillBuffer();
    side1->mapVirtualToPhysical();
    FastLED.show();
}
```

## API Reference

### Constructor

```cpp
LEDGroup(CRGB* physicalArray, uint16_t physicalArraySize,
         uint8_t group, uint16_t virtualSectionSize)
```

- `physicalArray`: Pointer to the shared physical LED array
- `physicalArraySize`: Total number of LEDs in physical array
- `group`: Group number (1-based, used for positioning and default colors)
- `virtualSectionSize`: Number of LEDs in this virtual group

### Core Functions

#### `void updateSquareWave()`
Updates the internal square wave timing state. Call this every loop iteration.

#### `void processStateChanges()`
Handles frame transitions based on square wave state. Call after `updateSquareWave()`.

#### `void fillBuffer()`
Generates new frames to keep the buffer full. Call regularly to ensure smooth animation.

#### `void mapVirtualToPhysical()`
Maps the virtual LED array to the physical strip. Call after all updates.

### Timing Control

#### `void setOnDuration(unsigned long onTime)`
Set the ON duration in microseconds.

#### `void setOffDuration(unsigned long offTime)`
Set the OFF duration in microseconds.

#### `void setDutyCycle(float percentage, unsigned long totalPeriod)`
Set duty cycle as a percentage (0.0-1.0) of total period in microseconds.

Example:
```cpp
// 95% on, 5% off, 8ms total period
group->setDutyCycle(0.95, 8000);
```

#### `bool getSquareWaveState()`
Returns `true` if currently ON, `false` if OFF.

### Effect Functions

#### `void setEffectFunction(void (*func)(CRGB*, uint16_t, uint8_t))`
Assign a custom effect function.

Function signature:
```cpp
void myEffect(CRGB* leds, uint16_t count, uint8_t groupNum) {
    // Modify leds array
}
```

#### `void applyEffect()`
Manually apply the current effect to virtual LEDs.

#### `void defaultColorEffect()`
Apply the default color (based on group number).

### Utility Functions

#### `void setGroupColor(CRGB color)`
Set all LEDs in group to a specific color.

#### `void fillVirtual(CRGB color)`
Fill virtual LED array with a color.

#### `CRGB getDefaultGroupColor()`
Get the default color assigned to this group number.

### Getters

```cpp
uint8_t getGroupNumber()    // Returns group number
uint16_t getVirtualSize()   // Returns virtual LED count
uint8_t getReadIndex()      // Returns current read buffer index
uint8_t getWriteIndex()     // Returns current write buffer index
CRGB* getVirtualLEDs()      // Returns pointer to virtual LED array
```

## Advanced Usage

### Custom Effects

Create effect functions that generate patterns:

```cpp
void rainbowEffect(CRGB* leds, uint16_t count, uint8_t groupNum) {
    static uint8_t hue = 0;
    fill_rainbow(leds, count, hue, 255 / count);
    hue += 2;
}

void setup() {
    group->setEffectFunction(rainbowEffect);
}
```

### POV Display Integration

For spinning displays, calculate timing based on RPM:

```cpp
void updateTiming(float rpm) {
    // Calculate rotation period
    unsigned long rotationPeriod = (60.0 / rpm) * 1000000; // microseconds

    // For 4 sides, each gets 1/4 of rotation
    unsigned long framePeriod = rotationPeriod / 4;

    // Set 95% duty cycle (5% black for visual separation)
    for(int i = 0; i < 4; i++) {
        sides[i]->setDutyCycle(0.95, framePeriod);
    }
}
```

### Multiple Groups

Manage multiple LED sections independently:

```cpp
LEDGroup* groups[4];

void setup() {
    for(int i = 0; i < 4; i++) {
        groups[i] = new LEDGroup(leds, NUM_LEDS, i+1, LEDS_PER_SIDE);
        groups[i]->setDutyCycle(0.9, 10000);
    }
}

void loop() {
    for(int i = 0; i < 4; i++) {
        groups[i]->updateSquareWave();
        groups[i]->processStateChanges();
        groups[i]->fillBuffer();
        groups[i]->mapVirtualToPhysical();
    }
    FastLED.show();
}
```

## How It Works

### Frame Buffer

Each LEDGroup maintains a circular buffer of 4 frames:

1. **Write Index**: Points to where new frames are generated
2. **Read Index**: Points to the current frame being displayed
3. **Buffered Frames**: Tracks how many frames are ready

This allows smooth animations even with varying generation times.

### Square Wave Timing

The square wave controls when LEDs are ON or OFF:

- **ON State**: Display current frame from buffer
- **OFF State**: Advance to next frame, clear display
- **Transitions**: Automatic based on configured durations

Perfect for POV displays where each group needs precise timing.

### Physical Mapping

Virtual LEDs map to physical strip based on group number:

```
Group 1: Physical LEDs [0-11]
Group 2: Physical LEDs [12-23]
Group 3: Physical LEDs [24-35]
Group 4: Physical LEDs [36-47]
```

Customize `mapVirtualToPhysical()` for different layouts.

## Performance Tips

1. **Keep Buffer Full**: Call `fillBuffer()` regularly to avoid stalls
2. **Optimize Effects**: Complex effects may slow frame generation
3. **Memory Management**: Each group allocates `virtualSize * 5` CRGB structs
4. **Timing Precision**: Use microseconds for accurate POV displays

## Examples

See `LEDGroup_Example.ino` for complete working examples including:
- Rainbow effects
- Pulse animations
- Sparkle effects
- ESP-NOW integration
- Dynamic timing adjustment

## Troubleshooting

**LEDs not displaying**
- Check that `mapVirtualToPhysical()` is called before `FastLED.show()`
- Verify physical array is properly initialized

**Jerky animations**
- Ensure `fillBuffer()` is called frequently enough
- Check that effect functions complete quickly

**Wrong colors/positions**
- Verify group numbers match your physical layout
- Adjust `mapVirtualToPhysical()` for your wiring

**Timing issues**
- Use microseconds, not milliseconds
- Account for actual rotation speed in POV setups

## License

Free to use and modify for any purpose.

## Credits

Designed for spinning LED cube projects using FastLED library.
