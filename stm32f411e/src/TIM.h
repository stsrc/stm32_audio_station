#ifndef _TIM_H_
#define _TIM_H_
#include <stdint.h>
void TIM2_init(void);
void TIM2_delay_us(uint32_t us);
void TIM2_deinit(void);
#endif
