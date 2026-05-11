#include "AS5600.h"

static I2C_HandleTypeDef *as5600_i2c;

uint8_t AS5600_Init(I2C_HandleTypeDef *hi2c) {
    as5600_i2c = hi2c;
    uint8_t status;
    if (HAL_I2C_Mem_Read(hi2c, AS5600_I2C_ADDR << 1,
                         AS5600_REG_STATUS, I2C_MEMADD_SIZE_8BIT,
                         &status, 1, 10) != HAL_OK) {
        return 1;
    }
    // 检查磁铁检测位 (bit 3)
    if (!(status & 0x08)) {
        return 2;  // 磁铁未检测到
    }
    return 0;
}

uint16_t AS5600_ReadAngle(I2C_HandleTypeDef *hi2c) {
    uint8_t buf[2];
    HAL_I2C_Mem_Read(hi2c, AS5600_I2C_ADDR << 1,
                     AS5600_REG_ANGLE_H, I2C_MEMADD_SIZE_8BIT,
                     buf, 2, 10);
    return ((uint16_t)buf[0] << 8) | buf[1];
}

uint8_t AS5600_ReadMagnitude(I2C_HandleTypeDef *hi2c) {
    uint8_t val;
    HAL_I2C_Mem_Read(hi2c, AS5600_I2C_ADDR << 1,
                     0x1B, I2C_MEMADD_SIZE_8BIT,
                     &val, 1, 10);
    return val;
}
