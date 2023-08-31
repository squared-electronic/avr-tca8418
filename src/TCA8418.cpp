#include "TCA8418.h"

#include <twi_master.h>

void TCA8418::begin() {
  // POR
}

void TCA8418::setKeys() {
  // Set rows / cols in KP_GPIO.
  // Each to value of 1
  uint8_t data[] = {0x03, 0x01, 0x05};
  ret_code_t err = tw_master_transmit(0x50, data, sizeof(data), false);
}

void TCA8418::handleInterupt() {}