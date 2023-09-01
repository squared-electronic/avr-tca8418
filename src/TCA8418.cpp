#include "TCA8418.h"

#include <twi_master.h>

void TCA8418::begin() {
  // POR
}

void TCA8418::setKeys() {
  // Set rows / cols in KP_GPIO.
  // Each to value of 1
  // Any GPIs configured with a 1 in the GPI_EM1-3 Registers will also be part of the event FIFO.
  uint8_t data[] = {0x03, 0x01, 0x05};
  ret_code_t err = tw_master_transmit(0x50, data, sizeof(data), false);
}

uint8_t TCA8418::readRegister(uint8_t address) { tw_master_transmit_one(0x50, 0x50, false); }

void TCA8418::handleInterupt() {
  // Bit 7 value '0' signifies key release, value 1 signifies key press.
}