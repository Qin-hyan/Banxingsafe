/**
 * @file hal_sensor.h
 * @brief Hardware Abstraction Layer - 传感器抽象接口
 *
 * 为上层算法与业务层提供统一的传感器访问接口。
 * 所有传感器操作必须通过此 HAL 接口完成，禁止算法层直接调用驱动。
 *
 * @author BanSafe Team
 * @version 0.1.0
 * @date 2026-05-13
 */

#ifndef HAL_SENSOR_H
#define HAL_SENSOR_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 * 宏定义
 *============================================================================*/

/** 传感器类型的枚举（按位掩码设计，支持多传感器融合） */
#define HAL_SENSOR_TYPE_ACCEL       (1 << 0)   /*!< 加速度计 */
#define HAL_SENSOR_TYPE_GYRO        (1 << 1)   /*!< 陀螺仪 */
#define HAL_SENSOR_TYPE_TEMP        (1 << 2)   /*!< 温度传感器 */

/** 传感器状态 */
#define HAL_SENSOR_STATUS_OK        (0)        /*!< 正常 */
#define HAL_SENSOR_STATUS_ERROR     (1)        /*!< 错误 */
#define HAL_SENSOR_STATUS_NOT_INIT  (2)        /*!< 未初始化 */

/*============================================================================
 * 数据结构
 *============================================================================*/

/**
 * @brief 加速度计数据（HAL 统一格式，单位: mg）
 */
typedef struct {
    int16_t x;      /*!< X 轴加速度 */
    int16_t y;      /*!< Y 轴加速度 */
    int16_t z;      /*!< Z 轴加速度 */
} hal_accel_data_t;

/**
 * @brief 陀螺仪数据（HAL 统一格式，单位: mdps）
 */
typedef struct {
    int16_t x;      /*!< X 轴角速度 */
    int16_t y;      /*!< Y 轴角速度 */
    int16_t z;      /*!< Z 轴角速度 */
} hal_gyro_data_t;

/**
 * @brief 融合传感器数据（带时间戳）
 */
typedef struct {
    hal_accel_data_t accel;     /*!< 加速度数据 */
    hal_gyro_data_t  gyro;      /*!< 陀螺仪数据 */
    uint32_t         timestamp_ms; /*!< 时间戳 (ms) */
} hal_sensor_data_t;

/*============================================================================
 * 公共接口
 *============================================================================*/

/**
 * @brief 初始化所有传感器子系统
 *
 * 初始化 I2C 总线并探测所有已连接的传感器设备。
 * 调用后可通过 hal_sensor_is_ready() 查询各传感器状态。
 *
 * @return esp_err_t ESP_OK 成功，其他为错误码
 */
esp_err_t hal_sensor_init(void);

/**
 * @brief 反初始化传感器子系统，释放资源
 *
 * @return esp_err_t ESP_OK 成功，其他为错误码
 */
esp_err_t hal_sensor_deinit(void);

/**
 * @brief 检查指定类型传感器是否就绪
 *
 * @param sensor_type 传感器类型位掩码 (HAL_SENSOR_TYPE_*)
 * @return true 就绪
 * @return false 未就绪
 */
bool hal_sensor_is_ready(uint32_t sensor_type);

/**
 * @brief 读取加速度数据
 *
 * 阻塞式读取，在传感器未就绪时返回 ESP_ERR_INVALID_STATE。
 *
 * @param[out] data 加速度数据输出指针
 * @return esp_err_t ESP_OK 成功，其他为错误码
 */
esp_err_t hal_sensor_read_accel(hal_accel_data_t *data);

/**
 * @brief 读取陀螺仪数据
 *
 * @param[out] data 陀螺仪数据输出指针
 * @return esp_err_t ESP_OK 成功，其他为错误码
 */
esp_err_t hal_sensor_read_gyro(hal_gyro_data_t *data);

/**
 * @brief 读取完整传感器数据（加速度 + 陀螺仪 + 时间戳）
 *
 * @param[out] data 融合数据输出指针
 * @return esp_err_t ESP_OK 成功，其他为错误码
 */
esp_err_t hal_sensor_read_all(hal_sensor_data_t *data);

/**
 * @brief 配置运动检测中断
 *
 * @param threshold 运动检测阈值 (0-255)
 * @param duration_ms 持续时间 (ms)
 * @return esp_err_t ESP_OK 成功，其他为错误码
 */
esp_err_t hal_sensor_config_motion_interrupt(uint8_t threshold, uint16_t duration_ms);

#ifdef __cplusplus
}
#endif

#endif /* HAL_SENSOR_H */