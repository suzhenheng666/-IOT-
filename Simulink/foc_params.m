% foc_params.m — DJI 2312 + DRV8301 FOC 电机参数初始化

% === 电机参数 ===
motor.KV = 920;                    % RPM/V
motor.PolePairs = 7;               % 12N14P = 7 对极
motor.PhaseR = 0.12;               % 相电阻 (Ω)
motor.PhaseL = 0.000041;           % 相电感 (H)
motor.J = 0.000002;                % 转动惯量 (kg·m²) — 估算值
motor.B = 0.000001;                % 阻尼系数 — 估算值
motor.Kt = 60 / (2 * pi * motor.KV); % 转矩常数 (Nm/A)
motor.Ke = 1 / motor.KV * 60 / (2 * pi); % 反电动势常数 (Vs/rad)

% === DRV8301 参数 ===
driver.Vbus = 12;                  % 母线电压 (V)
driver.PWM_freq = 10000;           % PWM 频率 (Hz)
driver.DeadTime = 2e-6;            % 死区时间 (s)
driver.CurrentGain = 10;           % CSA 增益 (V/A)

% === 速度环 PID 初始值 ===
speed_pid.Kp = 0.5;
speed_pid.Ki = 2.0;
speed_pid.Kd = 0.0;
speed_pid.SatLim = 10;             % 输出饱和 (A)

% === 电流环 PI 初始值 ===
current_pid.Kp = 1.0;
current_pid.Ki = 50.0;
current_pid.SatLim = 12;           % 输出饱和 (V)

% === 仿真设置 ===
sim.Ts = 1e-4;                     % 仿真步长 10kHz
sim.Tfinal = 2;                    % 仿真时长 (s)
StepTime = 0.5;                    % 阶跃时间 (s)
TargetRPM = 3000;                  % 目标转速

disp('DJI 2312 FOC 参数已加载');
fprintf('Kt = %.6f Nm/A, Ke = %.6f Vs/rad\n', motor.Kt, motor.Ke);
