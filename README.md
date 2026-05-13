# 🛡️ BanSafe - 伴星安全防护系统

<div align="center">

**专为 6-18 岁青少年打造的智能安全守护方案**

[![License](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Version](https://img.shields.io/badge/Version-v0.5_重构中-orange.svg)](docs/specifications/system_spec.md)
[![ESP32-S3](https://img.shields.io/badge/ESP32--S3-EYE-green.svg)](https://espressif.com)
[![TinyML](https://img.shields.io/badge/TinyML-TensorFlow%20Lite%20Micro-purple.svg)](https://www.tensorflow.org/micro)
[![CI](https://github.com/Qin-hyan/Banxingsafe/actions/workflows/ci.yml/badge.svg)](https://github.com/Qin-hyan/Banxingsafe/actions/workflows/ci.yml)

</div>

---

## 🌟 项目简介

> **"让每一次成长，都有科技的温暖守护"**

BanSafe（伴星安全）是一款面向儿童及青少年的智能安全防护系统。通过传感器融合技术和 AI 智能检测，为孩子们构建一道无形的安全防线。

### ✨ 核心特性

| 特性 | 描述 | 状态 |
|-----|------|------|
| 🎯 **三重智能检测** | 姿态 + 音频 + 视觉融合分析，精准识别危险场景 | ⚠️ 阶段1已实现 |
| 📍 **群体感知定位** | BLE 室内定位 | ❌ 规划中 |
| 🔔 **秒级警报响应** | 报警触发后即时本地响应 | ⚠️ 框架就绪 |
| 🔒 **隐私优先设计** | 敏感数据本地处理 | ✅ 架构支持 |
| 🔋 **分层唤醒机制** | 三级级联节省功耗 | ✅ 架构已落地 |

---

## 📁 项目结构 (重构后)

```
BanSafe/
├── firmware/                # 嵌入式固件（四层架构）
│   ├── app/                 # 应用层 — main.c 主入口
│   ├── algorithm/           # 算法与业务层 — 三重判定引擎
│   ├── hal/                 # 硬件抽象层 — 传感器/摄像头/音频接口
│   ├── drivers/             # 驱动层
│   │   ├── sensors/         #   QMA7981 等传感器驱动
│   │   ├── camera/          #   OV2640 驱动 (规划中)
│   │   └── display/         #   LCD 驱动 (规划中)
│   ├── models/              # TFLite AI 模型文件 (规划中)
│   └── CMakeLists.txt       # ESP-IDF 构建配置 (待适配)
├── src/                     # 云端/移动端
│   ├── backend/             # 后端 API (规划中)
│   └── mobile/              # 移动端 App (规划中)
├── docs/                    # 文档
│   ├── architecture/        #  系统架构、硬件选型、技术决策
│   ├── specifications/      #  系统规格说明书
│   ├── hardware/            #  硬件手册
│   └── api/                 #  API 接口文档
├── tests/                   # 测试代码
├── scripts/                 # 构建/部署脚本
├── dayplan/                 # 开发路线图
├── embedded_src/            # 旧代码（待清理确认）
└── .github/workflows/       # CI/CD
```

---

## 🚀 快速开始

### 前置要求

- **嵌入式开发**: ESP-IDF v5.0+, Python 3.8+
- **云端开发**: Node.js 18+ / Python 3.10+

### 安装步骤

```bash
# 克隆仓库
git clone https://github.com/Qin-hyan/Banxingsafe.git
cd Banxingsafe

# 切换重构分支
git checkout refactor/architecture-v1

# 嵌入式固件
# 将 firmware/ 复制到 ESP-IDF 项目目录中使用
# 详细构建说明见 docs/hardware/esp32_s3_eye_datasheet.md
```

---

## 🧠 技术架构

### 分层架构

```
应用层 (app) → 算法与业务层 (algorithm) → HAL ← 驱动层 (drivers)
```

详细架构图见 [系统架构文档](docs/architecture/system_architecture.md)。

### 三重判定引擎

当前实现状态：

```
阶段 1: 姿态检测 ✅ 已实现
    ↓ 评分 ≥ 40
阶段 2: 音频分析 ❌ 规划中（占位，当前放行）
    ↓ 评分 ≥ 60
阶段 3: 视觉分析 ❌ 规划中（占位，当前放行）
    ↓ 评分 ≥ 80
🚨 触发警报
```

> ⚠️ 阶段2/3未就绪时，引擎退化为单姿态判定（误报率较高）。

### 技术栈

#### 嵌入式端
- **主控**: ESP32-S3-WROOM-1 (双核 LX7, 240MHz)
- **RTOS**: FreeRTOS (ESP-IDF v5.0+)
- **AI 推理**: TensorFlow Lite Micro (规划中)
- **传感器**: QMA7981 (加速度计)
- **存储**: 8MB Flash + 8MB Octal PSRAM

#### 后端 (规划中)
- **API**: Python FastAPI
- **数据库**: SQLite / PostgreSQL
- **部署**: Docker

---

## 📚 文档导航

| 文档 | 路径 | 说明 |
|------|------|------|
| 系统架构 | [docs/architecture/system_architecture.md](docs/architecture/system_architecture.md) | 分层架构图、数据流 |
| 硬件选型 | [docs/architecture/hardware_selection.md](docs/architecture/hardware_selection.md) | 引脚分配、传感器型号 |
| 系统规格 | [docs/specifications/system_spec.md](docs/specifications/system_spec.md) | 修订后规格（与代码对齐） |
| 硬件手册 | [docs/hardware/esp32_s3_eye_datasheet.md](docs/hardware/esp32_s3_eye_datasheet.md) | ESP32-S3-EYE 参考 |
| API 设计 | [docs/api/api_design.md](docs/api/api_design.md) | HAL/引擎接口约定 |
| 技术决策 | [docs/architecture/decisions/](docs/architecture/decisions/) | ADR 记录 |
| 开发路线 | [dayplan/](dayplan/) | 14 天冲刺计划 |

---

## 📊 当前状态

| 子系统 | 完成度 | 说明 |
|--------|--------|------|
| QMA7981 驱动 | ✅ 100% | 已迁移到 firmware/drivers/sensors/ |
| HAL 传感器层 | ✅ 100% | 封装加速度/陀螺仪读取 |
| 三重判定引擎 | ⚠️ 33% | 仅阶段1（姿态）就绪 |
| HAL 摄像头层 | ❌ 0% | 接口已定义，驱动未实现 |
| HAL 音频层 | ❌ 0% | 接口已定义，驱动未实现 |
| Wi-Fi 通信 | ❌ 0% | 任务框架已创建 |
| BLE 定位 | ❌ 0% | 规划中 |
| 后端 API | ❌ 0% | 规划中 |
| 移动端 App | ❌ 0% | 规划中 |

---

## 📄 许可证

本项目采用 MIT 许可证。详见 [LICENSE](LICENSE) 文件。

---

<div align="center">

**❤️ 用科技守护童年，让安全触手可及**

⭐ 如果这个项目对你有帮助，请给一个 Star！

</div>