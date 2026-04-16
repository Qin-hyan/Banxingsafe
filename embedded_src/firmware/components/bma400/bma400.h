/**
 * @file bma400.h
 * @brief Bosch BMA400 低功耗加速度计驱动头文件
 * 
 * 12 位三轴加速度计
 * 适用于 ESP32-S3 嵌入式系统
 */

#ifndef BMA400_H
#define BMA400_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "driver/i2c.h"
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 * 宏定义
 *============================================================================*/

/** I2C 配置 */
#define BMA400_I2C_ADDR             (0x12)          /*!< I2C 地址 (ESP32-S3-EYE SUB V1.1 板载) */
#define BMA400_I2C_FREQ             (400000)        /*!< I2C 频率 400kHz */
#define BMA400_I2C_NUM              (I2C_NUM_0)     /*!< I2C 控制器编号 */
#define BMA400_I2C_SDA_GPIO         (GPIO_NUM_4)    /*!< I2C SDA 引脚 */
#define BMA400_I2C_SCL_GPIO         (GPIO_NUM_5)    /*!< I2C SCL 引脚 */

/** 中断引脚配置 */
#define BMA400_INT_GPIO             (GPIO_NUM_7)    /*!< 中断输入引脚 */

/** 设备 ID */
#define BMA400_CHIP_ID_ADDR         (0x00)          /*!< 芯片 ID 寄存器地址 */
#define BMA400_CHIP_ID_VALUE        (0x90)          /*!< BMA400 芯片 ID 值 */

/** 寄存器地址 */
#define BMA400_REG_CHIP_ID          (0x00)          /*!< 芯片 ID */
#define BMA400_REG_ERR_REG          (0x02)          /*!< 错误寄存器 */
#define BMA400_REG_STATUS           (0x03)          /*!< 状态寄存器 */
#define BMA400_REG_ACC_X_L          (0x04)          /*!< 加速度 X 低字节 */
#define BMA400_REG_ACC_X_H          (0x05)          /*!< 加速度 X 高字节 */
#define BMA400_REG_ACC_Y_L          (0x06)          /*!< 加速度 Y 低字节 */
#define BMA400_REG_ACC_Y_H          (0x07)          /*!< 加速度 Y 高字节 */
#define BMA400_REG_ACC_Z_L          (0x08)          /*!< 加速度 Z 低字节 */
#define BMA400_REG_ACC_Z_H          (0x09)          /*!< 加速度 Z 高字节 */
#define BMA400_REG_TEMP             (0x0A)          /*!< 温度 */
#define BMA400_REG_STEP_CNT_L       (0x0C)          /*!< 步数计数器低字节 */
#define BMA400_REG_STEP_CNT_H       (0x0D)          /*!< 步数计数器高字节 */
#define BMA400_REG_INT_STAT_0       (0x0E)          /*!< 中断状态 0 */
#define BMA400_REG_INT_STAT_1       (0x0F)          /*!< 中断状态 1 */
#define BMA400_REG_FIFO_L           (0x10)          /*!< FIFO 数据低字节 */
#define BMA400_REG_FIFO_H           (0x11)          /*!< FIFO 数据高字节 */
#define BMA400_REG_FIFO_W_L         (0x12)          /*!< FIFO 字数低字节 */
#define BMA400_REG_FIFO_W_H         (0x13)          /*!< FIFO 字数高字节 */
#define BMA400_REG_FIFO_D_L         (0x14)          /*!< FIFO 下载低字节 */
#define BMA400_REG_FIFO_D_H         (0x15)          /*!< FIFO 下载高字节 */
#define BMA400_REG_INT1_IO_CTRL     (0x16)          /*!< INT1 IO 控制 */
#define BMA400_REG_INT2_IO_CTRL     (0x17)          /*!< INT2 IO 控制 */
#define BMA400_REG_INT_LATCH        (0x18)          /*!< 中断锁存 */
#define BMA400_REG_INT_MAP_0        (0x19)          /*!< 中断映射 0 */
#define BMA400_REG_INT_MAP_1        (0x1A)          /*!< 中断映射 1 */
#define BMA400_REG_INT_MAP_2        (0x1B)          /*!< 中断映射 2 */
#define BMA400_REG_DATA_CTRL        (0x20)          /*!< 数据控制 */
#define BMA400_REG_RANGE            (0x21)          /*!< 量程配置 */
#define BMA400_REG_ODR              (0x22)          /*!< 输出数据率 */
#define BMA400_REG_ACC_CONF         (0x23)          /*!< 加速度计配置 */
#define BMA400_REG_PWR_CONF         (0x24)          /*!< 功耗配置 */
#define BMA400_REG_PWR_CTRL         (0x25)          /*!< 功耗控制 */
#define BMA400_REG_CMD              (0x26)          /*!< 命令寄存器 */

/** 量程配置 */
#define BMA400_RANGE_2G             (0x01)          /*!< ±2g */
#define BMA400_RANGE_4G             (0x02)          /*!< ±4g */
#define BMA400_RANGE_8G             (0x03)          /*!< ±8g */
#define BMA400_RANGE_16G            (0x00)          /*!< ±16g */

/** 输出数据率 (ODR) */
#define BMA400_ODR_100HZ            (0x09)          /*!< 100Hz */
#define BMA400_ODR_50HZ             (0x08)          /*!< 50Hz */
#define BMA400_ODR_25HZ             (0x07)          /*!< 25Hz */
#define BMA400_ODR_12_5HZ           (0x06)          /*!< 12.5Hz */
#define BMA400_ODR_6_25HZ           (0x05)          /*!< 6.25Hz */

/** 分辨率配置 */
#define BMA400_RES_12BIT            (0x02)          /*!< 12 位分辨率 */
#define BMA400_RES_14BIT            (0x01)          /*!< 14 位分辨率 */
#define BMA400_RES_16BIT            (0x00)          /*!< 16 位分辨率 */

/** 工作模式 */
#define BMA400_MODE_NORMAL          (0x01)          /*!< 正常模式 */
#define BMA400_MODE_LOW_POWER       (0x00)          /*!< 低功耗模式 */
#define BMA400_MODE_SUSPEND         (0x02)          /*!< 暂停模式 */

/** 数据结构 */
typedef struct {
    int16_t x;                          /*!< X 轴加速度 (mg) */
    int16_t y;                          /*!< Y 轴加速度 (mg) */
    int16_t z;                          /*!< Z 轴加速度 (mg) */
} bma400_accel_data_t;

/** 配置结构 */
typedef struct {
    i2c_port_t i2c_num;                 /*!< I2C 控制器编号 */
    gpio_num_t sda_gpio;                /*!< SDA 引脚 */
    gpio_num_t scl_gpio;                /*!< SCL 引脚 */
    gpio_num_t int_gpio;                /*!< 中断引脚 */
    uint8_t range;                      /*!< 量程配置 */
    uint8_t odr;                        /*!< 输出数据率 */
    uint8_t resolution;                 /*!< 分辨率 */
    uint8_t mode;                       /*!< 工作模式 */
} bma400_config_t;

/** 函数声明 */

/**
 * @brief 初始化 BMA400
 * @param config 配置参数
 * @return ESP_OK 成功，否则失败
 */
esp_err_t bma400_init(const bma400_config_t *config);

/**
 * @brief 使用默认配置初始化 BMA400
 * @return ESP_OK 成功，否则失败
 */
esp_err_t bma400_init_default(void);

/**
 * @brief 读取芯片 ID
 * @param chip_id 返回的芯片 ID
 * @return ESP_OK 成功，否则失败
 */
esp_err_t bma400_read_chip_id(uint8_t *chip_id);

/**
 * @brief 检查设备是否就绪
 * @return true 就绪，false 未就绪
 */
bool bma400_is_ready(void);

/**
 * @brief 读取加速度数据
 * @param data 返回的加速度数据
 * @return ESP_OK 成功，否则失败
 */
esp_err_t bma400_read_accel(bma400_accel_data_t *data);

/**
 * @brief 设置量程
 * @param range 量程配置
 * @return ESP_OK 成功，否则失败
 */
esp_err_t bma400_set_range(uint8_t range);

/**
 * @brief 设置输出数据率
 * @param odr 输出数据率配置
 * @return ESP_OK 成功，否则失败
 */
esp_err_t bma400_set_odr(uint8_t odr);

/**
 * @brief 设置工作模式
 * @param mode 工作模式
 * @return ESP_OK 成功，否则失败
 */
esp_err_t bma400_set_mode(uint8_t mode);

/**
 * @brief 进入暂停模式
 * @return ESP_OK 成功，否则失败
 */
esp_err_t bma400_suspend(void);

/**
 * @brief 从暂停模式恢复
 * @return ESP_OK 成功，否则失败
 */
esp_err_t bma400_resume(void);

/**
 * @brief 去初始化 BMA400
 * @return ESP_OK 成功，否则失败
 */
esp_err_t bma400_deinit(void);

/**
 * @brief 读取原始寄存器值（调试用）
 * @param reg 寄存器地址
 * @param value 返回的值
 * @return ESP_OK 成功，否则失败
 */
esp_err_t bma400_read_reg(uint8_t reg, uint8_t *value);

/**
 * @brief 写入原始寄存器值（调试用）
 * @param reg 寄存器地址
 * @param value 值
 * @return ESP_OK 成功，否则失败
 */
esp_err_t bma400_write_reg(uint8_t reg, uint8_t value);

#ifdef __cplusplus
}
#endif

#endif /* BMA400_H */
