#include "TCA8418.h"

#include <twi_master.h>

void TCA8418::begin() {
  // 7 AI Auto-increment for read and write operations; See below table for more information
  // 4 INT_CFG - processor interrupt is deasserted for 50 μs and reassert with pending interrupts
  // 0 KE_IEN - enabled; INT becomes asserted when a key event occurs
  writeRegister(register_t::CFG, 0b1001'0001);
}

void TCA8418::configureKeys() {
  // Set rows / cols in KP_GPIO.
  // Each to value of 1
  // Any GPIs configured with a 1 in the GPI_EM1-3 Registers will also be part of the event FIFO.
  // uint8_t data[] = {0x03, 0x01, 0x05};
  // ret_code_t err = tw_master_transmit(0x50, data, sizeof(data), false);
}

bool TCA8418::wasKeyPressed(uint8_t keyCode) const { return readBit(keysPushed, keyCode); }

bool TCA8418::wasKeyReleased(uint8_t keyCode) const { return readBit(keysReleased, keyCode); }

bool TCA8418::isKeyHeld(uint8_t keyCode) const { return readBit(keysStillPushed, keyCode); }

ret_code_t TCA8418::writeRegister(register_t register_address, uint8_t data) {
  uint8_t bytes[2] = {(uint8_t)register_address, data};
  return tw_master_transmit(I2C_ADDRESS, bytes, sizeof(bytes), false);
}

ret_code_t TCA8418::readRegister(register_t register_address, uint8_t* out_data) {
  auto error = tw_master_transmit_one(I2C_ADDRESS, (uint8_t)register_address, true);
  if (error != SUCCESS) {
    return error;
  }
  error = tw_master_receive(I2C_ADDRESS, out_data, 1);
  return error;
}

ret_code_t TCA8418::handleInterupt() {
  // Read INT_STAT to find out what triggered the interrupt
  uint8_t intStatReg = 0;
  auto error = readRegister(register_t::INT_STAT, &intStatReg);
  if (error != SUCCESS) {
    return error;
  }

  // Assume only K_INT for now TODO support all interrupts
  if ((intStatReg & 0b1111'1110) != 0) {
    return SUCCESS;  // An interrupt we don't support happened; ignore
  }

  uint8_t fifo[10];
  uint8_t fifoCount = 0;
  error = readKeyEvents(fifo, &fifoCount);
  if (error != SUCCESS) {
    return error;
  }

  for (int i = 0; i < fifoCount; ++i) {
    uint8_t keyCode = fifo[i] & 0b0111'1111;
    key_event_type_t eventType = (key_event_type_t)(fifo[i] & 0b1000'0000);

    uint8_t byteIndex = keyCode / 8;
    uint8_t bitIndex = keyCode % 8;

    if (eventType == key_event_type_t::PRESSED) {
      keysPushed[byteIndex] |= (1 << bitIndex);
    } else if (eventType == key_event_type_t::RELEASED) {
      keysPushed[byteIndex] &= ~(1 << bitIndex);
      keysReleased[byteIndex] |= (1 << bitIndex);
    } else {
      // Can never happen
      return 1;
    }
  }

  // Clear K_INT flag (others left untouched, TODO)
  auto error = writeRegister(register_t::INT_STAT, intStatReg & ~(1));

  if (error != SUCCESS) {
    return error;
  }
}

void TCA8418::updateButtonStates() {
  for (int i = 0; i < 8; ++i) {
    keysPushed[i] &= ~(keysReleased[i]);
    keysStillPushed[i] = keysPushed[i];
    keysPushed[i] = 0;
    keysReleased[i] = 0;
  }
}

uint8_t TCA8418::readBit(const uint8_t* bytes, uint8_t bitNumber) const {
  uint8_t byteIndex = bitNumber / 8;
  uint8_t bitInByteIndex = bitNumber % 8;
  return bytes[byteIndex] & (1 << bitInByteIndex);
}

ret_code_t TCA8418::readKeyEvents(uint8_t* fifo_out, uint8_t* fifo_items) {
  uint8_t keyEventReg = 0;
  auto error = readRegister(register_t::KEY_LCK_EC, &keyEventReg);

  if (error != SUCCESS) {
    return error;
  }

  *fifo_items = keyEventReg & 0x0F;

  for (uint8_t i = 0; i < *fifo_items; ++i) {
    uint8_t keyEventReg = 0;
    error = readRegister(register_t::KEY_EVENT_A, &keyEventReg);

    if (error != SUCCESS) {
      *fifo_items = 0;
      return error;
    }

    fifo_out[i] = keyEventReg;
  }
}