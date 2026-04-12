#include "Fan.h"
extern Fan_st Fan_state;

void Fan_Off(void)
{
	HAL_GPIO_WritePin(Fan_GPIO_Port, Fan_Pin, GPIO_PIN_SET);
	Fan_state=fan_off;
}

void Fan_On(void)
{
	HAL_GPIO_WritePin(Fan_GPIO_Port, Fan_Pin, GPIO_PIN_RESET);
	Fan_state=fan_on;
}
