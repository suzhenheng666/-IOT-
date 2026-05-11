# -IOT-- 项目概述

纯上行三层物联网系统：STM32F4 传感器+FOC 电机控制 → ESP32-S3 MQTT 网关+YOLO 视觉 → Linux C 后端 (libmosquitto+libmicrohttpd) + 纯 HTML 前端。

## 架构

```
STM32F4 (DHT11 + FOC风机) ──UART──→ ESP32-S3 ──MQTT──→ Mosquitto ──→ Backend_c/C ──REST──→ 纯HTML前端
                                    │   OV2640 摄像头
                                    │   ESP-DL YOLO (3类故障灯)
```

关键设计决策：
- **纯上行**：已删除所有下行控制链路（CloudCmd_t、Process_ESP32_Command、cloudCmdQueueHandle、手动风扇控制、Fan.h/c）
- **协议层**：二进制帧，两端统一 `0xA5 0x5A + cmd + len + payload + checksum`，fan_speed 为 uint16_t RPM
- **温度-PID 联动**：外层温度 PID（2.5s周期）根据机柜温度动态计算目标转速，内层速度环+电流环 FOC（1ms周期）跟踪执行
- **MQTT**：ESP32 发布到 Mosquitto broker（QoS 1），后端 libmosquitto 订阅
- **无 git 仓库**：项目当前未纳入版本控制

## 技术栈

| 层 | 技术 |
|----|------|
| STM32 固件 | STM32F4xx HAL, FreeRTOS/CMSIS-RTOS2, I2C(AS5600), TIM1 PWM, UART DMA |
| FOC 算法 | PID 速度环 + PI 电流环, Clarke/Park/InvPark/SVPWM, DJI 2312 (7对极, KV=920) |
| ESP32 固件 | ESP-IDF, esp-mqtt, esp32-camera (OV2640), ESP-DL, cJSON, FreeRTOS tasks |
| YOLO 模型 | YOLOv8n, 3类(绿/黄/红), 合成数据集 500 张, mAP50=0.995, ONNX INT8→ESP-DL |
| 仿真 | MATLAB/Simulink, foc_simulation.slx + pid_tune_gui.m |
| 后端(C) | libmosquitto, libmicrohttpd, cJSON, pthread, 端口 8080 |
| 后端(JS) | Express 4.x, mqtt.js 5.x, CORS, 端口 8080 (已被 Backend_c 替代) |
| 前端 | 纯 HTML + ECharts CDN，单文件无框架，5 秒轮询 |
| 前端(Vue) | Vue3 Composition API, ECharts, Axios, Vite (已被 frontend.html 替代) |
| MQTT Broker | Mosquitto |

## 目录结构

```
-IOT--main/
├── STM32/                          # STM32F4 固件 (Keil MDK-ARM)
│   ├── Core/Inc/User/
│   │   ├── AS5600.h                # 磁编码器 (I2C 0x36)
│   │   ├── BLDC_FOC.h              # FOC + PID 结构体 + 电机参数
│   │   ├── DHT11.h                 # 温湿度传感器
│   │   └── protocol.h              # 二进制协议帧定义 (CMD_SENSOR_REPORT=0x00)
│   ├── Core/Src/User/
│   │   ├── AS5600.c                # 12bit角度读取 + 磁铁检测
│   │   ├── BLDC_FOC.c              # FOC全流程: Clarke→Park→SpeedPID→CurrentPI→InvPark→SVPWM
│   │   ├── protocol.c              # Send_Sensor_Data + Calc_Checksum
│   │   └── DHT11.c
│   ├── Core/Src/freertos.c         # focTask (1ms周期, osPriorityHigh), StartFocTask
│   └── Core/Src/main.c             # 已清理: 无Process_ESP32_Command, 无cmd_rx_buffer
│
├── ESP32/                          # ESP32-S3 固件 (ESP-IDF)
│   ├── main/main.c                 # 4 Tasks: uart解析 / MQTT遥测 / 摄像头YOLO(5s) / 心跳(30s)
│   ├── components/
│   │   ├── AI/
│   │   │   ├── yolo_detect.h/c     # ESP-DL YOLO 推理 (Yolo11Nano), 分类+JSON序列化
│   │   │   └── fault_light_model.espdl  # 模型骨架 (完整需ESP-DL Model Converter)
│   │   ├── Middlewares/
│   │   │   ├── MQTT/mqtt_app.h/c   # MQTT连接+发布 (3个topic)
│   │   │   ├── Protocol/Protocol.h/c  # 与STM32一致的协议定义
│   │   │   └── WIFI/wifi_app.h/c
│   │   ├── Peripherals/Camera/camera_app.h/c  # OV2640初始化+JPEG捕获
│   │   └── BSP/                    # UART + LED 驱动
│   └── CMakeLists.txt              # EXTRA_COMPONENT_DIRS 已配置
│
├── YOLO_Training/                  # YOLOv8n 训练 (Python)
│   ├── generate_dataset.py         # 合成500张服务器面板图片 (320×320) + 自动标注
│   ├── train_yolo.py               # 训练脚本 (50epochs, mAP=0.995), 自动调generate_dataset
│   ├── export_espdl.py             # ONNX INT8量化 → .espdl 格式 (best.onnx → best_int8.onnx)
│   ├── best.pt                     # 训练权重 (6MB)
│   ├── best.onnx                   # ONNX导出 (12MB)
│   ├── best_int8.onnx              # INT8量化 (3.2MB)
│   └── dataset/                    # 合成数据集 (400 train + 100 val, 1462标注框)
│
├── Backend_c/                       # C 后端 (Linux) — 当前主力
│   ├── main.c                       # 入口: MQTT线程 + HTTP服务 (8080)
│   ├── store.h/c                    # 线程安全内存存储 (mutex, 环形队列1000/500)
│   ├── mqtt_client.h/c              # libmosquitto 订阅3路topic
│   ├── http_server.h/c              # libmicrohttpd REST API (5端点)
│   ├── cJSON.h/c                    # JSON解析/构建
│   ├── frontend.html                # 纯HTML仪表盘 (单文件, ECharts CDN)
│   └── Makefile
│
├── Backend/                        # Express 后端 (Node.js) — 已被 Backend_c 替代
│   ├── server.js
│   ├── mqtt_subscriber.js
│   ├── routes/devices.js
│   └── package.json
│
├── Frontend/                       # Vue3 前端 — 已被 frontend.html 替代
│   ├── src/views/DeviceList.vue
│   ├── src/api/index.js
│   └── src/components/DeviceChart.vue
│
├── Simulink/                       # MATLAB FOC 仿真
│   ├── foc_params.m                # DJI 2312 电机参数 (KV/PolePairs/PhaseR/PhaseL/J/B)
│   ├── build_foc_model.m           # 程序化构建 foc_simulation.slx (速度环+电流环+反馈)
│   ├── pid_tune_gui.m              # PID 调参 GUI (6滑条+实时转速/电流曲线+性能指标)
│   └── README.md
│
├── Server/                         # 旧版后端 (HTTP直连ESP32, 含MySQL) — 已被 Backend/ 替代
├── Presentation/                   # PPT 演示文稿
└── docs/superpowers/               # 设计文档
    ├── specs/2026-05-11-iot-mqtt-redesign.md   # 设计规格书
    └── plans/2026-05-11-iot-mqtt-redesign.md   # 实现计划
```

## 关键数据流

1. STM32: DHT11 读取(2.5s) → **温度 PID** 计算目标转速 → **速度环+电流环 FOC**(1ms) → SVPWM → TIM1 PWM → 周期上报 SensorPayload_t 帧至 UART1
2. ESP32: UART1 RX → **状态机搜帧头 0xA5 0x5A** → 校验通过 → FreeRTOS 队列 → mqtt_telemetry_task 发布 JSON
3. ESP32: 每5秒 camera_capture → JPEG解码→预处理→**INT8量化YOLO推理** → cJSON序列化 → `device/dev001/camera`
4. Backend_c: libmosquitto **三路主题订阅** → 线程安全 store (环形队列) → libmicrohttpd REST API (5 端点)
5. Frontend: frontend.html **Promise.all 并行 fetch** → 卡片+表格+ECharts 折线图，每 5 秒自动刷新

## 数据协议

STM32→ESP32 二进制帧:
```
0xA5 0x5A | cmd(1B) | len(1B) | temp_int temp_dec humi_int humi_dec fan_speed(2B) | checksum(1B)
```
- fan_speed 从 uint8_t 改为 uint16_t (RPM 范围: 0-65535)
- CMD_SENSOR_REPORT = 0x00 (只有这一个命令，无下行命令)

MQTT JSON (ESP32→Broker→Backend):
```json
// telemetry: {"device_id":"dev001","lon":...,"lat":...,"temperature":25.3,"humidity":60.1,"fan_speed":3000}
// camera:   {"device_id":"dev001","fault_status":"warning","red_prob":0.12,"yellow_prob":0.78,...}
// status:   {"device_id":"dev001","status":"online","uptime":3600}
```

## 快速启动

```bash
# C 后端 (Linux)
sudo apt install libmosquitto-dev libmicrohttpd-dev
cd Backend_c && make && ./iot_backend

# 前端: 浏览器直接打开 Backend_c/frontend.html 即可 (fetch → localhost:8080)

# YOLO 训练 (需要 PyTorch + CUDA)
cd YOLO_Training && pip install -r requirements.txt && python generate_dataset.py && cd dataset && python ../train_yolo.py

# MATLAB 仿真
# MATLAB 中运行:
#   cd('Simulink'); run('foc_params.m'); build_foc_model; pid_tune_gui;
```

## ESP32 构建前置条件

- ESP-IDF v5.x + esp-mqtt, esp32-camera, ESP-DL 组件
- OV2640 引脚: 见 `ESP32/components/Peripherals/Camera/camera_app.h`
- PSRAM 必须启用 (sdkconfig)
- `fault_light_model.espdl` 部署到 `ESP32/components/AI/` — 需 ESP-DL Model Converter 生成完整权重

## STM32 构建前置条件

- Keil MDK-ARM 或 STM32CubeIDE
- 外设: I2C1(AS5600 0x36), TIM1_CH1-3(PWM), UART1(TX to ESP32), UART2(debug), DHT11(GPIO)
- DRV8301: SPI 配置 (可选，当前代码使用简化驱动)

## 注意事项

- 项目根路径含中文 `测试`，部分工具（YOLO训练、Python）对中文路径敏感。已通过 `os.chdir("dataset")` + 相对路径解决。
- 无 git 仓库 — 备份靠手动复制。
- `Server/` 是旧版后端 (HTTP+MySQL)，已废弃，不要修改。
- `Backend/` 和 `Frontend/` 是 JS 版，已被 `Backend_c/` 和 `frontend.html` 替代。C 版无需 npm。
- `STM32/Core/Src/User/Fan.h` 和 `Fan.c` 已删除，风扇控制已迁移到 `BLDC_FOC.c` 中的 `focTask`，并加入了温度 PID 外层控制。
- `Backend_c/` 编译依赖 libmosquitto-dev 和 libmicrohttpd-dev（`sudo apt install` 安装）。
- YOLO 模型用合成数据集训练（不是真实服务器图片），真实场景精度待验证。
