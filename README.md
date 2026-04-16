# 🛡️ BanSafe - 伴星安全防护系统

<div align="center">

**专为 6-18 岁青少年打造的智能安全守护方案**

[![License](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Version](https://img.shields.io/badge/Version-v0.4-orange.svg)](docs/specifications/BanSafe_Specification_v0.4.md)
[![ESP32-S3](https://img.shields.io/badge/ESP32--S3-EYE-green.svg)](https://espressif.com)
[![TinyML](https://img.shields.io/badge/TinyML-TensorFlow%20Lite%20Micro-purple.svg)](https://www.tensorflow.org/micro)
[![CI](https://github.com/Qin-hyan/Banxingsafe/actions/workflows/ci.yml/badge.svg)](https://github.com/Qin-hyan/Banxingsafe/actions/workflows/ci.yml)

</div>

---

## 🌟 项目简介

> **"让每一次成长，都有科技的温暖守护"**

BanSafe（伴星安全）是一款面向儿童及青少年的智能安全防护系统。我们深知，在这个数字化时代，孩子们在校内外面临着各种潜在的安全挑战。BanSafe 通过前沿的传感器融合技术、AI 智能检测和群体协作定位，为孩子们构建一道无形的安全防线。

### ✨ 核心特性

| 特性 | 描述 |
|-----|------|
| 🎯 **三重智能检测** | 姿态传感器 + 麦克风 + 摄像头融合分析，精准识别危险场景 |
| 📍 **群体感知定位** | 利用周围设备 BLE 信号实现校园内精确定位，误差±3-5 米 |
| 🔔 **秒级警报响应** | 警报触发后 3 秒内推送至最近的 3-5 位责任人 |
| 🔒 **隐私优先设计** | 所有敏感数据本地处理，零原始音视频上传 |
| 🔋 **超长续航** | 分层唤醒机制，优化功耗设计 |

---

## 🚀 快速开始

### 前置要求

- **嵌入式开发**: ESP-IDF v5.0+, Python 3.8+
- **云端开发**: Node.js 18+ 或 Python 3.10+
- **移动端开发**: Flutter 3.0+, Android Studio / Xcode

### 安装步骤

```bash
# 克隆仓库
git clone https://github.com/Qin-hyan/Banxingsafe.git
cd Banxingsafe

# 嵌入式开发（ESP32-S3）
cd embedded_src
# 按照 ESP-IDF 文档配置环境
idf.py set-target esp32s3
idf.py build

# 云端服务（示例）
cd src/backend
npm install
npm run dev
```

---

## 📁 项目结构

```
BanSafe/
├── src/                    # 云端/服务端源代码
│   ├── backend/           # 后端 API 服务
│   └── mobile/            # 移动端应用（Flutter）
├── embedded_src/          # 嵌入式端源代码（ESP32-S3）
│   ├── firmware/          # 固件主程序
│   ├── drivers/           # 传感器驱动
│   └── models/            # AI 模型文件
├── tests/                 # 测试代码
│   ├── unit/              # 单元测试
│   ├── integration/       # 集成测试
│   └── embedded/          # 嵌入式测试
├── docs/                  # 项目文档
│   ├── specifications/    # 规格说明书
│   ├── api/               # API 文档
│   └── guides/            # 使用指南
├── scripts/               # 构建/部署脚本
├── .github/workflows/     # CI/CD 工作流
├── dayplan/               # 开发路线图
├── README.md              # 项目说明
├── LICENSE                # 许可证
└── .gitignore             # Git 忽略规则
```

---

## 🧠 技术架构

### 三重判定引擎

我们的核心创新技术，通过三个阶段的智能检测，将误报率控制在 **0.5% 以下**：

```
阶段 1: 姿态检测 (常开)
    ↓ 姿态评分 ≥ 40
阶段 2: 音频分析 (10 秒窗口)
    ↓ 综合评分 ≥ 60
阶段 3: 视觉分析 (15 秒窗口)
    ↓ 综合评分 ≥ 80
🚨 触发警报
```

### 群体感知定位

利用校园内其他 BanSafe 设备的 BLE 信号，实现无需 GPS 的室内精确定位：

```
      设备 A (RSSI: -65dBm)
            ●
           / \
          /   \
         /     \
 设备 B ●/       \● 设备 C
(RSSI: -70)  (RSSI: -68)
         \     /
          \   /
           \ /
            ●
      目标设备 (警报源)
```

---

## 📚 技术栈

### 嵌入式端
- **主控芯片**: ESP32-S3-WROOM-1 (双核 LX7, 240MHz)
- **操作系统**: ESP-IDF / FreeRTOS v5.1+
- **AI 推理**: TensorFlow Lite Micro v2.12+
- **传感器**: QMA7981 (姿态) + BMP388 (气压) + INMP441 (单麦) + OV2640 (摄像头)
- **存储**: 8MB Flash + 512KB SRAM
- **通信**: BLE 5.0 + WiFi 802.11 b/g/n + 4G Cat.1 (EC200U)

### 云端
- **API 服务**: Node.js / Python (RESTful)
- **数据库**: PostgreSQL + Redis
- **消息队列**: RabbitMQ / Kafka
- **推送服务**: Firebase / 极光推送

### 移动端
- **框架**: Flutter (iOS + Android)
- **保安室大屏**: Vue.js + WebSocket

---

## 📚 文档

### 规格说明书

| 版本 | 路径 | 说明 |
|-----|------|------|
| v0.1 | [docs/specifications/BanSafe_Specification_v0.1.md](docs/specifications/BanSafe_Specification_v0.1.md) | 初始概念版本 |
| v0.2 | [docs/specifications/BanSafe_Specification_v0.2.md](docs/specifications/BanSafe_Specification_v0.2.md) | 三重判定引擎版本 |
| v0.3 | [docs/specifications/BanSafe_Specification_v0.3.md](docs/specifications/BanSafe_Specification_v0.3.md) | ESP32-S3-EYE 深度适配版本 |
| v0.4 | [docs/specifications/BanSafe_Specification_v0.4.md](docs/specifications/BanSafe_Specification_v0.4.md) | 规格修正版本（Flash 8MB/单麦） |

### 其他文档

- [硬件规格](docs/hardware_specs.md)
- [开发路线图](dayplan/伴星安全防护系统开发路线图.html)
- [API 文档](docs/api/) (待完善)
- [用户手册](docs/guides/) (待完善)

---

## 🛠️ 开发规划

```
P0 (2 周)  → 硬件选型验证
    ↓
P1 (4 周)  → 基础驱动开发
    ↓
P2 (6 周)  → 三重判定引擎
    ↓
P3 (4 周)  → 通信定位系统
    ↓
P4 (4 周)  → 云端服务开发
    ↓
P5 (4 周)  → 移动端开发
    ↓
P6 (4 周)  → 联调测试
```

**预计 MVP 完成时间**: 约 6 个月

---

## 📊 性能指标

| 指标 | 目标值 |
|-----|-------|
| 误报率 | < 0.5% |
| 定位精度 | ±5 米 (BLE) / ±100 米 (基站) |
| 响应时间 | < 3 秒 |
| 续航时间 | 待实测 |
| 隐私合规 | 《未成年人保护法》+ GDPR |

---

## 🤝 参与贡献

我们欢迎任何形式的贡献！包括但不限于：

- 🐛 报告 Bug
- 💡 提出新功能建议
- 📝 改进文档
- 🔧 代码提交

### 贡献流程

1. Fork 本仓库
2. 创建特性分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 开启 Pull Request

---

## 📄 许可证

本项目采用 MIT 许可证。详见 [LICENSE](LICENSE) 文件。

---

## 📞 联系方式

- **项目仓库**: https://github.com/Qin-hyan/Banxingsafe
- **问题反馈**: 提交 [GitHub Issue](https://github.com/Qin-hyan/Banxingsafe/issues)

---

<div align="center">

**❤️ 用科技守护童年，让安全触手可及**

⭐ 如果这个项目对你有帮助，请给一个 Star！

</div>