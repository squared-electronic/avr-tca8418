#ifndef TCA8418_h
#define TCA8418_h

#include <stdint.h>

class TCA8418 {
 public:
  typedef uint8_t Error;
  static const Error NO_ERROR = 0;

  enum class row_t : uint8_t {
    ROW0 = 0,
    ROW1 = 1,
    ROW2 = 2,
    ROW3 = 3,
    ROW4 = 4,
    ROW5 = 5,
    ROW6 = 6,
    ROW7 = 7,
  };

  enum class col_t : uint8_t {
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

  struct Config {
    struct Keypad_ {
      TCA8418::row_t* Rows = nullptr;
      TCA8418::col_t* Cols = nullptr;
      uint8_t RowsCount = 0;
      uint8_t ColsCount = 0;
    } Keypad;

    struct GpioIn_ {
      bool InterruptOnRisingEdge = false;
      bool EnablePullups = true;
      bool EnableDebounce = true;
      TCA8418::pin_t* Pins = nullptr;
      uint8_t PinsCount = 0;
    } GpioInput;
  };

  typedef void (*KeyCodeCallback)(uint8_t);

  Error begin(const Config* c);
  void updateButtonStates();
  bool wasKeyPressed(uint8_t keyCode) const;
  bool wasKeyReleased(uint8_t keyCode) const;
  bool isKeyHeld(uint8_t keyCode) const;
  Error handleInterrupt();
  void setKeyPressedCallback(KeyCodeCallback cb);
  void setKeyReleasedCallback(KeyCodeCallback cb);

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
    GPIO_PULL1 = 0x2C,
    GPIO_PULL2 = 0x2D,
    GPIO_PULL3 = 0x2E,
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

  Error configureKeypad(const TCA8418::Config::Keypad_* config);
  Error configureGpioInputs(const TCA8418::Config::GpioIn_* config);
  void createRegisterTripleMask(pin_t* pins, uint8_t pins_count, uint8_t register_triple[3]);
  Error writeRegister(register_t register_address, uint8_t data);
  Error modifyRegister(register_t register_address, uint8_t data, uint8_t mask);
  Error readRegister(register_t register_address, uint8_t* out_data);
  Error readKeyEventsFifo();
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
  KeyCodeCallback keyPressCallback_{nullptr};
  KeyCodeCallback keyReleaseCallback_{nullptr};
};

#endif