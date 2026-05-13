# BanSafe 硬件选型与引脚分配

> 版本: 0.1.0 | 日期: 2026-05-13

---

## 1. 主控芯片

| 项目 | 规格 |
|------|------|
| 芯片 | ESP32-S3-WROOM-1 |
| 架构 | 双核 Xtensa LX7 @ 240 MHz |
| SRAM | 512 KB (internal) |
| PSRAM | 8 MB (Octal SPI) |
| Flash | 8 MB |
| Wi-Fi | 802.11 b/g/n (2.4 GHz) |
| BLE | Bluetooth 5.0 LE |

---

## 2. 开发板

**ESP32-S3-EYE** — Espressif 官方 AI 开发板

| 资源 | 说明 |
|------|------|
| 摄像头接口 | DVP 8-bit 并行接口 |
| 麦克风 | 数字 MEMS 麦克风 (I2S) |
| 加速度计 | QMA7981 (I2C) |
| LCD | ST7789V 1.3" TFT (240×240) |
| USB | USB-UART + JTAG (USB-C) |
| 供电 | USB 5V 或 LiPo 电池 |

---

## 3. 传感器引脚分配

### 3.1 QMA7981 加速度计 (I2C)

| 引脚 | ESP32-S3 GPIO | 功能 |
|------|---------------|------|
| SDA | GPIO 4 | I2C 数据 |
| SCL | GPIO 5 | I2C 时钟 |
| INT1 | GPIO 3 | 中断输出 |

- I2C 地址: `0x12` (7-bit)
- I2C 总线编号: `I2C_NUM_0`
- 时钟频率: 400 kHz (Fast Mode)
- 量程: ±2g / ±4g / ±8g / ±16g (可配置)

### 3.2 OV2640 摄像头 (DVP) 【规划中】

| 引脚 | ESP32-S3 GPIO | 功能 |
|------|---------------|------|
| D0-D7 | GPIO 11-18 | 数据总线 |
| XCLK | GPIO 10 | 主时钟输出 |
| PCLK | GPIO 9 | 像素时钟输入 |
| VSYNC | GPIO 6 | 垂直同步 |
| HREF | GPIO 7 | 水平参考 |
| SDA | GPIO 4 | SCCB 数据 (与 I2C 共用) |
| SCL | GPIO 5 | SCCB 时钟 (与 I2C 共用) |

### 3.3 INMP441 数字麦克风 (I2S) 【规划中】

| 引脚 | ESP32-S3 GPIO | 功能 |
|------|---------------|------|
| SD | GPIO 8 | I2S 数据输入 |
| SCK | GPIO 2 | I2S 时钟 |
| WS | GPIO 1 | I2S 字选 (L/R) |
| L/R | GND | 左声道 (单声道模式) |

### 3.4 LCD ST7789V (SPI) 【规划中】

| 引脚 | ESP32-S3 GPIO | 功能 |
|------|---------------|------|
| MOSI | GPIO 35 | SPI 数据 |
| SCLK | GPIO 36 | SPI 时钟 |
| CS | GPIO 37 | 片选 |
| DC | GPIO 38 | 数据/命令 |
| RST | GPIO 39 | 复位 |
| BL | GPIO 40 | 背光控制 |

---

## 4. 外设资源汇总

| 外设 | 接口 | GPIO | 驱动文件 |
|------|------|------|----------|
| QMA7981 | I2C0 | 4, 5 | `firmware/drivers/sensors/qma7981.c` |
| OV2640 | DVP + SCCB | 6-18 | 规划中 |
| INMP441 | I2S | 1, 2, 8 | 规划中 |
| LCD | SPI | 35-40 | 规划中 |
| USB-UART | USB-C | — | ESP-IDF 内置 |
| MicroSD | SPI | TBD | 规划中 |

---

## 5. 电源域

| 电压 | 用途 |
|------|------|
| 3.3V | ESP32-S3, QMA7981, INMP441, LCD |
| 2.8V | OV2640 模拟 |
| 1.2V | OV2640 数字核心 |

> 板载 PMU (电源管理单元) 自动处理上述电压。

---

## 6. 内存预算

| 区域 | 大小 | 用途 |
|------|------|------|
| Flash | 8 MB | 固件 + 文件系统 (LittleFS) + OTA |
| PSRAM | 8 MB | 图像缓冲、TFLite 张量、音频缓冲 |
| Internal SRAM | 512 KB | FreeRTOS 栈、Wi-Fi/BLE 协议栈、应用数据 |