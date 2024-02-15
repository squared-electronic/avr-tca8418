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

TCA8418 Keypad;

int main() {
  initI2c();
  initInterrupts();
  sei();

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

  auto error = Keypad.begin(&c);

  if (error) {
    // Handle error...
  }

  // Key codes reported directly from hardware
  const uint8_t keyCodes[] = {
      1, 2, 3, 11, 12, 13, 21, 22, 23, 31, 32, 33,
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