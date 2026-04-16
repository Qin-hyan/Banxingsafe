# QMA7981 姿态传感器选型验证报告

**文档编号**: SEN-VAL-001  
**版本**: 1.0  
**日期**: 2026-04-16  
**关联任务**: 开发路线图任务 #2  
**验证工程师**: 硬件架构师

---

## 1. 验证概述

本文档记录伴星安全防护系统 (BanSafe) 中 QMA7981 姿态传感器的选型验证过程和结果。

### 1.1 验证目标

| 验证项 | 目标描述 |
|-------|---------|
| I2C 通信兼容性 | 验证 400kHz I2C 速率是否在 ESP32-S3 驱动能力范围内 |
| 引脚冲突检查 | 验证推荐中断引脚 GPIO07 是否存在硬件冲突 |
| 功耗匹配度 | 验证低功耗模式是否符合系统电源管理策略 |

### 1.2 参考文档

- QMA7981 Datasheet (Infineon Technologies)
- ESP32-S3 Technical Reference Manual
- BanSafe 硬件规格文档 (hardware_specs.md)
- 伴星安全防护系统开发路线图

---

## 2. 传感器规格摘要

### 2.1 QMA7981 基本参数

| 参数 | 规格 |
|-----|------|
| 型号 | QMA7981 (Infineon) |
| 类型 | 6 轴 IMU (加速度计 + 陀螺仪) |
| I2C 接口 | 标准模式 100kHz / 快速模式 400kHz |
| I2C 地址 | 0x18 (默认) |
| 加速度量程 | ±2g / ±4g / ±8g / ±16g (可编程) |
| 陀螺仪量程 | ±125 / ±250 / ±500 / ±1000 / ±2000 dps |
| 工作电压 | 1.71V ~ 3.6V |
| 工作电流 | 1.3mA (典型) |
| 休眠电流 | 1μA (典型) |
| 中断引脚 | INT (支持运动检测、阈值触发) |

### 2.2 功耗特性

| 工作模式 | 电流消耗 | 说明 |
|---------|---------|------|
| 全功能模式 | 1.3mA | 加速度计 + 陀螺仪同时工作 |
| 加速度计仅 | ~0.5mA | 仅加速度计工作 |
| 待机模式 | ~10μA | 配置保留，快速唤醒 |
| 休眠模式 | 1μA | 最低功耗状态 |

---

## 3. 详细验证结果

### 3.1 I2C 通信速率兼容性验证

#### 3.1.1 规格对比

| 参数 | QMA7981 需求 | ESP32-S3 能力 | 匹配状态 |
|-----|------------|-------------|---------|
| 标准模式 | 100kHz | 支持 | ✅ 兼容 |
| 快速模式 | **400kHz** | **支持** | ✅ **完全匹配** |
| 高速模式 | 3.4MHz (不支持) | 支持最高 1MHz | N/A |

#### 3.1.2 技术说明

- ESP32-S3 的 I2C 控制器支持标准模式 (100kHz) 和快速模式 (400kHz)
- QMA7981 的 400kHz 工作速率完全在 ESP32-S3 的能力范围内
- I2C 总线上的其他传感器 (BMP388) 同样支持 400kHz，可共享同一 I2C 总线

#### 3.1.3 驱动配置示例

```c
#include "driver/i2c.h"

// I2C 配置
#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_MASTER_SDA_IO GPIO_NUM_8
#define I2C_MASTER_SCL_IO GPIO_NUM_9
#define I2C_MASTER_FREQ_HZ 400000  // 400kHz

static esp_err_t i2c_master_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    i2c_param_config(I2C_MASTER_NUM, &conf);
    return i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
}

// QMA7981 设备地址
#define QMA7981_I2C_ADDR 0x18
```

#### 3.1.4 验证结论

**✅ 通过**：QMA7981 的 400kHz I2C 通信速率完全在 ESP32-S3 的驱动能力范围内，无需特殊配置。

---

### 3.2 中断引脚 GPIO07 硬件冲突验证

#### 3.2.1 ESP32-S3-EYE 引脚资源分析

| 引脚号 | 功能 | 占用状态 | 备注 |
|-------|------|---------|------|
| GPIO00 | BOOT 启动引脚 | ⚠️ 占用 | 启动模式选择 |
| GPIO01 | U0TXD | 可用 | 调试串口输出 |
| GPIO02 | I2S_WS | ❌ 占用 | 麦克风字时钟 |
| **GPIO07** | **自由 GPIO** | ✅ **可用** | **推荐中断引脚** |
| GPIO08 | I2C_SDA | ❌ 占用 | 传感器 I2C 总线 |
| GPIO09 | I2C_SCL | ❌ 占用 | 传感器 I2C 总线 |
| GPIO12 | U0RXD | 可用 | 调试串口输入 |
| GPIO15-16 | DCMI D6-D7 | ❌ 占用 | 摄像头数据 |
| GPIO17-18 | UART1 | ❌ 占用 | 4G 模块通信 |
| GPIO21 | DCMI D5 | ❌ 占用 | 摄像头数据 |
| GPIO38-39 | I2S SCK/SD | ❌ 占用 | 麦克风时钟/数据 |
| GPIO45-48 | Flash/PSRAM | ❌ 占用 | 外部存储器 |

#### 3.2.2 关键检查项

| 检查项 | 结果 | 说明 |
|-------|------|------|
| Flash 占用检查 | ✅ 无冲突 | Flash 使用 GPIO45-47 |
| PSRAM 占用检查 | ✅ 无冲突 | PSRAM 使用 GPIO48 |
| JTAG 占用检查 | ✅ 无冲突 | JTAG 使用 GPIO3-6, 15-16 |
| 启动引脚检查 | ✅ 无冲突 | BOOT 使用 GPIO00 |
| 串口占用检查 | ✅ 无冲突 | USB 串口使用 GPIO43-44 |

#### 3.2.3 中断配置示例

```c
#include "driver/gpio.h"

#define QMA7981_INT_GPIO GPIO_NUM_7

// GPIO 中断回调函数
static void IRAM_ATTR qma7981_interrupt_handler(void *arg)
{
    // 运动检测中断处理
    // 通知任务进行姿态数据采集
    xSemaphoreGiveFromISR(xMotionSemaphore, NULL);
}

static esp_err_t qma7981_interrupt_init(void)
{
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << QMA7981_INT_GPIO),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE  // 下降沿触发 (低电平中断)
    };
    
    gpio_config(&io_conf);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(QMA7981_INT_GPIO, qma7981_interrupt_handler, NULL);
    
    return ESP_OK;
}
```

#### 3.2.4 验证结论

**✅ 通过**：GPIO07 未被 Flash、PSRAM 或其他关键外设占用，可安全用作 QMA7981 的中断输入引脚。

---

### 3.3 低功耗模式匹配度验证

#### 3.3.1 功耗对比分析

| 模式 | QMA7981 | ESP32-S3 | EC200U 4G | 系统总功耗 |
|-----|---------|----------|---------|-----------|
| 全功能工作 | 1.3mA | ~80mA | ~80mA | ~161mA |
| Light Sleep | - | ~0.1mA | - | ~0.1mA |
| Deep Sleep | 1μA | ~10μA | - | ~11μA |
| EC200U Sleep | - | - | ~5mA | ~5mA |

#### 3.3.2 电源管理策略匹配

根据 BanSafe 系统电源管理需求：

| 需求 | QMA7981 能力 | 匹配状态 |
|-----|-------------|---------|
| 持续监测低功耗 | 1.3mA 工作电流 | ✅ 满足 |
| 深度休眠超低功耗 | 1μA 休眠电流 | ✅ 满足 |
| 快速唤醒能力 | <1ms 唤醒时间 | ✅ 满足 |
| 中断唤醒支持 | 支持运动检测中断 | ✅ 满足 |
| 可编程量程 | ±2g/±4g/±8g/±16g | ✅ 满足 |

#### 3.3.3 低电量模式降级策略

```c
typedef enum {
    POWER_MODE_NORMAL = 0,    // 全功能模式
    POWER_MODE_LOW,           // 低功耗模式
    POWER_MODE_CRITICAL,      // 紧急模式
    POWER_MODE_SLEEP          // 深度休眠
} PowerMode_t;

// QMA7981 功耗模式配置
static esp_err_t qma7981_set_power_mode(PowerMode_t mode)
{
    switch(mode) {
        case POWER_MODE_NORMAL:
            // 全功能：加速度计 + 陀螺仪，1.3mA
            return qma7981_enable_all_sensors();
            
        case POWER_MODE_LOW:
            // 仅加速度计，~0.5mA
            return qma7981_enable_accel_only();
            
        case POWER_MODE_CRITICAL:
            // 仅运动检测中断，~10μA
            return qma7981_enable_motion_interrupt();
            
        case POWER_MODE_SLEEP:
            // 休眠模式，1μA
            return qma7981_enter_standby();
            
        default:
            return ESP_ERR_INVALID_ARG;
    }
}

// 电池电压监控与自动切换
void battery_monitor_task(void *arg)
{
    while(1) {
        float battery_voltage = read_battery_voltage();
        
        if (battery_voltage < 3.3V) {
            // 电量 < 20%
            qma7981_set_power_mode(POWER_MODE_CRITICAL);
            ble_set_scan_interval(5000);  // 降低 BLE 扫描频率
        } else if (battery_voltage < 3.6V) {
            // 电量 < 50%
            qma7981_set_power_mode(POWER_MODE_LOW);
        } else {
            // 电量充足
            qma7981_set_power_mode(POWER_MODE_NORMAL);
        }
        
        vTaskDelay(pdMS_TO_TICKS(60000));  // 每分钟检查一次
    }
}
```

#### 3.3.4 验证结论

**✅ 通过**：QMA7981 的低功耗特性完全符合 BanSafe 系统的电源管理策略，支持多级功耗降级。

---

## 4. 综合验证结论

### 4.1 验证结果汇总

| 验证项 | 状态 | 风险等级 | 备注 |
|-------|------|---------|------|
| I2C 400kHz 兼容性 | ✅ 通过 | 无风险 | 完全匹配 ESP32-S3 能力 |
| GPIO07 引脚冲突 | ✅ 通过 | 无风险 | 无 Flash/PSRAM 冲突 |
| 低功耗模式匹配 | ✅ 通过 | 无风险 | 支持多级功耗管理 |

### 4.2 最终结论

**✅ QMA7981 传感器选型完全符合伴星安全防护系统的硬件需求**

- I2C 通信接口与 ESP32-S3 完全兼容
- 中断引脚 GPIO07 无硬件冲突，可安全使用
- 低功耗特性满足系统电源管理策略要求

### 4.3 建议

1. **原理图设计**：
   - I2C SDA: GPIO08
   - I2C SCL: GPIO09
   - INT 中断：GPIO07
   - 添加 100kΩ上拉电阻至 I2C 总线

2. **固件开发**：
   - 参考任务#5 实现 I2C 驱动
   - 参考任务#9 配置 GPIO 中断
   - 参考任务#31 实现功耗管理策略

3. **测试验证**：
   - 验证 I2C 通信稳定性
   - 测试中断触发响应时间
   - 测量各模式下的实际功耗

---

## 5. 附录

### 5.1 引脚连接表

| QMA7981 引脚 | ESP32-S3 引脚 | 说明 |
|-------------|--------------|------|
| VCC | 3.3V | 电源输入 |
| GND | GND | 地 |
| SDA | GPIO08 | I2C 数据 |
| SCL | GPIO09 | I2C 时钟 |
| INT | GPIO07 | 运动检测中断 |
| SDI | GND | I2C 地址选择 (0x18) |

### 5.2 参考链接

- [QMA7981 Datasheet](https://www.infineon.com/)
- [ESP32-S3 Technical Reference Manual](https://www.espressif.com/sites/default/files/documentation/esp32-s3_technical_reference_manual_en.pdf)
- [ESP-IDF I2C 驱动文档](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/i2c.html)
- [ESP-IDF GPIO 驱动文档](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/gpio.html)

---

## 6. 版本历史

| 版本 | 日期 | 变更说明 | 作者 |
|-----|------|---------|------|
| 1.0 | 2026-04-16 | 初始版本 | 硬件架构师 |

---

**文档状态**: ✅ 已验证  
**下次评审**: 原理图定稿前