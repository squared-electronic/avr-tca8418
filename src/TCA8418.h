#ifndef TCA8418_h
#define TCA8418_h

#include <stdint.h>

class TCA8418 {
 public:
  enum class pin_t : uint8_t {
    ROW0 = 0,
    ROW1 = 1,
    ROW2 = 2,
    ROW3 = 3,
    ROW4 = 4,
    ROW5 = 5,
    ROW6 = 6,
    ROW7 = 7,
    COL0 = 8,
    COL1 = 9,
    COL2 = 10,
    COL3 = 11,
    COL4 = 12,
    COL5 = 13,
    COL6 = 14,
    COL7 = 15,
    COL8 = 16,
    COL9 = 17,
  };

  uint8_t begin();
  uint8_t configureKeypad(uint8_t* rows, uint8_t* cols, uint8_t rows_count, uint8_t cols_count);
  uint8_t configureGpio(pin_t* pins, uint8_t pins_count, bool enablePullup);
  void updateButtonStates();
  bool wasKeyPressed(uint8_t keyCode) const;
  bool wasKeyReleased(uint8_t keyCode) const;
  bool isKeyHeld(uint8_t keyCode) const;
  uint8_t handleInterupt();

 private:
  enum class register_t : uint8_t {
    CFG = 0x01,
    INT_STAT = 0x02,
    KEY_LCK_EC = 0x03,
    KEY_EVENT_A = 0x04,
    KP_GPIO1 = 0x1D,
    KP_GPIO2 = 0x1E,
    KP_GPIO3 = 0x1F,
    GPIO_INT_EN1 = 0x1A,
    GPIO_INT_EN2 = 0x1B,
    GPIO_INT_EN3 = 0x1C,
    GPIO_EM1 = 0x20,
    GPIO_EM2 = 0x21,
    GPIO_EM3 = 0x22,
    GPIO_DIR1 = 0x23,
    GPIO_DIR2 = 0x24,
    GPIO_DIR3 = 0x25,
    GPIO_INT_LVL1 = 0x26,
    GPIO_INT_LVL2 = 0x27,
    GPIO_INT_LVL3 = 0x28,
  };

  enum class key_event_type_t : uint8_t {
    RELEASED = 0,
    PRESSED = 1,
  };

  enum class keycode_type_t : uint8_t {
    UNKNOWN = 0,
    KEYPAD = 1,
    GPIO = 2,
  };

  void createRegisterTripleMask(pin_t* pins, uint8_t pins_count, uint8_t register_triple[3]);
  uint8_t writeRegister(register_t register_address, uint8_t data);
  uint8_t modifyRegister(register_t register_address, uint8_t data, uint8_t mask);
  uint8_t readRegister(register_t register_address, uint8_t* out_data);
  uint8_t readKeyEventsFifo();
  void updateButtonState(uint8_t pendingEvent);
  uint8_t readBit(const uint8_t* bytes, uint8_t bitNumber) const;
  void setBit(uint8_t* bytes, uint8_t bitNumber) const;
  void clearBit(uint8_t* bytes, uint8_t bitNumber) const;
  keycode_type_t classifyKeycode(uint8_t keyCode) const;
  keycode_type_t mapKeyCodeToArray(uint8_t rawKeyCode, uint8_t* outCorrectedCode) const;
  bool readKeyBit(const uint8_t* bytes, uint8_t rawKeyCode) const;

  const uint8_t I2C_ADDRESS = 0x34;
  uint8_t keysPushed[12];
  uint8_t keysReleased[12];
  uint8_t keysStillPushed[12];
  uint8_t pendingEvents[10];
  uint8_t pendingEventsCount = 0;
};

#endif