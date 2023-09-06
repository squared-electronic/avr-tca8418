#include "TCA8418.h"

#include <avr/interrupt.h>
#include <twi_master.h>

void TCA8418::begin() {
  // 4 INT_CFG - processor interrupt is deasserted for 50 μs and reassert with pending interrupts
  // 0 KE_IEN - enabled; INT becomes asserted when a key event occurs
  writeRegister(register_t::CFG, 0b0001'0001);
}

void TCA8418::configureKeypad(uint8_t* rows, uint8_t* cols, uint8_t rows_count,
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

  writeRegister(register_t::KP_GPIO1, kpGpio1Reg);
  writeRegister(register_t::KP_GPIO2, kpGpio2Reg);
  writeRegister(register_t::KP_GPIO3, kpGpio3Reg);
}

uint8_t TCA8418::configureGpio(gpio_pin_t* pins, uint8_t pins_count, bool enablePullup) {
  uint8_t error = SUCCESS;
  error = configureGpioInterrupts(pins, pins_count);
  if (error != SUCCESS) {
    return error;
  }
}

uint8_t TCA8418::configureGpioInterrupts(gpio_pin_t* pins, uint8_t pins_count) {
  uint8_t gpioInt1Reg = 0;
  uint8_t gpioInt2Reg = 0;
  uint8_t gpioInt3Reg = 0;

  for (uint8_t i = 0; i < pins_count; ++i) {
    uint8_t pin = static_cast<uint8_t>(pins[i]);
    if (pin < 8) {
      gpioInt1Reg |= (1 << pin);
    } else if (pin < 16) {
      gpioInt2Reg |= (1 << pin);
    } else if (pin < 18) {
      gpioInt3Reg |= (1 << pin);
    }
  }

  uint8_t error = SUCCESS;
  error |= writeRegister(register_t::GPIO_ENG_EN1, gpioInt1Reg);
  error |= writeRegister(register_t::GPIO_ENG_EN2, gpioInt2Reg);
  error |= writeRegister(register_t::GPIO_ENG_EN3, gpioInt3Reg);
  return error;
}

bool TCA8418::wasKeyPressed(uint8_t keyCode) const { return readBit(keysPushed, keyCode); }

bool TCA8418::wasKeyReleased(uint8_t keyCode) const { return readBit(keysReleased, keyCode); }

bool TCA8418::isKeyHeld(uint8_t keyCode) const { return readBit(keysStillPushed, keyCode); }

uint8_t TCA8418::writeRegister(register_t register_address, uint8_t data) {
  uint8_t bytes[2] = {(uint8_t)register_address, data};
  return tw_master_transmit(I2C_ADDRESS, bytes, sizeof(bytes), false);
}

uint8_t TCA8418::readRegister(register_t register_address, uint8_t* out_data) {
  auto error = tw_master_transmit(I2C_ADDRESS, (uint8_t*)register_address, 1, true);
  if (error != SUCCESS) {
    return error;
  }
  error = tw_master_receive(I2C_ADDRESS, out_data, 1);
  return error;
}

uint8_t TCA8418::handleInterupt() {
  // Read INT_STAT to find out what triggered the interrupt
  uint8_t intStatReg = 0;
  auto error = readRegister(register_t::INT_STAT, &intStatReg);
  if (error != SUCCESS) {
    return error;
  }

  // Assume only K_INT for now, TODO support all interrupts
  if ((intStatReg & 0b1111'1110) != 0) {
    return 1;  // An interrupt we don't support happened; leave.
  }

  error = readKeyEventsFifo();
  if (error != SUCCESS) {
    return error;
  }

  // Clear K_INT flag (others left untouched, TODO)
  error = writeRegister(register_t::INT_STAT, intStatReg & ~(1));

  return error;
}

void TCA8418::updateButtonStates() {
  uint8_t sreg = SREG;
  cli();

  for (uint8_t i = 0; i < 8; ++i) {
    keysPushed[i] = 0;
    keysReleased[i] = 0;
  }

  for (uint8_t i = 0; i < pendingEventsCount; ++i) {
    uint8_t keyCode = pendingEvents[i] & 0b0111'1111;
    key_event_type_t eventType = (key_event_type_t)((pendingEvents[i] & 0b1000'0000) >> 7);

    if (eventType == key_event_type_t::PRESSED) {
      setBit(keysPushed, keyCode);
      setBit(keysStillPushed, keyCode);
    } else if (eventType == key_event_type_t::RELEASED) {
      clearBit(keysPushed, keyCode);
      clearBit(keysStillPushed, keyCode);
      setBit(keysReleased, keyCode);
    } else {
      // Can never happen
    }
  }

  SREG = sreg;
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

uint8_t TCA8418::readKeyEventsFifo() {
  uint8_t keyLockReg = 0;
  auto error = readRegister(register_t::KEY_LCK_EC, &keyLockReg);

  if (error != SUCCESS) {
    return error;
  }

  pendingEventsCount = keyLockReg & 0x0F;

  for (uint8_t i = 0; i < pendingEventsCount; ++i) {
    uint8_t keyEventReg = 0;
    error = readRegister(register_t::KEY_EVENT_A, &keyEventReg);

    if (error != SUCCESS) {
      pendingEventsCount = 0;
      return error;
    }

    pendingEvents[i] = keyEventReg;
  }

  return SUCCESS;
}