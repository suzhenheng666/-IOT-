#include "My_delay_us.h"

void delay_us(uint16_t us) {
    uint16_t start = __HAL_TIM_GET_COUNTER(&htim6); // 获取当前计数值
    // 等待差值达到设定的 us 数。无符号16位减法自带溢出处理。
    while ((uint16_t)(__HAL_TIM_GET_COUNTER(&htim6) - start) < us);
}
