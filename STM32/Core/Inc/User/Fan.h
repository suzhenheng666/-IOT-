#ifndef FAN_H__
#define FAN_H__

#include "gpio.h"
#include "main.h"
#include <stdio.h>

typedef enum
{
	fan_off = 0x00,
	fan_on = 0x01,
}Fan_st;

void Fan_On(void);

void Fan_Off(void);

#endif
