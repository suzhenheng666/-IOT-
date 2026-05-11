#include "BLDC_FOC.h"
#include <math.h>
#include <string.h>

void PID_Init(PID_Controller *pid, float kp, float ki, float kd, float i_limit, float o_limit) {
    pid->Kp = kp;
    pid->Ki = ki;
    pid->Kd = kd;
    pid->integral = 0.0f;
    pid->prev_error = 0.0f;
    pid->integral_limit = i_limit;
    pid->output_limit = o_limit;
}

float PID_Update(PID_Controller *pid, float setpoint, float measurement, float dt) {
    float error = setpoint - measurement;
    pid->integral += error * dt;

    // Anti-windup: 积分限幅
    if (pid->integral > pid->integral_limit) pid->integral = pid->integral_limit;
    if (pid->integral < -pid->integral_limit) pid->integral = -pid->integral_limit;

    float derivative = (error - pid->prev_error) / dt;
    pid->prev_error = error;

    float output = pid->Kp * error + pid->Ki * pid->integral + pid->Kd * derivative;

    // 输出限幅
    if (output > pid->output_limit) output = pid->output_limit;
    if (output < -pid->output_limit) output = -pid->output_limit;

    return output;
}

#define SQRT3_DIV2  0.8660254f
#define SQRT3_DIV3  0.57735027f

void FOC_Init(FOC_State *foc) {
    memset(foc, 0, sizeof(FOC_State));
}

void FOC_ClarkeTransform(FOC_State *foc) {
    // ia + ib + ic = 0, 只需测两相
    foc->i_alpha = foc->ia;
    foc->i_beta = SQRT3_DIV3 * (foc->ia + 2.0f * foc->ib);
}

void FOC_ParkTransform(FOC_State *foc) {
    float sin_val = sinf(foc->theta);
    float cos_val = cosf(foc->theta);
    foc->i_d = foc->i_alpha * cos_val + foc->i_beta * sin_val;
    foc->i_q = -foc->i_alpha * sin_val + foc->i_beta * cos_val;
}

void FOC_InvParkTransform(FOC_State *foc) {
    float sin_val = sinf(foc->theta);
    float cos_val = cosf(foc->theta);
    foc->v_alpha = foc->v_d * cos_val - foc->v_q * sin_val;
    foc->v_beta  = foc->v_d * sin_val + foc->v_q * cos_val;
}

void FOC_SVPWM(FOC_State *foc, TIM_HandleTypeDef *htim) {
    // SVPWM: αβ → 三相占空比
    float v1 = foc->v_beta;
    float v2 = -0.5f * foc->v_beta + SQRT3_DIV2 * foc->v_alpha;
    float v3 = -0.5f * foc->v_beta - SQRT3_DIV2 * foc->v_alpha;

    float v_offset = 0.5f * V_BUS;
    float ta = (foc->v_alpha + v_offset) / V_BUS;
    float tb = (v2 + v_offset) / V_BUS;
    float tc = (v3 + v_offset) / V_BUS;

    // 限幅
    ta = ta > 0.95f ? 0.95f : (ta < 0.05f ? 0.05f : ta);
    tb = tb > 0.95f ? 0.95f : (tb < 0.05f ? 0.05f : tb);
    tc = tc > 0.95f ? 0.95f : (tc < 0.05f ? 0.05f : tc);

    // 写入 TIM1 CCR
    __HAL_TIM_SET_COMPARE(htim, TIM_CHANNEL_1, (uint32_t)(ta * PWM_PERIOD));
    __HAL_TIM_SET_COMPARE(htim, TIM_CHANNEL_2, (uint32_t)(tb * PWM_PERIOD));
    __HAL_TIM_SET_COMPARE(htim, TIM_CHANNEL_3, (uint32_t)(tc * PWM_PERIOD));
}

void FOC_CurrentLoop(FOC_State *foc, PID_Controller *pid_d, PID_Controller *pid_q, float i_d_ref, float i_q_ref) {
    foc->v_d = PID_Update(pid_d, i_d_ref, foc->i_d, 0.0001f);
    foc->v_q = PID_Update(pid_q, i_q_ref, foc->i_q, 0.0001f);
}

float FOC_ReadSpeed(FOC_State *foc, float dt) {
    static float prev_angle = 0.0f;
    float delta = foc->theta_mech - prev_angle;
    if (delta < -M_PI) delta += 2.0f * M_PI;
    if (delta > M_PI) delta -= 2.0f * M_PI;
    prev_angle = foc->theta_mech;
    foc->speed_rpm = (delta / dt) * 60.0f / (2.0f * M_PI);
    return foc->speed_rpm;
}
