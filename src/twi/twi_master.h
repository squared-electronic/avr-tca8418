/*
 * twi_master.h
 *
 * Created: 09-Jun-19 11:20:04 AM
 *  Author: TEP SOVICHEA
 */

#ifndef TWI_MASTER_H_
#define TWI_MASTER_H_

#include <avr/io.h>
#include <stdbool.h>
#include <util/twi.h>

#define DEBUG_LOG 0
#define SUCCESS 0

typedef uint8_t ret_code_t;

#ifdef __cplusplus
extern "C" {
#endif

ret_code_t tw_master_transmit(uint8_t slave_addr, uint8_t* p_data, uint8_t len, bool repeat_start);
ret_code_t tw_master_transmit_one(uint8_t slave_addr, uint8_t data, bool repeat_start);
ret_code_t tw_master_receive(uint8_t slave_addr, uint8_t* p_data, uint8_t len);

#ifdef __cplusplus
}
#endif

#endif /* TWI_MASTER_H_ */