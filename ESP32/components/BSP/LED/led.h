#ifndef __LED_H__
#define __LED_H__

#include "driver/gpio.h"

#define LED_GPIO_PIN 	GPIO_NUM_1

enum GPIO_OUTPUT_START
{
	PIN_RESET,
	PIN_SET
};

#define LED(X)  do{X?\
					gpio_set_level(LED_GPIO_PIN,PIN_SET):\
					gpio_set_level(LED_GPIO_PIN,PIN_RESET);\
}while(0)

#define LED_TOGGLE()  do{ \
					gpio_set_level(LED_GPIO_PIN,!gpio_get_level(LED_GPIO_PIN));\
}while(0)

void led_init(void);


#endif