/**
 * @file qma7981.h
 * @brief QMA7981 姿态传感器驱动头文件
 * 
 * 6 轴 IMU (加速度计 + 陀螺仪) 驱动接口
 * 适用于 ESP32-S3 嵌入式系统
 */

#ifndef QMA7981_H
#define QMA7981_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "driver/i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 * 宏定义
 *============================================================================*/

/** I2C 配置 */
#define QMA7981_I2C_ADDR            (0x18)          /*!< I2C 默认地址 (SDI 接地) */
#define QMA7981_I2C_FREQ            (400000)        /*!< I2C 频率 400kHz */
#define QMA7981_I2C_NUM             (I2C_NUM_0)     /*!< I2C 控制器编号 */
#define QMA7981_I2C_SDA_GPIO        (GPIO_NUM_8)    /*!< I2C SDA 引脚 */
#define QMA7981_I2C_SCL_GPIO        (GPIO_NUM_9)    /*!< I2C SCL 引脚 */

/** 中断引脚配置 */
#define QMA7981_INT_GPIO            (GPIO_NUM_7)    /*!< 中断输入引脚 */

/** 设备 ID */
#define QMA7981_DEVID_ADDR          (0x0F)          /*!< 设备 ID 寄存器地址 */
#define QMA7981_DEVID_VALUE         (0x01)          /*!< QMA7981 设备 ID 值 */

/** 控制寄存器 */
#define QMA7981_CTRL_ADDR           (0x4B)          /*!< 控制寄存器地址 */
#define QMA7981_CTRL_RESET          (0xB6)          /*!< 软复位值 */

/** 数据寄存器 */
#define QMA7981_ACC_X_LSB_ADDR      (0x02)          /*!< 加速度 X 低字节 */
#define QMA7981_GYRO_X_LSB_ADDR     (0x08)          /*!< 陀螺仪 X 低字节 */

/** 量程配置 */
#define QMA7981_RANGE_2G            (0)             /*!< ±2g */
#define QMA7981_RANGE_4G            (1)             /*!< ±4g */
#define QMA7981_RANGE_8G            (2)             /*!< ±8g */
#define QMA7981_RANGE_16G           (3)             /*!< ±16g */

/** 带宽配置 */
#define QMA7981_BW_7_81HZ           (0)             /*!< 7.81Hz */
#define QMA7981_BW_15_63HZ          (1)             /*!< 15.63Hz */
#define QMA7981_BW_31_25HZ          (2)             /*!< 31.25Hz */
#define QMA7981_BW_62_5HZ           (3)             /*!< 62.5Hz */
#define QMA7981_BW_125HZ            (4)             /*!< 125Hz */
#define QMA7981_BW_250HZ            (5)             /*!< 250Hz */
#define QMA7981_BW_500HZ            (6)             /*!< 500Hz */
#define QMA7981_BW_1000HZ           (7)             /*!< 1000Hz */

/** 功耗模式 */
#define QMA7981_MODE_STANDBY        (0)             /*!< 待机模式 */
#define QMA7981_MODE_ACCEL          (1)             /*!< 加速度计模式 */
#define QMA7981_MODE_GYRO           (2)             /*!< 陀螺仪模式 */
#define QMA7981_MODE_ACCEL_GYRO     (3)             /*!< 加速度计 + 陀螺仪模式 */

/*============================================================================
 * 数据结构
 *============================================================================*/

/**
 * @brief QMA7981 配置结构体
 */
typedef struct {
    i2c_port_t i2c_num;                 /*!< I2C 控制器编号 */
    gpio_num_t sda_gpio;                /*!< I2C SDA 引脚 */
    gpio_num_t scl_gpio;                /*!< I2C SCL 引脚 */
    gpio_num_t int_gpio;                /*!< 中断引脚 */
    uint8_t accel_range;                /*!< 加速度量程 0-3 */
    uint8_t accel_bandwidth;            /*!< 加速度带宽 0-7 */
    uint8_t gyro_range;                 /*!< 陀螺仪量程 0-4 */
    uint8_t mode;                       /*!< 工作模式 */
} qma7981_config_t;

/**
 * @brief 加速度数据结构体
 */
typedef struct {
    int16_t x;                          /*!< X 轴加速度 (mg) */
    int16_t y;                          /*!< Y 轴加速度 (mg) */
    int16_t z;                          /*!< Z 轴加速度 (mg) */
} qma7981_accel_data_t;

/**
 * @brief 陀螺仪数据结构体
 */
typedef struct {
    int16_t x;                          /*!< X 轴角速度 (mdps) */
    int16_t y;                          /*!< Y 轴角速度 (mdps) */
    int16_t z;                          /*!< Z 轴角速度 (mdps) */
} qma7981_gyro_data_t;

/**
 * @brief 完整姿态数据结构体
 */
typedef struct {
    qma7981_accel_data_t accel;         /*!< 加速度数据 */
    qma7981_gyro_data_t gyro;           /*!< 陀螺仪数据 */
    uint32_t timestamp_ms;              /*!< 时间戳 (ms) */
} qma7981_sensor_data_t;

/*============================================================================
 * 函数声明
 *============================================================================*/

/**
 * @brief 初始化 QMA7981 传感器
 * 
 * @param config 配置结构体指针
 * @return esp_err_t ESP_OK 成功，其他为错误码
 */
esp_err_t qma7981_init(const qma7981_config_t *config);

/**
 * @brief 使用默认配置初始化 QMA7981
 * 
 * @return esp_err_t ESP_OK 成功，其他为错误码
 */
esp_err_t qma7981_init_default(void);

/**
 * @brief 读取设备 ID
 * 
 * @param device_id 设备 ID 输出指针
 * @return esp_err_t ESP_OK 成功，其他为错误码
 */
esp_err_t qma7981_read_device_id(uint8_t *device_id);

/**
 * @brief 检查设备是否就绪
 * 
 * @return bool true 设备就绪，false 设备未就绪
 */
bool qma7981_is_ready(void);

/**
 * @brief 软复位传感器
 * 
 * @return esp_err_t ESP_OK 成功，其他为错误码
 */
esp_err_t qma7981_reset(void);

/**
 * @brief 设置加速度计量程
 * 
 * @param range 量程配置 (QMA7981_RANGE_2G/4G/8G/16G)
 * @return esp_err_t ESP_OK 成功，其他为错误码
 */
esp_err_t qma7981_set_accel_range(uint8_t range);

/**
 * @brief 设置加速度计带宽
 * 
 * @param bandwidth 带宽配置 (QMA7981_BW_7_81HZ ~ QMA7981_BW_1000HZ)
 * @return esp_err_t ESP_OK 成功，其他为错误码
 */
esp_err_t qma7981_set_accel_bandwidth(uint8_t bandwidth);

/**
 * @brief 设置陀螺仪量程
 * 
 * @param range 量程配置 (0-4 对应 ±125/±250/±500/±1000/±2000 dps)
 * @return esp_err_t ESP_OK 成功，其他为错误码
 */
esp_err_t qma7981_set_gyro_range(uint8_t range);

/**
 * @brief 设置工作模式
 * 
 * @param mode 工作模式 (QMA7981_MODE_*)
 * @return esp_err_t ESP_OK 成功，其他为错误码
 */
esp_err_t qma7981_set_mode(uint8_t mode);

/**
 * @brief 读取加速度数据
 * 
 * @param data 加速度数据输出指针
 * @return esp_err_t ESP_OK 成功，其他为错误码
 */
esp_err_t qma7981_read_accel(qma7981_accel_data_t *data);

/**
 * @brief 读取陀螺仪数据
 * 
 * @param data 陀螺仪数据输出指针
 * @return esp_err_t ESP_OK 成功，其他为错误码
 */
esp_err_t qma7981_read_gyro(qma7981_gyro_data_t *data);

/**
 * @brief 读取完整传感器数据
 * 
 * @param data 传感器数据输出指针
 * @return esp_err_t ESP_OK 成功，其他为错误码
 */
esp_err_t qma7981_read_all_data(qma7981_sensor_data_t *data);

/**
 * @brief 配置运动检测中断
 * 
 * @param threshold 运动检测阈值 (0-255)
 * @param duration 持续时间 (ms)
 * @return esp_err_t ESP_OK 成功，其他为错误码
 */
esp_err_t qma7981_config_motion_interrupt(uint8_t threshold, uint16_t duration);

/**
 * @brief 初始化 GPIO 中断
 * 
 * @return esp_err_t ESP_OK 成功，其他为错误码
 */
esp_err_t qma7981_interrupt_init(void);

/**
 * @brief 移除 GPIO 中断
 * 
 * @return esp_err_t ESP_OK 成功，其他为错误码
 */
esp_err_t qma7981_interrupt_remove(void);

/**
 * @brief 进入待机模式 (低功耗)
 * 
 * @return esp_err_t ESP_OK 成功，其他为错误码
 */
esp_err_t qma7981_enter_standby(void);

/**
 * @brief 进入深度休眠模式 (最低功耗)
 * 
 * @return esp_err_t ESP_OK 成功，其他为错误码
 */
esp_err_t qma7981_enter_deep_sleep(void);

/**
 * @brief 退出休眠模式
 * 
 * @return esp_err_t ESP_OK 成功，其他为错误码
 */
esp_err_t qma7981_wake_up(void);

/**
 * @brief 去初始化 QMA7981，释放资源
 * 
 * @return esp_err_t ESP_OK 成功，其他为错误码
 */
esp_err_t qma7981_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* QMA7981_H */