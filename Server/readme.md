# MyTinyHttpd - 轻量级C语言HTTP服务器

![C](https://img.shields.io/badge/language-C99-blue)
![License](https://img.shields.io/badge/license-MIT-green)

MyTinyHttpd是一个基于C语言开发的轻量级高性能HTTP服务器，专为嵌入式系统和Linux环境设计。项目采用libevent实现事件驱动的非阻塞I/O模型，支持设备管理API和前端交互API，具备MySQL数据库集成能力。

## 🌟 特性

- **高性能**: 基于libevent的事件驱动架构，支持高并发连接
- **轻量级**: 纯C实现，资源占用少，适合嵌入式环境
- **API支持**: 提供完整的设备管理和前端交互RESTful API
- **数据库集成**: 内置MySQL支持，实现数据持久化
- **JSON处理**: 集成cJSON库，支持JSON数据解析与生成
- **配置灵活**: 支持配置文件动态加载服务器参数
- **优雅退出**: 完善的信号处理机制，支持SIGINT/SIGTERM优雅关闭

## 🏗️ 系统架构

```
.
├── config                 # 配置文件目录
│   └── server.conf        # 服务器配置文件
├── include                # 头文件目录
│   ├── config.h           # 配置管理接口
│   ├── db_mysql.h         # MySQL数据库接口
│   ├── device_api.h       # 设备管理API接口
│   ├── frontend_api.h     # 前端API接口
│   ├── http_parse.h       # HTTP请求解析接口
│   ├── http_routes.h      # 路由分发接口
│   ├── http_server.h      # HTTP服务器主接口
│   ├── signal_handler.h   # 信号处理接口
│   └── utils.h            # 工具函数接口
├── src                    # 源代码目录
│   ├── config.c           # 配置文件解析实现
│   ├── db_mysql.c         # MySQL数据库操作实现
│   ├── device_api.c       # 设备管理业务逻辑
│   ├── frontend_api.c     # 前端交互业务逻辑
│   ├── http_parse.c       # HTTP协议解析实现
│   ├── http_routes.c      # URL路由分发实现
│   ├── http_server.c      # HTTP服务器核心实现
│   ├── main.c             # 程序入口点
│   ├── signal_handler.c   # 信号处理实现
│   └── utils.c            # 通用工具函数实现
└── third_party            # 第三方依赖
    └── cJSON              # cJSON JSON库源码
        ├── cJSON.c
        └── cJSON.h
```

### 架构层次

1. **表现层**: HTTP Server/Routes - 处理网络连接和请求分发
2. **业务逻辑层**: API Modules - 实现具体的设备管理和前端交互功能
3. **数据访问层**: DB MySQL - 封装数据库操作
4. **基础支撑层**: Utils/Config/Signal - 提供通用工具和系统服务

## ⚙️ 技术栈

- **编程语言**: C (C99标准)
- **构建系统**: CMake (>= 3.10)
- **核心依赖**:
  - libevent: 异步网络通信
  - mysqlclient: MySQL数据库连接
  - cJSON: JSON数据处理（静态链接）

## 🚀 快速开始

### 环境准备

在Ubuntu/Debian系统上安装依赖：

```bash
sudo apt-get update
sudo apt-get install cmake build-essential pkg-config libevent-dev libmysqlclient-dev
```

### 构建项目

```bash
# 克隆项目（如果尚未克隆）
git clone <your-repo-url>
cd linux2412-master

# 创建构建目录
mkdir build
cd build

# 配置项目
cmake ..

# 编译
make
```

### 配置服务器

编辑配置文件 `config/server.conf`，设置服务器参数如端口号、数据库连接信息等。

### 运行服务器

```bash
# 在build目录下运行
./MyTinyHttpd
```

服务器将根据配置文件启动，并监听指定端口。

## 📝 API文档

### 设备管理API (`/api/device/*`)

- `GET /api/device/list` - 获取设备列表
- `POST /api/device/add` - 添加新设备
- `PUT /api/device/update` - 更新设备信息
- `DELETE /api/device/remove` - 删除设备

### 前端交互API (`/api/frontend/*`)

- `GET /api/frontend/status` - 获取系统状态
- `POST /api/frontend/command` - 发送控制命令
- `GET /api/frontend/data` - 获取实时数据

> **注意**: 具体API端点和参数请参考源代码中的 `device_api.c` 和 `frontend_api.c` 实现。

## 🔒 安全考虑

- **SQL注入防护**: 数据库操作使用预处理语句防止SQL注入攻击
- **输入验证**: HTTP请求解析器对所有输入进行严格校验
- **内存安全**: 遵循C语言内存管理最佳实践，避免缓冲区溢出

## 🤝 贡献指南

欢迎提交Issue和Pull Request！贡献前请确保：

1. 遵循项目的代码风格和注释规范
2. 添加必要的单元测试（如果适用）
3. 更新相关文档

## 📄 许可证

本项目采用 [MIT许可证](LICENSE)。

**MyTinyHttpd** - 让轻量级Web服务变得简单高效！
