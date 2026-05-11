# FOC PID 仿真调参 — DJI 2312 + DRV8301

## 文件

| 文件 | 说明 |
|------|------|
| `foc_params.m` | 电机 & 驱动器参数初始化 |
| `build_foc_model.m` | 程序化构建 foc_simulation.slx |
| `pid_tune_gui.m` | PID 调参交互界面 |
| `foc_simulation.slx` | 构建产物 — (build_foc_model 生成) |

## 快速开始

```matlab
% 1. 加载参数
run('foc_params.m');

% 2. 构建 Simulink 模型 (只需运行一次)
build_foc_model;

% 3. 打开 GUI 调参
pid_tune_gui;
```

## 模型结构

```
Step(RPM) → RPM2Rad → Speed_Error → Speed_PID → Voltage_Sum → Electrical_TF(1/(Ls+R))
                ↑                         |              ↑                |
                |                     Vq_ref     Ke*omega (补偿)     i_q → Kt → Te
                |                                                             |
                └── Mechanical_TF(1/(Js+B)) ← Torque_Sum ← ───────┘
                        ↓
                    Rad2RPM → Scope / To Workspace
```

- **速度环**: PID 控制，输入 RPM 误差，输出 i_q 参考
- **反电动势前馈**: Ke × omega 补偿，抵消电机反电动势影响
- **电流环**: 简化为电气传递函数 1/(Ls+R)
- **机械模型**: 1/(Js+B) 从转矩到角速度

## GUI 说明

| 滑条 | 范围 | 说明 |
|------|------|------|
| 速度 Kp | 0–5 | 比例增益 — 增大加快响应 |
| 速度 Ki | 0–20 | 积分增益 — 消除稳态误差 |
| 速度 Kd | 0–1 | 微分增益 — 抑制超调 |
| 电流 Kp | 0–10 | (预留，当前模型简化) |
| 电流 Ki | 0–200 | (预留，当前模型简化) |
| 目标 RPM | 500–8000 | DJI 2312 配 12V 最高约 8000 |
| 仿真时长 | 0.5–5s | 阶跃在 t=0.5s 发生 |

## 参数导出到 C 代码

调好后将 PID 值复制到 STM32 固件:
- `STM32/Core/Inc/User/BLDC_FOC.h` — PID_Controller 结构体
- `STM32/Core/Src/freertos.c` — speed_pid / pid_d / pid_q 初始化

## 电机参数 (DJI 2312)

| 参数 | 值 |
|------|-----|
| KV | 920 RPM/V |
| 极对数 | 7 (12N14P) |
| 相电阻 | 0.12 Ω |
| 相电感 | 41 µH |
| 母线电压 | 12V |
| PWM 频率 | 10 kHz |
