#ifndef AS5600_H__
#define AS5600_H__

#include "main.h"
#include <stdint.h>

#define AS5600_I2C_ADDR  0x36  // 7-bit address
#define AS5600_REG_ANGLE_H  0x0E
#define AS5600_REG_ANGLE_L  0x0F
#define AS5600_REG_STATUS   0x0B
#define AS5600_REG_CONF     0x07

// 初始化磁编码器 I2C
uint8_t AS5600_Init(I2C_HandleTypeDef *hi2c);

// 读取原始角度 (0-4095, 对应 0-360°)
uint16_t AS5600_ReadAngle(I2C_HandleTypeDef *hi2c);

// 读取磁场强度，检测磁铁是否在位
uint8_t AS5600_ReadMagnitude(I2C_HandleTypeDef *hi2c);

#endif
