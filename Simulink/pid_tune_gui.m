function pid_tune_gui()
% pid_tune_gui — FOC PID 调参交互界面 (DJI 2312)
% 前置: run('foc_params.m'); build_foc_model;

    % 检查模型是否存在
    if ~bdIsLoaded('foc_simulation')
        try
            load_system('foc_simulation');
        catch
            uialert(uifigure(), '模型不存在，请先运行 build_foc_model', '错误');
            return;
        end
    end

    fig = uifigure('Name', 'FOC PID Tuner — DJI 2312', ...
                   'Position', [100 100 900 700]);

    % ===== 速度环参数 =====
    uilabel(fig, 'Text', '速度环 PID', 'Position', [30 650 200 22], ...
            'FontWeight', 'bold', 'FontSize', 14);

    uilabel(fig, 'Text', 'Kp', 'Position', [30 615 30 22]);
    sld_Kp = uislider(fig, 'Position', [55 620 200 3], ...
                      'Limits', [0 5], 'Value', speed_pid.Kp);
    lbl_Kp = uilabel(fig, 'Text', num2str(speed_pid.Kp), 'Position', [265 615 50 22]);

    uilabel(fig, 'Text', 'Ki', 'Position', [30 575 30 22]);
    sld_Ki = uislider(fig, 'Position', [55 580 200 3], ...
                      'Limits', [0 20], 'Value', speed_pid.Ki);
    lbl_Ki = uilabel(fig, 'Text', num2str(speed_pid.Ki), 'Position', [265 575 50 22]);

    uilabel(fig, 'Text', 'Kd', 'Position', [30 535 30 22]);
    sld_Kd = uislider(fig, 'Position', [55 540 200 3], ...
                      'Limits', [0 1], 'Value', speed_pid.Kd);
    lbl_Kd = uilabel(fig, 'Text', num2str(speed_pid.Kd), 'Position', [265 535 50 22]);

    % ===== 电流环参数 =====
    uilabel(fig, 'Text', '电流环 PI', 'Position', [30 480 200 22], ...
            'FontWeight', 'bold', 'FontSize', 14);

    uilabel(fig, 'Text', 'Kp', 'Position', [30 445 30 22]);
    sld_Kp_i = uislider(fig, 'Position', [55 450 200 3], ...
                      'Limits', [0 10], 'Value', current_pid.Kp);
    lbl_Kp_i = uilabel(fig, 'Text', num2str(current_pid.Kp), 'Position', [265 445 50 22]);

    uilabel(fig, 'Text', 'Ki', 'Position', [30 405 30 22]);
    sld_Ki_i = uislider(fig, 'Position', [55 410 200 3], ...
                      'Limits', [0 200], 'Value', current_pid.Ki);
    lbl_Ki_i = uilabel(fig, 'Text', num2str(current_pid.Ki), 'Position', [265 405 50 22]);

    % ===== 目标转速 =====
    uilabel(fig, 'Text', '目标转速 (RPM)', 'Position', [30 350 150 22], ...
            'FontWeight', 'bold', 'FontSize', 14);
    sld_Target = uislider(fig, 'Position', [55 355 200 3], ...
                           'Limits', [500 8000], 'Value', TargetRPM);
    lbl_Target = uilabel(fig, 'Text', num2str(TargetRPM), 'Position', [265 350 80 22]);

    % ===== 仿真时长 =====
    uilabel(fig, 'Text', '仿真时长 (s)', 'Position', [30 305 150 22]);
    sld_Time = uislider(fig, 'Position', [55 310 200 3], ...
                        'Limits', [0.5 5], 'Value', sim.Tfinal);
    lbl_Time = uilabel(fig, 'Text', num2str(sim.Tfinal), 'Position', [265 305 50 22]);

    % ===== 运行按钮 =====
    btn = uibutton(fig, 'push', 'Text', '▶ 运行仿真', ...
                   'Position', [30 250 120 35], ...
                   'FontSize', 13, 'FontWeight', 'bold', ...
                   'ButtonPushedFcn', @run_sim_callback);

    % ===== 性能指标 =====
    uilabel(fig, 'Text', '性能指标', 'Position', [30 200 200 22], ...
            'FontWeight', 'bold', 'FontSize', 14);
    lbl_overshoot = uilabel(fig, 'Text', '超调量: --', 'Position', [30 175 200 22]);
    lbl_settling = uilabel(fig, 'Text', '稳定时间: --', 'Position', [30 150 200 22]);
    lbl_steadystate = uilabel(fig, 'Text', '稳态误差: --', 'Position', [30 125 200 22]);

    % ===== 绘图区域 =====
    ax_speed = uiaxes(fig, 'Position', [370 370 500 300]);
    title(ax_speed, '转速响应');
    xlabel(ax_speed, '时间 (s)');
    ylabel(ax_speed, '转速 (RPM)');
    grid(ax_speed, 'on');

    ax_current = uiaxes(fig, 'Position', [370 40 500 280]);
    title(ax_current, 'q 轴电流');
    xlabel(ax_current, '时间 (s)');
    ylabel(ax_current, '电流 (A)');
    grid(ax_current, 'on');

    % ===== 滑块实时更新标签 =====
    sld_Kp.ValueChangedFcn = @(s,~) set(lbl_Kp, 'Text', num2str(s.Value, '%.2f'));
    sld_Ki.ValueChangedFcn = @(s,~) set(lbl_Ki, 'Text', num2str(s.Value, '%.2f'));
    sld_Kd.ValueChangedFcn = @(s,~) set(lbl_Kd, 'Text', num2str(s.Value, '%.3f'));
    sld_Kp_i.ValueChangedFcn = @(s,~) set(lbl_Kp_i, 'Text', num2str(s.Value, '%.2f'));
    sld_Ki_i.ValueChangedFcn = @(s,~) set(lbl_Ki_i, 'Text', num2str(s.Value, '%.1f'));
    sld_Target.ValueChangedFcn = @(s,~) set(lbl_Target, 'Text', num2str(round(s.Value)));
    sld_Time.ValueChangedFcn = @(s,~) set(lbl_Time, 'Text', num2str(s.Value, '%.1f'));

    % ===== 仿真回调 =====
    function run_sim_callback(~, ~)
        % 读取滑条值
        Kp_val = sld_Kp.Value;
        Ki_val = sld_Ki.Value;
        Kd_val = sld_Kd.Value;
        Kp_i_val = sld_Kp_i.Value;
        Ki_i_val = sld_Ki_i.Value;
        target_val = sld_Target.Value;
        time_val = sld_Time.Value;

        % 更新工作区变量 (sim 从 base workspace 读取)
        assignin('base', 'speed_pid', struct('Kp',Kp_val,'Ki',Ki_val,'Kd',Kd_val,'SatLim',speed_pid.SatLim));
        assignin('base', 'current_pid', struct('Kp',Kp_i_val,'Ki',Ki_i_val,'SatLim',current_pid.SatLim));
        assignin('base', 'TargetRPM', target_val);
        assignin('base', 'sim', struct('Tfinal',time_val,'Ts',sim.Ts,'StepTime',sim.StepTime));

        % 更新模型参数
        set_param('foc_simulation/Speed_PID', 'P', num2str(Kp_val), ...
                  'I', num2str(Ki_val), 'D', num2str(Kd_val));
        set_param('foc_simulation/Electrical_TF', ...
                  'Denominator', sprintf('[%.6f %.3f]', motor.PhaseL, motor.PhaseR));
        set_param('foc_simulation/Step_RPM', 'After', num2str(target_val));
        set_param('foc_simulation', 'StopTime', num2str(time_val));

        % 运行仿真
        simOut = sim('foc_simulation', 'SrcWorkspace', 'current');

        % 读取仿真结果
        try
            % 优先从 simOut 读取
            if exist('simOut', 'var') && ~isempty(simOut)
                st = simOut.get('sim_time');
                sr = simOut.get('sim_rpm');
                si = simOut.get('sim_iq');
            else
                % 备用: 从 base workspace 读取
                st = evalin('base', 'sim_time');
                sr = evalin('base', 'sim_rpm');
            end
            t = st.Data;
            rpm_actual = sr.Data;
            rpm_ref = ones(size(t)) * target_val;
        catch
            uialert(fig, '无法读取仿真结果。请确认模型中有 sim_time/sim_rpm To Workspace 块。', '错误');
            return;
        end

        % 绘制速度响应
        plot(ax_speed, t, rpm_ref, 'r--', 'LineWidth', 1.5);
        hold(ax_speed, 'on');
        plot(ax_speed, t, rpm_actual, 'b-', 'LineWidth', 2);
        hold(ax_speed, 'off');
        xlabel(ax_speed, '时间 (s)');
        ylabel(ax_speed, '转速 (RPM)');
        legend(ax_speed, {'目标', '实际'}, 'Location', 'southeast');
        grid(ax_speed, 'on');
        xlim(ax_speed, [0 time_val]);

        % 计算性能指标
        ss_val = rpm_actual(end);
        steady_error = abs(ss_val - target_val) / target_val * 100;
        overshoot = max(0, (max(rpm_actual) - target_val) / target_val * 100);

        % 稳定时间 (进入 ±2% 范围内的最晚时间)
        band = 0.02 * target_val;
        in_band = abs(rpm_actual - target_val) < band;
        settle_idx = find(in_band, 1, 'last');
        if isempty(settle_idx) || settle_idx < length(rpm_actual) * 0.5
            settle_time = inf;
        else
            % 向后找到首次进入的索引
            first_in = find(~in_band, 1, 'last');
            if isempty(first_in), first_in = 0; end
            first_in = min(first_in + 1, length(t));
            settle_time = t(first_in);
        end

        set(lbl_overshoot, 'Text', sprintf('超调量: %.1f%%', overshoot));
        if isinf(settle_time)
            set(lbl_settling, 'Text', '稳定时间: 未稳定');
        else
            set(lbl_settling, 'Text', sprintf('稳定时间: %.3f s', settle_time - sim.StepTime));
        end
        set(lbl_steadystate, 'Text', sprintf('稳态误差: %.1f%%', steady_error));

        % 绘制电流
        try
            if exist('si', 'var') && ~isempty(si)
                tc = si.Time;
                iq = si.Data;
            else
                si = evalin('base', 'sim_iq');
                tc = si.Time;
                iq = si.Data;
            end
            plot(ax_current, tc, iq, 'g-', 'LineWidth', 1.5);
            xlabel(ax_current, '时间 (s)');
            ylabel(ax_current, '电流 (A)');
            grid(ax_current, 'on');
            xlim(ax_current, [0 time_val]);
        catch
            cla(ax_current);
            text(ax_current, 0.5, 0.5, '电流数据不可用', 'HorizontalAlignment', 'center');
        end

        % 打印结果到命令窗口
        fprintf('\n=== PID 调参结果 ===\n');
        fprintf('速度环: Kp=%.2f  Ki=%.2f  Kd=%.3f\n', Kp_val, Ki_val, Kd_val);
        fprintf('电流环: Kp=%.2f  Ki=%.1f\n', Kp_i_val, Ki_i_val);
        fprintf('超调量: %.1f%%  稳态误差: %.1f%%\n', overshoot, steady_error);
        if ~isinf(settle_time)
            fprintf('稳定时间: %.3f s\n', settle_time - sim.StepTime);
        end
    end

    % ===== 初始运行 =====
    run_sim_callback();
end
