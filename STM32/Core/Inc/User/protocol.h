#ifndef PROTOCOL_H__
#define PROTOCOL_H__

#include <stdint.h>
#include "print.h"
#include "usart.h"

typedef struct {
    uint8_t cmd;      // 控制命令 (比如 0x01 手动开)
    uint8_t value;    // 附带数值 (比如阈值 30)
} CloudCmd_t;

//1 字节紧密排列
#pragma pack(push, 1)
typedef struct
{
		uint8_t temp_int;
		uint8_t temp_dec;
		uint8_t humi_int;
		uint8_t humi_dec;
		uint8_t fan_state;
}SensorPayload_t;

typedef struct
{
		uint8_t  header1;   // 固定 0xA5
    uint8_t  header2;   // 固定 0x5A
		uint8_t  cmd;
		uint8_t  len;    //Payload长度
		SensorPayload_t payload;	//有效数据 (这里先写死为传感器的结构体，后期可以用 union 兼容多种数据)
		uint8_t  checksum; //校验和
}DataFrame_t;
#pragma pack(pop)

void Send_Sensor_Data(uint8_t t_i, uint8_t t_d, uint8_t h_i, uint8_t h_d, uint8_t fan_st);
uint8_t Calc_Checksum(uint8_t *data, uint16_t length);

#endif
