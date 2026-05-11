% build_foc_model.m
% 构建 foc_simulation.slx — DJI 2312 FOC 速度环仿真
% 使用: run('foc_params.m'); build_foc_model;

function build_foc_model()
    model = 'foc_simulation';

    % 关闭已有模型
    if bdIsLoaded(model), close_system(model, 0); end
    new_system(model);

    % 求解器
    set_param(model, 'Solver', 'ode23tb', ...
        'MaxStep', '1e-4', 'StopTime', '2');

    % ===== 速度参考 =====
    add_block('simulink/Sources/Step', [model '/Step_RPM'], ...
        'Position', [50, 120, 80, 150]);
    set_param([model '/Step_RPM'], ...
        'Time', 'StepTime', 'Before', '0', 'After', 'TargetRPM');

    % ===== RPM → rad/s =====
    add_block('simulink/Math Operations/Gain', [model '/RPM2Rad'], ...
        'Position', [130, 120, 165, 150], 'Gain', '2*pi/60');

    % ===== 速度误差 =====
    add_block('simulink/Math Operations/Sum', [model '/Speed_Error'], ...
        'Position', [220, 115, 245, 155], 'Inputs', '+-');

    % ===== 速度环 PID =====
    add_block('simulink/Continuous/PID Controller', [model '/Speed_PID'], ...
        'Position', [300, 115, 355, 155]);
    set_param([model '/Speed_PID'], ...
        'P', 'speed_pid.Kp', 'I', 'speed_pid.Ki', 'D', 'speed_pid.Kd', ...
        'FilterCoefficient', '100', ...
        'LimitOutput', 'on', ...
        'UpperSaturationLimit', 'speed_pid.SatLim', ...
        'LowerSaturationLimit', '-speed_pid.SatLim');

    % ===== 电气传递函数: 1/(L*s + R) → i_q =====
    add_block('simulink/Continuous/Transfer Fcn', [model '/Electrical_TF'], ...
        'Position', [430, 115, 490, 155]);
    set_param([model '/Electrical_TF'], ...
        'Numerator', '[1]', 'Denominator', '[motor.PhaseL motor.PhaseR]');

    % ===== Kt: i_q → 电磁转矩 =====
    add_block('simulink/Math Operations/Gain', [model '/Kt_Gain'], ...
        'Position', [550, 120, 585, 150], ...
        'Gain', '1.5 * motor.PolePairs * motor.Kt');

    % ===== 负载转矩 =====
    add_block('simulink/Sources/Constant', [model '/LoadTorque'], ...
        'Position', [590, 230, 620, 260], 'Value', '0');

    % ===== 转矩差 (Te - TL) =====
    add_block('simulink/Math Operations/Sum', [model '/Torque_Sum'], ...
        'Position', [630, 170, 655, 220], 'Inputs', '+-');

    % ===== 机械传递函数: 1/(J*s + B) → omega (rad/s) =====
    add_block('simulink/Continuous/Transfer Fcn', [model '/Mechanical_TF'], ...
        'Position', [710, 170, 770, 210]);
    set_param([model '/Mechanical_TF'], ...
        'Numerator', '[1]', 'Denominator', '[motor.J motor.B]');

    % ===== Rad/s → RPM =====
    add_block('simulink/Math Operations/Gain', [model '/Rad2RPM'], ...
        'Position', [830, 175, 865, 205], 'Gain', '60/(2*pi)');

    % ===== 反电动势反馈 (Ke * omega) =====
    add_block('simulink/Math Operations/Gain', [model '/Ke_Gain'], ...
        'Position', [830, 70, 865, 100], 'Gain', 'motor.Ke');

    % ===== 电压误差 (Vq - Bemf) =====
    add_block('simulink/Math Operations/Sum', [model '/Voltage_Sum'], ...
        'Position', [380, 215, 405, 265], 'Inputs', '+-');

    % ===== 示波器 =====
    add_block('simulink/Sinks/Scope', [model '/Speed_Scope'], ...
        'Position', [900, 120, 945, 160]);
    set_param([model '/Speed_Scope'], 'NumInputPorts', '2');
    % 启用数据记录 (供 GUI 读取)
    set_param([model '/Speed_Scope'], 'DataLogging', 'on');
    set_param([model '/Speed_Scope'], 'DataLoggingVariableName', 'SpeedLog');
    set_param([model '/Speed_Scope'], 'DataLoggingLimitDataPoints', '50000');
    set_param([model '/Speed_Scope'], 'DataLoggingDecimateData', 'off');

    add_block('simulink/Sinks/Scope', [model '/Current_Scope'], ...
        'Position', [900, 240, 945, 280]);
    set_param([model '/Current_Scope'], 'DataLogging', 'on');
    set_param([model '/Current_Scope'], 'DataLoggingVariableName', 'CurrentLog');
    set_param([model '/Current_Scope'], 'DataLoggingLimitDataPoints', '50000');
    set_param([model '/Current_Scope'], 'DataLoggingDecimateData', 'off');

    % ===== To Workspace (供 GUI 绘图) =====
    add_block('simulink/Sinks/To Workspace', [model '/sim_time'], ...
        'Position', [900, 50, 945, 80]);
    set_param([model '/sim_time'], 'VariableName', 'sim_time', ...
        'SaveFormat', 'Structure With Time');

    add_block('simulink/Sinks/To Workspace', [model '/sim_rpm'], ...
        'Position', [900, 10, 945, 40]);
    set_param([model '/sim_rpm'], 'VariableName', 'sim_rpm', ...
        'SaveFormat', 'Structure With Time');

    add_block('simulink/Sinks/To Workspace', [model '/sim_iq'], ...
        'Position', [900, 280, 945, 310]);
    set_param([model '/sim_iq'], 'VariableName', 'sim_iq', ...
        'SaveFormat', 'Structure With Time');

    % ===== Clock =====
    add_block('simulink/Sources/Clock', [model '/Clock'], ...
        'Position', [50, 20, 80, 50]);

    % ===== 连线 =====
    % 速度环
    add_line(model, 'Step_RPM/1', 'RPM2Rad/1');
    add_line(model, 'RPM2Rad/1', 'Speed_Error/1');
    add_line(model, 'Speed_Error/1', 'Speed_PID/1');

    % 电压 = Speed_PID 输出 (Vq_ref)
    add_line(model, 'Speed_PID/1', 'Voltage_Sum/1');

    % 反电动势反馈: Mechanical → Ke_Gain → Voltage_Sum(负)
    add_line(model, 'Mechanical_TF/1', 'Ke_Gain/1');

    % 连线 Ke 输出到 Voltage_Sum 的负端
    Ke_port = get_param([model '/Ke_Gain'], 'PortHandles');
    Vsum_port = get_param([model '/Voltage_Sum'], 'PortHandles');
    add_line(model, Ke_port.Outport(1), Vsum_port.Inport(2));

    % Vq → Electrical TF → Kt → Torque_Sum → Mechanical TF
    add_line(model, 'Voltage_Sum/1', 'Electrical_TF/1');
    add_line(model, 'Electrical_TF/1', 'Kt_Gain/1');
    add_line(model, 'Kt_Gain/1', 'Torque_Sum/1');
    add_line(model, 'LoadTorque/1', 'Torque_Sum/2');
    add_line(model, 'Torque_Sum/1', 'Mechanical_TF/1');

    % 反馈: Mechanical ω → Speed_Error (负)
    Mech_port = get_param([model '/Mechanical_TF'], 'PortHandles');
    SpdErr_port = get_param([model '/Speed_Error'], 'PortHandles');
    add_line(model, Mech_port.Outport(1), SpdErr_port.Inport(2));

    % RPM 显示
    add_line(model, 'Mechanical_TF/1', 'Rad2RPM/1');

    % 示波器
    add_line(model, 'Step_RPM/1', 'Speed_Scope/1');
    Rad2RPM_port = get_param([model '/Rad2RPM'], 'PortHandles');
    add_line(model, Rad2RPM_port.Outport(1), 'Speed_Scope/2');

    % Current
    add_line(model, 'Electrical_TF/1', 'Current_Scope/1');

    % To Workspace
    add_line(model, 'Clock/1', 'sim_time/1');
    add_line(model, Rad2RPM_port.Outport(1), 'sim_rpm/1');
    add_line(model, 'Electrical_TF/1', 'sim_iq/1');

    % ===== 模型注释 =====
    add_block('simulink/Annotations/Note', [model '/Note'], ...
        'Position', [400, 20, 600, 100]);
    set_param([model '/Note'], ...
        'FontSize', '12', ...
        'Text', ...
        'FOC 速度环仿真 — DJI 2312\n' + ...
        '电流环(内环)简化为电气TF\n' + ...
        '速度环(外环) = PID控制\n' + ...
        '反电动势: Ke*omega 前馈补偿');

    % 保存
    save_system(model);
    fprintf('=== %s.slx 构建完成 ===\n', model);
    fprintf('\n运行仿真:\n');
    fprintf('  run(''foc_params.m''); sim(''%s'');\n', model);
    fprintf('\n打开 PID 调参界面:\n');
    fprintf('  pid_tune_gui\n');
end
