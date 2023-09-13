# AVR TCA8418 I2C Keypad Driver

**Product Page**: https://www.ti.com/product/TCA8418

**Datasheet**: https://www.ti.com/lit/ds/symlink/tca8418.pdf

This is a driver for the TCA8418 for AVR microcontrollers.

## Features

- Interrupt-driven only
- Reports key presses, releases, and holds.
- Properly reports multi-key presses, holds, and releases
- All hardware row and column keypad configurations supported
- `wasKeyPressed`, `wasKeyReleased`, `isKeyHeld` simple API
- 46 bytes of SRAM required for full for 80-key and 18-GPIO map space
- Provides error codes on all I2C operations which may fail
- Disables interrupts around all I2C communication to prevent reading/writing garbage

## Usage

- Initialize I2C
- Enable an external interrupt on falling edge, and connect the TCA8418's INT pin to the MCU
- Add an interrupt vector, and call `handleInterrupt` inside
- Place `updateButtonStates` at the beginning of the main loop to process any pending interrupt events to be observable by the API on this loop

The `wasKeyPressed` and similar API is guaranteed to only return `true` once, then be false after the next call ot `updateButtonStates`, unless the key is re-pressed. Use `isKeyHeld` to detect holds. This prevents duplicate events on checking for a key press each on each loop.

The driver guarantees each iteration of the main loop is completed with the same information. For example, if an interrupt arrives in the middle of the main loop, no state changes will be observed until the start of the next loop. Place `updateButtonStates` at the start of the main loop to sync any pending events.

The driver includes a small I2C wrapper (modified from https://github.com/Sovichea/avr-i2c-library) and expects the user to initialize the I2C bus to their application's needs.

## Example

```cpp
#include <TCA8418.h>

void initI2c() {
  // set pullups for SDA / SCL
  DDRC &= ~_BV(PC1);
  DDRC &= ~_BV(PC0);
  PORTC |= _BV(PC1) | _BV(PC0);

  // initialize twi prescaler and bit rate (250k)
  // Set prescaler value of 1
  TWSR &= ~_BV(TWPS0);
  TWSR &= ~_BV(TWPS1);
  TWBR = ((F_CPU / 250000) - 16) / 2;
}

void initInterrupts() {
  // Falling edge of INT1 for keypad to read current key
  EICRA |= _BV(ISC11);
  EICRA &= ~_BV(ISC10);

  EIMSK |= _BV(INT1);
}

TCA8418 Keypad;

int main() {
  initI2c();
  initInterrupts();
  sei();

  uint8_t rows[] = {0, 1, 2, 3};
  uint8_t cols[] = {0, 1, 2};

  auto error = Keypad.begin();
  if (error) {
    // handle error...
  }

  error = Keypad.configureKeypad(rows, 4, cols, 3);
  if (error) {
    // handle error...
  }

  // Key codes reported directly from hardware
  const uint8_t keyCodes[] = {
       1,  2,  3,
      11, 12, 13,
      21, 22, 23,
      31, 32, 33,
  };

  while (1) {
    Keypad.updateButtonStates();

    for (uint8_t i = 0; i < sizeof(keyCodes); ++i) {
      if (Keypad.wasKeyPressed(keyCodes[i])) {
        // Handle key press
      } else if (Keypad.wasKeyReleased(keyCodes[i])) {
        // Handle key release
      }
    }
  }

  return 0;
}

ISR(INT1_vect) {
    Keypad.handleInterrupt();
}

```
