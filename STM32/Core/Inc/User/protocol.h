#ifndef PROTOCOL_H__
#define PROTOCOL_H__

#include <stdint.h>
#include "print.h"
#include "usart.h"

// 传感器上报帧类型
#define CMD_SENSOR_REPORT  0x00

// 帧头常量
#define FRAME_HEADER1  0xA5
#define FRAME_HEADER2  0x5A

// 1 字节紧密排列
#pragma pack(push, 1)
typedef struct {
    uint8_t  temp_int;
    uint8_t  temp_dec;
    uint8_t  humi_int;
    uint8_t  humi_dec;
    uint16_t fan_speed;   // RPM, 原 fan_state(1B) → fan_speed(2B)
} SensorPayload_t;

typedef struct {
    uint8_t  header1;   // 固定 0xA5
    uint8_t  header2;   // 固定 0x5A
    uint8_t  cmd;       // 0x00 = 传感器上报
    uint8_t  len;       // Payload 长度 = sizeof(SensorPayload_t)
    SensorPayload_t payload;
    uint8_t  checksum;  // 校验和：从 cmd 到 payload 末尾
} DataFrame_t;
#pragma pack(pop)

void Send_Sensor_Data(uint8_t t_i, uint8_t t_d, uint8_t h_i, uint8_t h_d, uint16_t fan_spd);
uint8_t Calc_Checksum(uint8_t *data, uint16_t length);

#endif
