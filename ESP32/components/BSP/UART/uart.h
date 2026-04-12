#ifndef __UART_H__
#define __UART_H__

#include "driver/uart.h"
#include "driver/gpio.h"

#define USART_UX		UART_NUM_0
#define USART_TX_GPIO_NUM		GPIO_NUM_43
#define USART_RX_GPIO_NUM		GPIO_NUM_44

#define STM_UART_NUM   UART_NUM_1
#define STM_TXD_PIN    (GPIO_NUM_18) 
#define STM_RXD_PIN    (GPIO_NUM_17)

#define RX_BUF_SIZE		1024

extern QueueHandle_t uart0_queue;
extern QueueHandle_t uart1_queue;

void usart_init(void);

#endif