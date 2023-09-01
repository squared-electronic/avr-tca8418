#ifndef TCA8418_h
#define TCA8418_h

class TCA8418 {
  enum class register_t : uint8_t {
    CFG = 0x01,
    INT_STAT = 0x02,
    KEY_LCK_EC = 0x03,
    KEY_EVENT_A = 0x04
  };

  enum class key_event_type_t : uint8_t {
    RELEASED = 0,
    PRESSED = 1,
  };

 public:
  void begin();
  void configureKeys();
  void updateButtonStates();
  bool wasKeyPressed(uint8_t keyCode) const;
  bool wasKeyReleased(uint8_t keyCode) const;
  bool isKeyHeld(uint8_t keyCode) const;
  ret_code_t handleInterupt();

 private:
  const uint8_t I2C_ADDRESS = 0x34;

  ret_code_t writeRegister(register_t register_address, uint8_t data);
  ret_code_t readRegister(register_t register_address, uint8_t* out_data);
  ret_code_t readKeyEvents(uint8_t* fifo_out, uint8_t* fifo_items);
  uint8_t readBit(const uint8_t* bytes, uint8_t bitIndex) const;

  uint8_t keysPushed[10];  // TODO not long enough buffer to support GPIOs
  uint8_t keysReleased[10];
  uint8_t keysStillPushed[10];
};

#endif