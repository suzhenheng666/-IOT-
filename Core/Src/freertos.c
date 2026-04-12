/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "DHT11.h"
#include "Fan.h"
#include "print.h"
#include <string.h>
#include "protocol.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
extern Fan_st Fan_state;
/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for dht11Task */
osThreadId_t dht11TaskHandle;
const osThreadAttr_t dht11Task_attributes = {
  .name = "dht11Task",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for fanTask */
osThreadId_t fanTaskHandle;
const osThreadAttr_t fanTask_attributes = {
  .name = "fanTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for uart1TxSem */
osSemaphoreId_t uart1TxSemHandle;
const osSemaphoreAttr_t uart1TxSem_attributes = {
  .name = "uart1TxSem"
};
/* Definitions for uart2TxSem */
osSemaphoreId_t uart2TxSemHandle;
const osSemaphoreAttr_t uart2TxSem_attributes = {
  .name = "uart2TxSem"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void StartDht11Task(void *argument);
void StartFanTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* creation of uart1TxSem */
  uart1TxSemHandle = osSemaphoreNew(1, 1, &uart1TxSem_attributes);

  /* creation of uart2TxSem */
  uart2TxSemHandle = osSemaphoreNew(1, 1, &uart2TxSem_attributes);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of dht11Task */
  dht11TaskHandle = osThreadNew(StartDht11Task, NULL, &dht11Task_attributes);

  /* creation of fanTask */
  fanTaskHandle = osThreadNew(StartFanTask, NULL, &fanTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
	uint8_t MESSAGE[] = "Task Started...\r\n";
	print_pc(MESSAGE,sizeof(MESSAGE)-1);
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_StartDht11Task */
/**
* @brief Function implementing the dht11Task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartDht11Task */
void StartDht11Task(void *argument)
{
  /* USER CODE BEGIN StartDht11Task */
	uint8_t temperature = 0;
	uint8_t temperature_deci=0;
	uint8_t humidity = 0;
	uint8_t humidity_deci=0;
	uint8_t status=0;
	uint8_t tx_buf[50];
	uint16_t len=0;
  /* Infinite loop */
  for(;;)
  {
		osDelay(2500);
		status = DHT11_ReadData(&temperature, &temperature_deci,&humidity,&humidity_deci);
		
    if(status == 0)
    {
				Send_Sensor_Data(temperature,temperature_deci,humidity,humidity_deci,Fan_state);
    }
    else if(status == 1)
    {
				snprintf((char*)tx_buf,sizeof(tx_buf),"DHT11 Not Response!\r\n");
				len=strlen((char*)tx_buf);
        print_pc(tx_buf,len);
    }
    else if(status == 2)
    {
				snprintf((char*)tx_buf,sizeof(tx_buf),"CheckSum Error!\r\n");
				len=strlen((char*)tx_buf);
        print_pc(tx_buf,len);
    }
  }
  /* USER CODE END StartDht11Task */
}

/* USER CODE BEGIN Header_StartFanTask */
/**
* @brief Function implementing the fanTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartFanTask */
void StartFanTask(void *argument)
{
  /* USER CODE BEGIN StartFanTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartFanTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

