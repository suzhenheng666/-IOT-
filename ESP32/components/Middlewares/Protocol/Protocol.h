#ifndef PROTOCOL_H__
#define PROTOCOL_H__

#include "driver/gpio.h"

#define CMD_SENSOR_REPORT  0x00
#define FRAME_HEADER1  0xA5
#define FRAME_HEADER2  0x5A

#pragma pack(push, 1)
typedef struct {
    uint8_t  temp_int;
    uint8_t  temp_dec;
    uint8_t  humi_int;
    uint8_t  humi_dec;
    uint16_t fan_speed;
} SensorPayload_t;

typedef struct {
    uint8_t  header1;
    uint8_t  header2;
    uint8_t  cmd;
    uint8_t  len;
    SensorPayload_t payload;
    uint8_t  checksum;
} DataFrame_t;
#pragma pack(pop)

uint8_t Calc_Checksum(uint8_t *data, uint16_t length);

#endif
