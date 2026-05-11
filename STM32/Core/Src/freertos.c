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
#include "print.h"
#include "BLDC_FOC.h"
#include "AS5600.h"
#include <string.h>
#include "protocol.h"
#include "queue.h"
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
float current_temp=0;
uint8_t temperature = 0;
uint8_t temperature_deci=0;

// FOC 相关
extern I2C_HandleTypeDef hi2c1;
extern TIM_HandleTypeDef htim1;

FOC_State foc_state;
PID_Controller speed_pid;
PID_Controller pid_d;
PID_Controller pid_q;

// === 温度-PID 控制 ===
// 温度越高 → 风扇越快，把温度压在阈值以下
#define TEMP_THRESHOLD  35.0f   // 目标温度上限 (°C)
#define MAX_FAN_RPM     5000.0f // 风扇最大转速
#define MIN_FAN_RPM     800.0f  // 风扇起步转速 (低于此值无有效风量)

PID_Controller temp_pid;        // 温度→转速 PID

float target_speed_rpm = 0.0f;  // 动态目标，由温度 PID 实时计算
uint16_t current_fan_speed = 0;
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
/* Definitions for focTask */
osThreadId_t focTaskHandle;
const osThreadAttr_t focTask_attributes = {
  .name = "focTask",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityHigh,
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
void StartFocTask(void *argument);

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

  /* Create the queue(s) */
  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of dht11Task */
  dht11TaskHandle = osThreadNew(StartDht11Task, NULL, &dht11Task_attributes);

  /* creation of focTask */
  focTaskHandle = osThreadNew(StartFocTask, NULL, &focTask_attributes);

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
	uint8_t humidity = 0;
	uint8_t humidity_deci=0;
	uint8_t status=0;
	uint8_t tx_buf[50];
	uint16_t len=0;
  /* Infinite loop */
  for(;;)
  {
		osDelay(2500);
		status = DHT11_ReadData(&temperature, &temperature_deci, &humidity, &humidity_deci);
		current_temp = temperature > 0 ? temperature + temperature_deci / 10.0f
									: temperature - temperature_deci / 10.0f;

    // === 温度→风扇转速 PID 控制 ===
    // 误差=当前温度-阈值, 超温时为正→PID输出正→风扇加速
    if (current_temp > TEMP_THRESHOLD - 5.0f) {
        target_speed_rpm = PID_Update(&temp_pid, current_temp, TEMP_THRESHOLD, 2.5f);
        if (target_speed_rpm < 0.0f) target_speed_rpm = 0.0f;
        if (target_speed_rpm > MAX_FAN_RPM) target_speed_rpm = MAX_FAN_RPM;
    } else {
        temp_pid.integral = 0.0f;  // 远低于阈值: 清零积分, 防止重启动时过冲
        target_speed_rpm = 0.0f;
    }


    if (status == 0) {
				Send_Sensor_Data(temperature, temperature_deci, humidity, humidity_deci,
						             current_fan_speed);
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

/* USER CODE BEGIN Header_StartFocTask */
/**
* @brief Function implementing the focTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartFocTask */
void StartFocTask(void *argument)
{
  /* USER CODE BEGIN StartFocTask */
    AS5600_Init(&hi2c1);
    FOC_Init(&foc_state);

    // 温度 PID：Kp=500 (每°C超温+500RPM), Ki=50, Kd=100 (温度上升快时提前加速)
    PID_Init(&temp_pid, 500.0f, 50.0f, 100.0f, 2000.0f, MAX_FAN_RPM);

    PID_Init(&speed_pid, 0.5f, 2.0f, 0.0f, 1000.0f, V_BUS * 0.9f);
    PID_Init(&pid_d, 1.0f, 50.0f, 0.0f, V_BUS, V_BUS);
    PID_Init(&pid_q, 1.0f, 50.0f, 0.0f, V_BUS, V_BUS);

    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);

    TickType_t last_wake = xTaskGetTickCount();

    for(;;) {
        foc_state.angle_raw = AS5600_ReadAngle(&hi2c1);
        foc_state.theta_mech = (float)foc_state.angle_raw / 4096.0f * 2.0f * M_PI;
        foc_state.theta = foc_state.theta_mech * DJI2312_POLE_PAIRS;

        // 读电流 (DRV8301 CSA via ADC -- 硬件对接时取消注释)
        // foc_state.ia = ADC_ReadPhaseA();
        // foc_state.ib = ADC_ReadPhaseB();
        // foc_state.ic = -foc_state.ia - foc_state.ib;

        FOC_ClarkeTransform(&foc_state);
        FOC_ParkTransform(&foc_state);

        float i_q_ref = PID_Update(&speed_pid, target_speed_rpm, foc_state.speed_rpm, 0.001f);
        FOC_CurrentLoop(&foc_state, &pid_d, &pid_q, 0.0f, i_q_ref);

        FOC_InvParkTransform(&foc_state);
        FOC_SVPWM(&foc_state, &htim1);

        FOC_ReadSpeed(&foc_state, 0.001f);
        current_fan_speed = (uint16_t)foc_state.speed_rpm;

        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(1));
    }
  /* USER CODE END StartFocTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

