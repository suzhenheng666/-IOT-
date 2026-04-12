#ifndef PROTOCOL_H__
#define PROTOCOL_H__

#include "driver/gpio.h"

#pragma pack(push, 1) // 强制单字节对齐
typedef struct {
    uint8_t temp_int;
    uint8_t temp_dec;
    uint8_t humi_int;
    uint8_t humi_dec;
    uint8_t fan_state;
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