#ifndef _ADC_H_
#define _ADC_H_

#include <stdint.h>

void ADC_init(void);
void ADC_probe(void);
uint16_t ADC_result(void);
#endif
