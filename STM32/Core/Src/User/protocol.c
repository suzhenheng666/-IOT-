#include "protocol.h"
#include <string.h>

uint8_t Calc_Checksum(uint8_t *data, uint16_t length) {
    uint8_t sum = 0;
    for (uint16_t i = 0; i < length; i++) {
        sum += data[i];
    }
    return sum;
}

void Send_Sensor_Data(uint8_t t_i, uint8_t t_d, uint8_t h_i, uint8_t h_d, uint16_t fan_spd) {
    DataFrame_t frame;
    frame.header1 = FRAME_HEADER1;
    frame.header2 = FRAME_HEADER2;
    frame.cmd = CMD_SENSOR_REPORT;
    frame.len = sizeof(SensorPayload_t);
    frame.payload.temp_int = t_i;
    frame.payload.temp_dec = t_d;
    frame.payload.humi_int = h_i;
    frame.payload.humi_dec = h_d;
    frame.payload.fan_speed = fan_spd;

    uint8_t *calc_start = (uint8_t*)&frame.cmd;
    uint8_t calc_len = 1 + 1 + sizeof(SensorPayload_t);
    frame.checksum = Calc_Checksum(calc_start, calc_len);

    HAL_UART_Transmit(&huart1, (uint8_t*)&frame, sizeof(DataFrame_t), 100);
}
