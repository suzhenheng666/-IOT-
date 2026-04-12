#ifndef DHT11_H__
#define DHT11_H__

#include "gpio.h"
#include "main.h"
#include <stdio.h>
#include "My_delay_us.h"
#include "cmsis_os.h"

uint8_t DHT11_ReadData(uint8_t *temp,uint8_t *temperature_deci, uint8_t *humi,uint8_t *humidity_deci);

#endif
