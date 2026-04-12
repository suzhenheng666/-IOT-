#ifndef MY_DELAY_US_H__
#define MY_DELAY_US_H__

#include "gpio.h"
#include "main.h"
#include <stdio.h>

extern TIM_HandleTypeDef htim6;

void delay_us(uint16_t us);

#endif
