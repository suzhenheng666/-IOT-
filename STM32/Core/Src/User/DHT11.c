#include "DHT11.h"

#define DHT11_OUT_H() HAL_GPIO_WritePin(DHT11_GPIO_Port,DHT11_Pin,GPIO_PIN_SET)
#define DHT11_OUT_L()   HAL_GPIO_WritePin(DHT11_GPIO_Port, DHT11_Pin, GPIO_PIN_RESET)
#define DHT11_IN()      HAL_GPIO_ReadPin(DHT11_GPIO_Port, DHT11_Pin)


//输出模式
void DHT11_SetOutputMode(void)
{
	GPIO_InitTypeDef dht11_struct={0};
	dht11_struct.Mode=GPIO_MODE_OUTPUT_PP;
	dht11_struct.Pin=DHT11_Pin;
	dht11_struct.Speed=GPIO_SPEED_FREQ_HIGH;
	dht11_struct.Pull=GPIO_NOPULL;
	HAL_GPIO_Init(DHT11_GPIO_Port,&dht11_struct);
}

//输入模式
void DHT11_SetInputMode(void)
{
  GPIO_InitTypeDef dht11_struct={0};
	dht11_struct.Mode=GPIO_MODE_INPUT;
	dht11_struct.Pin=DHT11_Pin;
	dht11_struct.Speed=GPIO_SPEED_FREQ_HIGH;
	dht11_struct.Pull=GPIO_PULLUP;
	HAL_GPIO_Init(DHT11_GPIO_Port,&dht11_struct);
}

uint8_t DHT11_Init(void)
{
	uint16_t timeout=0;
	DHT11_OUT_H();
	delay_us(13);
	
	DHT11_SetInputMode();
	while(DHT11_IN()==GPIO_PIN_SET)
	{
		delay_us(1);
		timeout++;
		if(timeout>35)
		{
			return 1;
		}
	}
	
	timeout=0;
	while(DHT11_IN()==GPIO_PIN_RESET)
	{
		delay_us(1);
		timeout++;
		if(timeout>88)
		{
			return 1;
		}
	}
	
	timeout=0;
	while(DHT11_IN()==GPIO_PIN_SET)
	{
		delay_us(1);
		timeout++;
		if(timeout>92)
		{
			return 1;
		}
	}
	return 0;
}

uint8_t DHT11_ReadByte(void)
{
    uint8_t data = 0;
    uint16_t timeout;
    uint16_t high_time; 
    
    for(uint8_t j = 0; j < 8; j++)
    {
        timeout = 0;
        // 1. 等待低电平结束
        while(DHT11_IN() == 0)
        {
            delay_us(1);
            timeout++;
            if(timeout > 100) return 0; 
        }
        
        high_time = 0;
        // 2. 核心大招：测量高电平维持的时间！
        while(DHT11_IN() == 1)
        {
            delay_us(1);
            high_time++;
            if(high_time > 100) break; 
        }
        
        // 3. 大于 40us 就是 '1'，极其稳健！
        if(high_time > 40)
        {
            data |= (1 << (7 - j));  
        }
    }
    return data;
}

uint8_t DHT11_ReadData(uint8_t *temp,uint8_t *temperature_deci, uint8_t *humi,uint8_t *humidity_deci)
{
	uint8_t buf[5];
	uint8_t checksum;
	uint8_t tmp=0;
	DHT11_SetOutputMode();
	DHT11_OUT_L();
	osDelay(18);
	taskENTER_CRITICAL();
	
	if(DHT11_Init())
	{
		taskEXIT_CRITICAL();
		return 1;
	}
	for(uint8_t i=0;i<5;i++)
	{
		buf[i]=DHT11_ReadByte();
	}
	taskEXIT_CRITICAL();
	checksum=buf[0]+buf[1]+buf[2]+buf[3];
	if(checksum != buf[4])
   {
        return 2;  // 校验错误
   }
	*humi=buf[0];
	*humidity_deci=buf[1];
	*temp=buf[2];
	 tmp=buf[3];
	*humidity_deci=(tmp&0x7f);
	if(buf[3] & 0x80)
    {
        *temp = -*temp;  // 温度值为负
    }
		return 0;
}
