#ifndef PRINT_H__
#define PRINT_H__

#include "main.h"
#include "usart.h"
#include <stdio.h>
#include "cmsis_os.h"
#include "usart.h"

void print_all(uint8_t txData[],uint16_t Size);

void print_pc(uint8_t txData[],uint16_t Size);

#endif
