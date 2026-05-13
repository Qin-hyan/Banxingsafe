# ESP32-S3-EYE 硬件手册

> 来源: Espressif 官方文档 + 项目实测
> 版本: 0.1.0 | 日期: 2026-05-13

---

## 1. 开发板简介

ESP32-S3-EYE 是 Espressif 官方推出的 AI 评估开发板，专为图像识别、语音处理等边缘 AI 应用设计。

### 1.1 板载资源

| 组件 | 型号 | 说明 |
|------|------|------|
| 主控 | ESP32-S3-WROOM-1 | 双核 240MHz, 8MB Flash, 8MB PSRAM |
| 摄像头 | OV2640 | 200W 像素, DVP 接口 |
| 麦克风 | MEMS 数字麦克风 | I2S 接口 |
| 加速度计 | QMA7981 | 三轴 ±2/4/8/16g |
| LCD | ST7789V | 1.3" TFT, 240×240 |
| USB | USB-C | UART + JTAG |

---

## 2. ESP32-S3 规格

| 参数 | 值 |
|------|-----|
| CPU | 双核 Xtensa LX7 @ 240 MHz |
| SRAM | 512 KB |
| PSRAM | 8 MB (Octal SPI) |
| Flash | 8 MB |
| Wi-Fi | 802.11 b/g/n (2.4 GHz) |
| BLE | Bluetooth 5.0 LE |
| GPIO | 45 个可编程 IO |
| I2C | 2 组 |
| SPI | 4 组 |
| I2S | 2 组 |
| UART | 3 组 |
| USB | USB 1.1 OTG |

---

## 3. 引脚映射

### 3.1 QMA7981 (I2C)

| 功能 | GPIO | 备注 |
|------|------|------|
| SDA | 4 | I2C 数据线 |
| SCL | 5 | I2C 时钟线 |
| INT1 | 3 | 中断（可选） |
| I2C 地址 | 0x12 | 7-bit |

### 3.2 OV2640 (DVP) — 规划中

| 功能 | GPIO |
|------|------|
| XCLK | 10 |
| PCLK | 9 |
| VSYNC | 6 |
| HREF | 7 |
| D0-D7 | 11-18 |
| SDA | 4 (复用) |
| SCL | 5 (复用) |

### 3.3 INMP441 (I2S) — 规划中

| 功能 | GPIO |
|------|------|
| SD | 8 |
| SCK | 2 |
| WS | 1 |

### 3.4 LCD ST7789V (SPI) — 规划中

| 功能 | GPIO |
|------|------|
| MOSI | 35 |
| SCLK | 36 |
| CS | 37 |
| DC | 38 |
| RST | 39 |
| BL | 40 |

---

## 4. 电源

- USB-C 5V 供电
- 可选 LiPo 电池 (3.7V → 板载充电管理)
- 3.3V LDO 输出给 ESP32-S3 和外设

---

## 5. 开发环境

| 工具 | 版本要求 |
|------|----------|
| ESP-IDF | v5.0+ |
| CMake | ≥ 3.16 |
| Python | ≥ 3.7 |
| 烧录工具 | esptool.py |
| 串口驱动 | CH340 / CP210x (板载) |

### 常用命令

```bash
# 配置项目
idf.py set-target esp32s3
idf.py menuconfig

# 编译
idf.py build

# 烧录 + 监视
idf.py -p COMx flash monitor
```

---

## 6. 参考资料

- [ESP32-S3 技术参考手册](https://www.espressif.com/sites/default/files/documentation/esp32-s3_technical_reference_manual_cn.pdf)
- [ESP32-S3-EYE 用户指南](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s3/esp32-s3-eye/user_guide.html)
- [QMA7981 数据手册](docs/sensor_validation/qma7981_validation.md)