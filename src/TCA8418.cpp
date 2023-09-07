#include "TCA8418.h"

#include <avr/interrupt.h>
#include <string.h>
#include <twi_master.h>

struct ScopedInterruptLock2 {
  ScopedInterruptLock2();
  ~ScopedInterruptLock2();

 private:
  uint8_t SReg;
};

ScopedInterruptLock2::ScopedInterruptLock2() {
  SReg = SREG;
  cli();
}

ScopedInterruptLock2::~ScopedInterruptLock2() { SREG = SReg; }

#define TRY_ERR(function)    \
  do {                       \
    auto error = function;   \
    if (error) return error; \
  } while (0);

uint8_t TCA8418::begin() {
  // 4 INT_CFG - processor interrupt is deasserted for 50 μs and reassert with pending interrupts
  TRY_ERR(writeRegister(register_t::CFG, 0b0001'0000));
  return SUCCESS;
}

uint8_t TCA8418::configureKeypad(uint8_t* rows, uint8_t* cols, uint8_t rows_count,
                                 uint8_t cols_count) {
  uint8_t kpGpio1Reg = 0;
  uint8_t kpGpio2Reg = 0;
  uint8_t kpGpio3Reg = 0;

  for (uint8_t i = 0; i < rows_count; ++i) {
    kpGpio1Reg |= (1 << rows[i]);
  }

  for (uint8_t i = 0; i < cols_count; ++i) {
    uint8_t col = cols[i];
    if (col < 8) {
      kpGpio2Reg |= (1 << cols[i]);
    } else {
      kpGpio3Reg |= (1 << cols[i]);
    }
  }

  // Enable rows / cols and  Keypad interrupts
  TRY_ERR(writeRegister(register_t::KP_GPIO1, kpGpio1Reg));
  TRY_ERR(writeRegister(register_t::KP_GPIO2, kpGpio2Reg));
  TRY_ERR(writeRegister(register_t::KP_GPIO3, kpGpio3Reg));
  TRY_ERR(modifyRegister(register_t::CFG, 1, 0x01));

  return SUCCESS;
}

uint8_t TCA8418::configureGpio(pin_t* pins, uint8_t pins_count, bool) {
  ScopedInterruptLock2 lock;
  uint8_t reg_data_mask[3] = {0, 0, 0};
  createRegisterTripleMask(pins, pins_count, reg_data_mask);

  // Set as GPIO, instead of keypad (KP_GPIO1–3), write 0s here
  TRY_ERR(modifyRegister(register_t::KP_GPIO1, 0x00, reg_data_mask[0]));
  TRY_ERR(modifyRegister(register_t::KP_GPIO2, 0x00, reg_data_mask[1]));
  TRY_ERR(modifyRegister(register_t::KP_GPIO3, 0x00, reg_data_mask[2]));

  // Enable interrupts for selected pins (GPIO_INT_EN1–3), write 1s here
  TRY_ERR(modifyRegister(register_t::GPIO_INT_EN1, 0xFF, reg_data_mask[0]));
  TRY_ERR(modifyRegister(register_t::GPIO_INT_EN2, 0xFF, reg_data_mask[1]));
  TRY_ERR(modifyRegister(register_t::GPIO_INT_EN3, 0xFF, reg_data_mask[2]));

  // Enable each pin for adding to FIFO (GPI_EM1–3), write 1s here
  TRY_ERR(modifyRegister(register_t::GPIO_EM1, 0xFF, reg_data_mask[0]));
  TRY_ERR(modifyRegister(register_t::GPIO_EM2, 0xFF, reg_data_mask[1]));
  TRY_ERR(modifyRegister(register_t::GPIO_EM3, 0xFF, reg_data_mask[2]));

  // Set selected pins as inputs (GPIO_DIR1–3), write 0s here
  TRY_ERR(modifyRegister(register_t::GPIO_DIR1, 0x00, reg_data_mask[0]));
  TRY_ERR(modifyRegister(register_t::GPIO_DIR2, 0x00, reg_data_mask[1]));
  TRY_ERR(modifyRegister(register_t::GPIO_DIR3, 0x00, reg_data_mask[2]));

  // Set interrupt mode high to low transition (GPIO_INT_LVL1–3) write 0s here (high to low)
  TRY_ERR(modifyRegister(register_t::GPIO_INT_LVL1, 0x00, reg_data_mask[0]));
  TRY_ERR(modifyRegister(register_t::GPIO_INT_LVL2, 0x00, reg_data_mask[1]));
  TRY_ERR(modifyRegister(register_t::GPIO_INT_LVL3, 0x00, reg_data_mask[2]));

  // Enable GPIO Interrupts
  TRY_ERR(modifyRegister(register_t::CFG, 1, 0x02));

  return SUCCESS;
}

void TCA8418::createRegisterTripleMask(pin_t* pins, uint8_t pins_count,
                                       uint8_t register_triple[3]) {
  for (uint8_t i = 0; i < pins_count; ++i) {
    uint8_t pin = static_cast<uint8_t>(pins[i]);
    if (pin < 8) {
      register_triple[0] |= (1 << pin);
    } else if (pin < 16) {
      register_triple[1] |= (1 << pin);
    } else if (pin < 18) {
      register_triple[2] |= (1 << pin);
    }
  }
}

bool TCA8418::wasKeyPressed(uint8_t keyCode) const { return readKeyBit(keysPushed, keyCode); }

bool TCA8418::wasKeyReleased(uint8_t keyCode) const { return readKeyBit(keysReleased, keyCode); }

bool TCA8418::isKeyHeld(uint8_t keyCode) const { return readKeyBit(keysStillPushed, keyCode); }

bool TCA8418::readKeyBit(const uint8_t* bytes, uint8_t rawKeyCode) const {
  uint8_t keyIndex = 0;
  auto keyType = mapKeyCodeToArray(rawKeyCode, &keyIndex);
  if (keyType == keycode_type_t::UNKNOWN) return false;
  return readBit(bytes, keyIndex);
}

uint8_t TCA8418::writeRegister(register_t register_address, uint8_t data) {
  ScopedInterruptLock2 lock;
  uint8_t bytes[2] = {(uint8_t)register_address, data};
  return tw_master_transmit(I2C_ADDRESS, bytes, sizeof(bytes), false);
}

uint8_t TCA8418::modifyRegister(register_t register_address, uint8_t data, uint8_t mask) {
  uint8_t registerData = 0;
  TRY_ERR(readRegister(register_address, &registerData));

  uint8_t modification = (~mask) | data;
  registerData &= modification;
  TRY_ERR(writeRegister(register_address, registerData));

  return SUCCESS;
}

uint8_t TCA8418::readRegister(register_t register_address, uint8_t* out_data) {
  ScopedInterruptLock2 lock;
  TRY_ERR(tw_master_transmit(I2C_ADDRESS, (uint8_t*)register_address, 1, true));
  TRY_ERR(tw_master_receive(I2C_ADDRESS, out_data, 1));
  return SUCCESS;
}

uint8_t TCA8418::handleInterupt() {
  const uint8_t K_INT_BIT = 1;
  const uint8_t GPI_INT_BIT = 2;

  // Read INT_STAT to find out what triggered the interrupt
  uint8_t intStatReg = 0;
  TRY_ERR(readRegister(register_t::INT_STAT, &intStatReg));

  // Support K_INT and GPIP_INT for now, TODO support all interrupts?
  if (intStatReg & _BV(K_INT_BIT) || intStatReg & _BV(GPI_INT_BIT)) {
    // Ignore possible error; Continue to clear interrupt regardless.
    readKeyEventsFifo();
  }

  // Acknowledge interrupt and clear flags
  TRY_ERR(writeRegister(register_t::INT_STAT, 0xFF));

  return SUCCESS;
}

void TCA8418::updateButtonStates() {
  ScopedInterruptLock2 lock;

  memset(keysPushed, 0, sizeof(keysPushed));
  memset(keysReleased, 0, sizeof(keysReleased));

  for (uint8_t i = 0; i < pendingEventsCount; ++i) {
    updateButtonState(pendingEvents[i]);
  }

  pendingEventsCount = 0;
}

uint8_t TCA8418::readBit(const uint8_t* bytes, uint8_t bitNumber) const {
  uint8_t byteIndex = bitNumber / 8;
  uint8_t bitInByteIndex = bitNumber % 8;
  return bytes[byteIndex] & (1 << bitInByteIndex);
}

void TCA8418::setBit(uint8_t* bytes, uint8_t bitNumber) const {
  uint8_t byteIndex = bitNumber / 8;
  uint8_t bitInByteIndex = bitNumber % 8;
  bytes[byteIndex] |= (1 << bitInByteIndex);
}

void TCA8418::clearBit(uint8_t* bytes, uint8_t bitNumber) const {
  uint8_t byteIndex = bitNumber / 8;
  uint8_t bitInByteIndex = bitNumber % 8;
  bytes[byteIndex] &= ~(1 << bitInByteIndex);
}

TCA8418::keycode_type_t TCA8418::classifyKeycode(uint8_t keyCode) const {
  if (keyCode >= 1 && keyCode <= 80) {
    return keycode_type_t::KEYPAD;
  } else if (keyCode >= 97 && keyCode <= 114) {
    return keycode_type_t::GPIO;
  } else {
    return keycode_type_t::UNKNOWN;
  }
}

TCA8418::keycode_type_t TCA8418::mapKeyCodeToArray(uint8_t rawKeyCode,
                                                   uint8_t* outCorrectedCode) const {
  auto type = classifyKeycode(rawKeyCode);

  if (type == keycode_type_t::KEYPAD) {
    // Make 0-based instead of 1 based.
    // We want to map Keycodes 1-80 to 0-79.
    rawKeyCode -= 1;
  } else if (type == keycode_type_t::GPIO) {
    // Make 0-based instead of 1 based.
    // We want to map GPIO codes 97-114 to 80-113.
    // Offset for GPIO keys being another 16 above of the keypad codes
    // in order to stuff into last 2 bytes of keysPushed/Released arrays
    rawKeyCode -= 1;
    rawKeyCode -= 16;
  }

  *outCorrectedCode = rawKeyCode;

  return type;
}

uint8_t TCA8418::readKeyEventsFifo() {
  uint8_t keyLockReg = 0;
  TRY_ERR(readRegister(register_t::KEY_LCK_EC, &keyLockReg));
  uint8_t eventsCount = keyLockReg & 0x0F;

  for (uint8_t i = 0; i < eventsCount; ++i) {
    uint8_t keyEventReg = 0;
    TRY_ERR(readRegister(register_t::KEY_EVENT_A, &keyEventReg));
    pendingEvents[i] = keyEventReg;
  }

  pendingEventsCount = eventsCount;

  return SUCCESS;
}

void TCA8418::updateButtonState(uint8_t pendingEvent) {
  uint8_t rawKeyCode = pendingEvent & 0b0111'1111;
  key_event_type_t eventType = static_cast<key_event_type_t>((pendingEvent & 0b1000'0000) >> 7);

  uint8_t arrayIndex = 0;
  auto type = mapKeyCodeToArray(rawKeyCode, &arrayIndex);
  if (type == keycode_type_t::UNKNOWN) return;

  if (eventType == key_event_type_t::PRESSED) {
    setBit(keysPushed, arrayIndex);
    setBit(keysStillPushed, arrayIndex);
  } else if (eventType == key_event_type_t::RELEASED) {
    clearBit(keysPushed, arrayIndex);
    clearBit(keysStillPushed, arrayIndex);
    setBit(keysReleased, arrayIndex);
  } else {
    // Can never happen
  }
}