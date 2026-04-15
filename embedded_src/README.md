# 🔌 BanSafe 嵌入式源代码

本目录存放 BanSafe 项目的嵌入式端源代码（ESP32-S3）。

## 📁 目录结构

```
embedded_src/
├── firmware/        # 固件主程序
├── drivers/         # 传感器驱动
├── models/          # AI 模型文件
└── README.md        # 本文件
```

## 🎯 硬件平台

- **主控**: ESP32-S3-WROOM-1
- **传感器**:
  - QMA7981 (姿态传感器)
  - BMP388 (气压传感器)
  - INMP441 (麦克风)
  - OV2640 (摄像头)
- **通信**: BLE 5.0, WiFi, 4G Cat.1

## 🔧 开发环境

### 前置要求
- ESP-IDF v5.0+
- Python 3.8+

### 构建固件
```bash
cd firmware
idf.py set-target esp32s3
idf.py build
idf.py flash
```

## 📚 相关文档

- [硬件规格](../docs/hardware_specs.md)
- [规格说明](../docs/specifications/BanSafe_Specification_v0.4.md)