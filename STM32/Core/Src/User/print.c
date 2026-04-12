#include "print.h"
extern osSemaphoreId_t uart2TxSemHandle;
extern osSemaphoreId_t uart1TxSemHandle;

void print_all(uint8_t txData[],uint16_t Size)
{
	HAL_UART_Transmit_DMA(&huart1, txData, Size);
	osSemaphoreAcquire(uart1TxSemHandle, osWaitForever);
	HAL_UART_Transmit_DMA(&huart2, txData, Size);
	osSemaphoreAcquire(uart2TxSemHandle, osWaitForever);
}

void print_pc(uint8_t txData[],uint16_t Size)
{
	HAL_UART_Transmit_DMA(&huart1, txData, Size);
	osSemaphoreAcquire(uart1TxSemHandle, osWaitForever);
}
