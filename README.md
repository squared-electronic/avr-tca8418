# AVR TCA8418 I2C Keypad Driver

**Product Page**: https://www.ti.com/product/TCA8418

**Datasheet**: https://www.ti.com/lit/ds/symlink/tca8418.pdf

This is a driver for the TCA8418 for AVR microcontrollers.

## Features

- Interrupt-driven only
- Reports key presses, releases, and holds.
- Supports callbacks for key presses and releases
- Properly reports multi-key presses, holds, and releases
- Support GPIO interrupt-driven inputs
- Small SRAM size for all device 80-key and 18-GPIO support
- `wasKeyPressed`, `wasKeyReleased`, `isKeyHeld` simple API
- Provides error codes on all I2C operations which may fail

## Not Supported

- GPIO Outputs
- Keypad lock / unlock features

## Usage

- Initialize I2C
- Enable an external interrupt on falling edge, and connect the TCA8418's INT pin to the MCU
- Call `handleInterrupt` in the main loop if the external interrupt was triggered
- Place `updateButtonStates` at the beginning of the main loop to process any pending interrupt events to be observable by the API on this loop

The `wasKeyPressed` and similar API is guaranteed to only return `true` once, then be false after the next call to `updateButtonStates`, unless the key is re-pressed. Use `isKeyHeld` to detect holds. This prevents duplicate events on checking for a key press on each loop.

The driver guarantees each iteration of the main loop is completed with the same information. For example, if an interrupt arrives in the middle of the main loop, no state changes will be observed until after the next call to `updateButtonStates`.

The driver includes a small I2C wrapper (modified from https://github.com/Sovichea/avr-i2c-library) and expects the user to initialize the I2C bus to their application's needs.

## ATmega324 Example

```cpp
#include <TCA8418.h>
#include <avr/interrupt.h>
#include <avr/io.h>

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

volatile bool CheckKeypad = false;

void onKeyPressed(uint8_t keyCode) {
  // Do something with key press....
}

void onKeyReleased(uint8_t keyCode) {
  // Do something with key release....
}

int main() {
  initI2c();
  initInterrupts();
  sei();

  TCA8418 keypad;

  TCA8418::row_t key_rows[] = {TCA8418::row_t::ROW0, TCA8418::row_t::ROW1, TCA8418::row_t::ROW2,
                               TCA8418::row_t::ROW3};

  TCA8418::col_t key_cols[] = {TCA8418::col_t::COL0, TCA8418::col_t::COL1};

  TCA8418::pin_t gpios[] = {TCA8418::pin_t::COL6, TCA8418::pin_t::COL7};

  TCA8418::Config c = {
      .Keypad =
          {
              .Rows = key_rows,
              .Cols = key_cols,
              .RowsCount = sizeof(key_rows) / sizeof(key_rows[0]),
              .ColsCount = sizeof(key_cols) / sizeof(key_cols[0]),
          },
      .GpioInput =
          {
              .InterruptOnRisingEdge = false,
              .EnablePullups = false,
              .EnableDebounce = true,
              .Pins = gpios,
              .PinsCount = sizeof(gpios) / sizeof(gpios[0]),
          },
  };

  keypad.setKeyPressedCallback(onKeyPressed);
  keypad.setKeyReleasedCallback(onKeyReleased);

  auto error = keypad.begin(&c);

  if (error) {
    // Handle error...
  }

  // Key codes reported directly from hardware
  const uint8_t keyCodes[] = {
      1, 2, 3, 11, 12, 13, 21, 22, 23, 31, 32, 33,
  };

  while (1) {
    if (CheckKeypad) {
      keypad.handleInterrupt();
      CheckKeypad = false;
    }

    keypad.updateButtonStates();

    // Check for keys directly, or rely on the onKeyPress and onKeyRelease callbacks.
    for (uint8_t i = 0; i < sizeof(keyCodes); ++i) {
      if (keypad.wasKeyPressed(keyCodes[i])) {
        // Handle key press
      } else if (keypad.wasKeyReleased(keyCodes[i])) {
        // Handle key release
      }
    }
  }

  return 0;
}

ISR(INT1_vect) {
  CheckKeypad = true;
}
```
