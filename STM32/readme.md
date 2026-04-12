# STM32 边缘传感执行终端：AIoT 实时操作系统底座

## 📌 项目简介
本项目基于 **STM32** 微控制器开发，作为整个 AIoT 系统的“物理交互层”。它运行 **FreeRTOS** 实时操作系统，负责精准驱动环境传感器（温湿度 DHT11 等），并将物理数据转换为自定义的高密度二进制流上报；同时，它通过异步中断监听来自边缘网关（ESP32）的控制指令，实现对继电器/风扇的微秒级实时响应。

## 🛠 核心功能
- **环境感知**：按严格的时序要求驱动 DHT11 等传感器，完成数据的精确采集。
- **硬件执行**：解析网关下发的指令，控制 GPIO 翻转，驱动风扇/继电器等外设。
- **RTOS 任务调度**：基于 FreeRTOS 实现多任务抢占式调度与信号量（Semaphore）同步。
- **高实时性通信**：采用 DMA/IT（中断）机制进行双向 UART 数据流转，彻底告别阻塞式等待。

## 🚀 技术亮点 (Shining Operations)

### 1. 极轻量级异步状态机 (Asynchronous State Machine)
摒弃了单片机开发中消耗极大资源的字符串处理（`strcmp`），专门为下行控制链路设计了 **4 字节定长二进制指令帧 (`0x55 0xAA CMD 0xFF`)**。
- **零阻塞解析**：在 `HAL_UART_RxCpltCallback` 回调中采用 1 字节中断接收（`HAL_UART_Receive_IT`），每到达一个字节便推入 `switch-case` 状态机。
- **性能极客**：指令解析复杂度降至 $O(1)$，全程不占用主任务 CPU 时间，即刻触发硬件动作。

### 2. 跨越 HAL 库与 FreeRTOS 的调度鸿沟
在项目开发中，深入剖析了 ST HAL 库与 FreeRTOS 内核的交接底层逻辑：
- 解决了因 `osKernelStart()` 接管 CPU 控制权导致的“初始化死区”陷阱。
- 确保外设默认状态（如 `Fan_Off()`）和初始中断监听指令被正确前置到调度器启动之前，保证了系统的启动绝对安全。

### 3. 基于信号量 (Semaphore) 的任务流转
彻底抛弃了传统的“前后台大循环 (`while(1)`)”架构，将系统拆分为高内聚、低耦合的独立任务。
- 利用 `vTaskDelay` 替换死等延时（`HAL_Delay`），压榨 CPU 并发性能。
- 在串口发送完成回调（`HAL_UART_TxCpltCallback`）中释放信号量（`osSemaphoreRelease`），优雅地唤醒处于阻塞态的数据上报任务，实现了纯正的事件驱动（Event-Driven）模型。

## 📂 代码架构简述
- `main.c`: 包含外设初始化、FreeRTOS 内核启动及指令解析状态机 `Process_ESP32_Command()`。
- `freertos.c`: 包含核心业务任务（如传感器轮询任务、串口通信上报任务）。
- `stm32fxx_it.c`: 存放硬件中断服务路由（ISR），如 `USART_IRQHandler`。

---
**开发者：** Gemini AI 协同完成
**架构层级：** 底层物理交互层 (感知与执行)
**技术栈：** C / STM32 HAL / FreeRTOS / UART 协议设计
