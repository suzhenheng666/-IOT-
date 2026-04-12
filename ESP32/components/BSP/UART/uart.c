#include "uart.h"

QueueHandle_t uart0_queue;
QueueHandle_t uart1_queue;

void usart_init(void)
{
	uart_config_t uart_struct;
	uart_struct.baud_rate=115200;
	uart_struct.data_bits=UART_DATA_8_BITS;
	uart_struct.flow_ctrl=UART_HW_FLOWCTRL_DISABLE;
	uart_struct.parity=UART_PARITY_DISABLE;
	uart_struct.rx_flow_ctrl_thresh=122;
	uart_struct.source_clk=UART_SCLK_APB;
	uart_struct.stop_bits=UART_STOP_BITS_1;
	uart_param_config(USART_UX,&uart_struct);

	uart_set_pin(USART_UX,USART_TX_GPIO_NUM,USART_RX_GPIO_NUM,UART_PIN_NO_CHANGE,UART_PIN_NO_CHANGE);

	uart_driver_install(USART_UX,RX_BUF_SIZE*2,RX_BUF_SIZE*2,20,&uart0_queue,0);

	//UART1
	ESP_ERROR_CHECK(uart_param_config(STM_UART_NUM,&uart_struct));

	ESP_ERROR_CHECK(uart_set_pin(STM_UART_NUM,STM_TXD_PIN,STM_RXD_PIN,UART_PIN_NO_CHANGE,UART_PIN_NO_CHANGE));

	ESP_ERROR_CHECK(uart_driver_install(STM_UART_NUM,RX_BUF_SIZE*2,RX_BUF_SIZE*2,20,&uart1_queue,0));

	
}