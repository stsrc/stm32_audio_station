#ifndef _I2C_H_
#define _I2C_H_

#include <stdint.h>

void I2C1_init(void);
void I2C1_write(uint8_t address, uint8_t *value, uint8_t size);
void I2C1_read(uint8_t address, uint8_t *value);

#endif
