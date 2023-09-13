/*
 * twi_master.c
 *
 * Created: 09-Jun-19 11:20:17 AM
 *  Author: TEP SOVICHEA
 */

#include "twi_master.h"

#define TW_SLA_W(ADDR) ((ADDR << 1) | TW_WRITE)
#define TW_SLA_R(ADDR) ((ADDR << 1) | TW_READ)
#define TW_READ_ACK 1
#define TW_READ_NACK 0

static ret_code_t tw_start(void) {
  /* Send START condition */
#if DEBUG_LOG
  printf(BG "Send START condition..." RESET);
#endif
  TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTA);

  /* Wait for TWINT flag to set */
  while (!(TWCR & (1 << TWINT)))
    ;

  /* Check error */
  if (TW_STATUS != TW_START && TW_STATUS != TW_REP_START) {
#if DEBUG_LOG
    printf("\n");
#endif
    return TW_STATUS;
  }

#if DEBUG_LOG
  printf("SUCCESS\n");
#endif
  return SUCCESS;
}

static void tw_stop(void) {
  /* Send STOP condition */
#if DEBUG_LOG
  puts(BG "Send STOP condition." RESET);
#endif
  TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);
}

static ret_code_t tw_write_sla(uint8_t sla) {
  /* Transmit slave address with read/write flag */
#if DEBUG_LOG
  printf(BG "Write SLA + R/W: 0x%02X..." RESET, sla);
#endif
  TWDR = sla;
  TWCR = (1 << TWINT) | (1 << TWEN);

  /* Wait for TWINT flag to set */
  while (!(TWCR & (1 << TWINT)))
    ;
  if (TW_STATUS != TW_MT_SLA_ACK && TW_STATUS != TW_MR_SLA_ACK) {
#if DEBUG_LOG
    printf("\n");
#endif
    return TW_STATUS;
  }

#if DEBUG_LOG
  printf("SUCCESS\n");
#endif
  return SUCCESS;
}

static ret_code_t tw_write(uint8_t data) {
  /* Transmit 1 byte*/
#if DEBUG_LOG
  printf(BG "Write data byte: 0x%02X..." RESET, data);
#endif
  TWDR = data;
  TWCR = (1 << TWINT) | (1 << TWEN);

  /* Wait for TWINT flag to set */
  while (!(TWCR & (1 << TWINT)))
    ;
  if (TW_STATUS != TW_MT_DATA_ACK) {
#if DEBUG_LOG
    printf("\n");
#endif
    return TW_STATUS;
  }

#if DEBUG_LOG
  printf("SUCCESS\n");
#endif
  return SUCCESS;
}

static uint8_t tw_read(bool read_ack) {
  if (read_ack) {
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWEA);
    while (!(TWCR & (1 << TWINT)))
      ;
    if (TW_STATUS != TW_MR_DATA_ACK) {
      return TW_STATUS;
    }
  } else {
    TWCR = (1 << TWINT) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT)))
      ;
    if (TW_STATUS != TW_MR_DATA_NACK) {
      return TW_STATUS;
    }
  }
  uint8_t data = TWDR;
#if DEBUG_LOG
  printf(BG "Read data byte: 0x%02X\n" RESET, data);
#endif
  return data;
}

ret_code_t tw_master_transmit(uint8_t slave_addr, uint8_t* p_data, uint8_t len, bool repeat_start) {
  ret_code_t error_code;

  /* Send START condition */
  error_code = tw_start();
  if (error_code != SUCCESS) {
    return error_code;
  }

  /* Send slave address with WRITE flag */
  error_code = tw_write_sla(TW_SLA_W(slave_addr));
  if (error_code != SUCCESS) {
    return error_code;
  }

  /* Send data byte in single or burst mode */
  for (int i = 0; i < len; ++i) {
    error_code = tw_write(p_data[i]);
    if (error_code != SUCCESS) {
      return error_code;
    }
  }

  if (!repeat_start) {
    /* Send STOP condition */
    tw_stop();
  }

  return SUCCESS;
}

ret_code_t tw_master_transmit_one(uint8_t slave_addr, uint8_t data, bool repeat_start) {
  uint8_t data_bytes[] = {data};
  return tw_master_transmit(slave_addr, data_bytes, 1, repeat_start);
}

ret_code_t tw_master_receive(uint8_t slave_addr, uint8_t* p_data, uint8_t len) {
  ret_code_t error_code;

  /* Send START condition */
  error_code = tw_start();
  if (error_code != SUCCESS) {
    return error_code;
  }

  /* Write slave address with READ flag */
  error_code = tw_write_sla(TW_SLA_R(slave_addr));
  if (error_code != SUCCESS) {
    return error_code;
  }

  /* Read single or multiple data byte and send ack */
  for (int i = 0; i < len - 1; ++i) {
    p_data[i] = tw_read(TW_READ_ACK);
  }
  p_data[len - 1] = tw_read(TW_READ_NACK);

  /* Send STOP condition */
  tw_stop();

  return SUCCESS;
}
