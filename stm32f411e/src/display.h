#ifndef _DISPLAY_H_
#define _DISPLAY_H_
#include <stdint.h>
void display_notify(uint16_t x, uint16_t y, uint16_t z);
void display_task(void *pvParameters);
#endif
