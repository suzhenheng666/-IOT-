#ifndef BLDC_FOC_H__
#define BLDC_FOC_H__

#include "main.h"
#include <stdint.h>
#include <math.h>

// --- PID 结构体 ---
typedef struct {
    float Kp;
    float Ki;
    float Kd;
    float integral;
    float prev_error;
    float integral_limit;
    float output_limit;
} PID_Controller;

// --- FOC 状态 ---
typedef struct {
    float ia;          // 相电流 A
    float ib;          // 相电流 B
    float ic;          // 相电流 C
    float i_alpha;     // Clarke: α 轴电流
    float i_beta;      // Clarke: β 轴电流
    float i_d;         // Park: d 轴电流
    float i_q;         // Park: q 轴电流
    float v_d;         // PI 输出 d 轴电压
    float v_q;         // PI 输出 q 轴电压
    float v_alpha;     // 反 Park: α 轴电压
    float v_beta;      // 反 Park: β 轴电压
    float theta;       // 电角度 (rad)
    float theta_mech;  // 机械角度 (rad)
    float speed_rpm;   // 当前转速 RPM
    uint16_t angle_raw;// AS5600 原始角度
} FOC_State;

// --- DJI 2312 参数 ---
#define DJI2312_KV        920.0f   // KV 值
#define DJI2312_POLE_PAIRS 7       // 12N14P → 7 对极
#define DJI2312_PHASE_R    0.12f   // 相电阻 Ω
#define DJI2312_PHASE_L    0.000041f // 相电感 H

#define PWM_PERIOD         8399    // 84MHz / 10kHz PWM = 8400 - 1
#define V_BUS              12.0f   // 母线电压

// --- API ---
void PID_Init(PID_Controller *pid, float kp, float ki, float kd, float i_limit, float o_limit);
float PID_Update(PID_Controller *pid, float setpoint, float measurement, float dt);
void FOC_Init(FOC_State *foc);
void FOC_ClarkeTransform(FOC_State *foc);
void FOC_ParkTransform(FOC_State *foc);
void FOC_InvParkTransform(FOC_State *foc);
void FOC_SVPWM(FOC_State *foc, TIM_HandleTypeDef *htim);
float FOC_ReadSpeed(FOC_State *foc, float dt);
void FOC_CurrentLoop(FOC_State *foc, PID_Controller *pid_d, PID_Controller *pid_q, float i_d_ref, float i_q_ref);

#endif
