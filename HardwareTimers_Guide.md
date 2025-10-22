# ESP Hardware Timers Guide for LED Square Wave Generation

## Overview

Hardware timers provide microsecond-precision timing with minimal CPU overhead. Unlike software timers (like TickTwo), hardware timers run independently and trigger interrupts at precise intervals.

**Perfect for:**
- High-precision square wave generation
- POV (Persistence of Vision) LED displays
- Duty cycle control with minimal jitter
- Real-time LED multiplexing

## ESP8266 Hardware Timers

### Available Timers

ESP8266 has **1 hardware timer** available to users:
- **Timer1** - 23-bit timer, programmable divider

(Timer0 is used by system functions and WiFi)

### Timer1 Specifications

- **Resolution**: 23-bit counter (0 to 8,388,607)
- **Clock Source**: 80 MHz CPU clock
- **Prescaler**: Divides clock by 1, 16, or 256
- **Max Period**:
  - `/1`: 104.8 microseconds
  - `/16`: 1.67 milliseconds
  - `/256`: 26.8 milliseconds
- **Reload**: Can auto-reload for continuous operation

### Timer1 Advantages

✅ **Microsecond precision** - 12.5ns per tick at 80MHz
✅ **Non-blocking** - Runs independently of main code
✅ **ISR-based** - Callback at exact intervals
✅ **Auto-reload** - Perfect for square waves

### Timer1 Limitations

⚠️ **Only ONE timer** - Must multiplex or use clever state machines
⚠️ **ISR context** - Keep interrupt handlers fast
⚠️ **No PWM output** - Must toggle pins in software

## ESP32 Hardware Timers

### Available Timers

ESP32 has **4 hardware timers**:
- **Timer Group 0**: Timer 0, Timer 1
- **Timer Group 1**: Timer 0, Timer 1

All timers are **64-bit** with full control over prescaler and counter.

### Timer Specifications

- **Resolution**: 64-bit counter
- **Clock Source**: 80 MHz APB clock (default)
- **Prescaler**: 2 to 65536 (divides APB clock)
- **Max Period**: Practically unlimited with 64-bit counter
- **Modes**: One-shot or auto-reload
- **Alarm**: Programmable compare value

### ESP32 Advantages

✅ **Four independent timers** - One per LED group!
✅ **64-bit precision** - Massive range
✅ **Flexible prescaler** - Fine-tune resolution
✅ **Alarm interrupts** - Trigger at specific counts
✅ **Better for complex timing** - Multiple simultaneous square waves

### ESP32 LED Control (LEDC)

ESP32 also has dedicated **LED PWM hardware (LEDC)**:
- **16 channels** - Independent PWM outputs
- **Hardware duty cycle** - No CPU needed
- **Frequency control** - 1 Hz to 40 MHz
- **Resolution**: Up to 16-bit (adjustable)

**LEDC is IDEAL for LED timing but limited to specific pins**

## Comparison: Software vs Hardware Timers

| Feature | TickTwo (Software) | Hardware Timer | LEDC (ESP32) |
|---------|-------------------|----------------|--------------|
| **Precision** | ±10-100μs | ±1μs | ±0.1μs |
| **CPU Usage** | Medium | Very Low | None |
| **Multiple Timers** | Unlimited | 1 (ESP8266), 4 (ESP32) | 16 (ESP32) |
| **Dynamic Adjustment** | Easy | Medium | Easy |
| **ISR Overhead** | None | Small | None |
| **Best For** | General timing | Critical timing | LED PWM |

## When to Use Each

### Use TickTwo (Software) When:
- You need many timers (>4)
- Timing precision of ~100μs is acceptable
- Frequent interval changes
- Simple to implement

### Use Hardware Timers When:
- Need microsecond precision
- Consistent, jitter-free timing critical
- CPU cycles must be minimized
- Square wave generation for POV

### Use LEDC (ESP32 only) When:
- Dedicated PWM on specific pins is acceptable
- Need hardware-generated waveforms
- 16 channels or less
- Absolutely minimal CPU usage required

## Calculating Timer Settings

### For ESP8266 Timer1

```
Timer Frequency = CPU Clock / Prescaler / Ticks
Ticks = (CPU Clock / Prescaler) / Desired Frequency

Example: 1kHz square wave (1ms period)
- CPU Clock: 80,000,000 Hz
- Prescaler: 80 (divide by 80)
- Timer Clock: 80,000,000 / 80 = 1,000,000 Hz (1MHz)
- For 1kHz: Ticks = 1,000,000 / 1,000 = 1,000 ticks
```

### For ESP32 Timers

```
Timer Frequency = APB Clock / Prescaler
Alarm Value = Timer Frequency / Desired Interrupt Rate

Example: 10kHz interrupts for LED timing
- APB Clock: 80,000,000 Hz
- Prescaler: 80
- Timer Frequency: 1,000,000 Hz (1MHz)
- Alarm: 1,000,000 / 10,000 = 100 ticks
```

## Adjusting Duty Cycle Dynamically

### State Machine Approach (Works on ESP8266 with 1 timer)

```cpp
volatile bool ledState = false;
volatile uint32_t onTicks = 900;   // 90% duty
volatile uint32_t offTicks = 100;  // 10% duty
volatile bool pendingUpdate = false;
volatile uint32_t newOnTicks, newOffTicks;

void ICACHE_RAM_ATTR timerISR() {
    // Toggle state
    ledState = !ledState;

    // Update timing if pending
    if(pendingUpdate && !ledState) {
        onTicks = newOnTicks;
        offTicks = newOffTicks;
        pendingUpdate = false;
    }

    // Reload timer with appropriate duration
    if(ledState) {
        timer1_write(onTicks);
    } else {
        timer1_write(offTicks);
    }

    // Call LED update (keep FAST!)
    updateLEDs();
}

// Safe update from main loop
void setDutyCycle(float duty, uint32_t periodUs) {
    newOnTicks = (periodUs * duty) * (80000000 / 80) / 1000000;
    newOffTicks = (periodUs * (1-duty)) * (80000000 / 80) / 1000000;
    pendingUpdate = true;
}
```

### Multi-Timer Approach (ESP32 - one timer per group)

```cpp
void IRAM_ATTR timer0ISR() {
    timerAlarmWrite(timer0, group1OnTicks, true);
    // Update group 1 LEDs
}

void IRAM_ATTR timer1ISR() {
    timerAlarmWrite(timer1, group2OnTicks, true);
    // Update group 2 LEDs
}

// Each group gets independent timing control
void setGroup1Timing(uint32_t periodUs, float duty) {
    group1OnTicks = calculateTicks(periodUs, duty);
    // Timer automatically uses new value on next alarm
}
```

## Memory Considerations

### ISR Variables
- Always use `volatile` keyword
- Keep ISRs SHORT (<10μs execution)
- Use `ICACHE_RAM_ATTR` (ESP8266) or `IRAM_ATTR` (ESP32)
- Avoid `Serial.print()` in ISRs

### LED Updates in ISR
```cpp
// GOOD - Fast bit manipulation
GPOS = (1 << LED_PIN);  // Turn on
GPOC = (1 << LED_PIN);  // Turn off

// GOOD - FastLED with careful usage
FastLED.show();  // Can work if optimized

// BAD - Slow operations
digitalWrite(pin, HIGH);  // Too slow
delay(10);                // NEVER in ISR
Serial.println();         // NEVER in ISR
```

## Recommendations for Your Project

### For Spinning Cube POV:

**ESP8266**:
- Use **Timer1 with state machine** for square wave
- 4 sides = 4 states in ISR
- Calculate ON/OFF times per side

**ESP32**:
- Use **4 hardware timers** (one per side)
- Or use **LEDC** if you can dedicate pins
- Gives independent control per face

### Integration Strategy:

1. **Replace software timing in LEDGroup** with hardware callbacks
2. **Keep frame buffering** - that's still valuable
3. **Use ISR to trigger** frame transitions
4. **Main loop** focuses on frame generation

## Next Steps

See the example implementations:
- `ESP8266_HardwareTimer_Example.ino` - Single timer with state machine
- `ESP32_HardwareTimer_Example.ino` - Multi-timer approach
- `LEDGroup_HardwareTimer_Integration.ino` - Full integration

## References

- [ESP8266 Timer Documentation](https://www.espressif.com/sites/default/files/documentation/esp8266-technical_reference_en.pdf)
- [ESP32 Timer API](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/timer.html)
- [ESP32 LEDC Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/ledc.html)
