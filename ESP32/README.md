# ESP32 边缘计算网关：AIoT 双向控制系统

## 📌 项目简介
本项目基于 **ESP32-S3** 开发，作为整个 AIoT 系统的“中枢神经”，充当边缘计算网关的角色。它向上通过 Wi-Fi 连接 Linux C 语言服务器，向下通过自定义串口协议管理 STM32 传感器终端。

## 🛠 核心功能
- **多任务并发 (FreeRTOS)**：采用多任务架构，独立处理串口收发、HTTP 通信与状态监控。
- **边缘计算预处理**：对 STM32 上报的原始十六进制字节流进行校验、还原及 JSON 格式化封装。
- **全双工指令下发**：实现“顺风车”模式，在 HTTP 响应中实时拦截并解析云端控制指令。
- **异常处理机制**：具备完善的网络断连重连、HTTP 超时拦截以及 cJSON 内存安全管理。

## 🚀 技术亮点 (Shining Operations)

### 1. 跨平台二进制协议状态机
为了解决 UART 通信中常见的粘包、碎包问题，在 ESP32 端实现了一套健壮的状态机：
- **上行**：捕获 `0xA55A` 帧头，校验 CS 校验和，确保传感器数据准确性。
- **下发**：封装 `0x55AA` 控制帧，将复杂的云端逻辑压缩为 4 字节高密度指令，降低 MCU 负载。

### 2. IEEE 754 精度陷阱修复
深入底层内存布局，解决了 `float` 隐式提升为 `double` 时产生的二进制截断误差。通过优化 cJSON 序列化逻辑，消除了类似 `26.20000076` 的长尾无效字符，使上报报文更加精准且节省带宽。

### 3. 解耦式消息队列架构
利用 **FreeRTOS Message Queue** 实现生产者（串口读取）与消费者（HTTP 上传）的异步解耦。
- **稳定性**：网络波动期间，传感器采集任务不受阻塞，数据在队列中排队。
- **实时性**：一旦收到数据，立即唤醒阻塞中的 HTTP 任务，响应延迟降至百毫秒级。

### 4. 严苛的内存安全管控
在资源受限的嵌入式环境下，严格执行 `cJSON` 和 `esp_http_client` 的生命周期管理：
- 采用 `cleanup` 钩子机制，确保每次 HTTP 请求后的句柄释放与堆内存归还，杜绝 OOM 风险。

## 📂 代码结构
- `_http_event_handler`: 处理 HTTP 协议栈底层事件，实现分片数据拼接。
- `http_upload_task`: FreeRTOS 消费者任务，执行 JSON 打包与网络上传。
- `uart_event_task`: 串口监听任务，执行自定义协议状态机解析。

---
**开发者：** 苏振恒
**语言：** C / ESP-IDF
| Supported Targets | ESP32 | ESP32-C2 | ESP32-C3 | ESP32-S2 | ESP32-S3 |
| ----------------- | ----- | -------- | -------- | -------- | -------- |

# _Sample project_

(See the README.md file in the upper level 'examples' directory for more information about examples.)

This is the simplest buildable example. The example is used by command `idf.py create-project`
that copies the project to user specified path and set it's name. For more information follow the [docs page](https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html#start-a-new-project)



## How to use example
We encourage the users to use the example as a template for the new projects.
A recommended way is to follow the instructions on a [docs page](https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html#start-a-new-project).

## Example folder contents

The project **sample_project** contains one source file in C language [main.c](main/main.c). The file is located in folder [main](main).

ESP-IDF projects are built using CMake. The project build configuration is contained in `CMakeLists.txt`
files that provide set of directives and instructions describing the project's source files and targets
(executable, library, or both). 

Below is short explanation of remaining files in the project folder.

```
├── CMakeLists.txt
├── main
│   ├── CMakeLists.txt
│   └── main.c
└── README.md                  This is the file you are currently reading
```
Additionally, the sample project contains Makefile and component.mk files, used for the legacy Make based build system. 
They are not used or needed when building with CMake and idf.py.
