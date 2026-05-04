/**
 * @file day4_sensor_capture.h
 * @brief Day 4: 传感器数据采集模块头文件
 *
 * 封装 QMA7981 加速度计的初始化和数据采集功能。
 * 基于 ESP32-S3-EYE 板载传感器 (I2C 地址 0x12, SDA=GPIO8, SCL=GPIO9)。
 *
 * 依赖:
 *   - driver (I2C)
 *   - esp_timer
 *   - freertos
 */

#ifndef DAY4_SENSOR_CAPTURE_H
#define DAY4_SENSOR_CAPTURE_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 * 类型定义
 *============================================================================*/

/**
 * @brief 传感器数据包 (三轴加速度 + 时间戳)
 */
typedef struct {
    int16_t accel_x;        /*!< X 轴加速度原始值 (LSB) */
    int16_t accel_y;        /*!< Y 轴加速度原始值 (LSB) */
    int16_t accel_z;        /*!< Z 轴加速度原始值 (LSB) */
    uint32_t timestamp_ms;  /*!< 采集时间戳 (毫秒) */
    bool valid;             /*!< 数据有效标志 */
} day4_sensor_data_t;

/*============================================================================
 * 全局变量 (供主任务读取)
 *============================================================================*/

/** 最新传感器数据，由 day4_sensor_capture 周期更新 */
extern volatile day4_sensor_data_t g_sensor_data;

/*============================================================================
 * 函数声明
 *============================================================================*/

/**
 * @brief Day 4 传感器初始化
 *
 * 初始化 I2C 总线 (GPIO8/GPIO9)，配置 QMA7981 传感器进入主动采集模式。
 * 调用方式与 Day2 的 wifi_init() 一致，由 app_main 顺序调用。
 *
 * @return
 *   - ESP_OK: 初始化成功
 *   - 其它: 对应 ESP-IDF 错误码
 */
esp_err_t day4_sensor_init(void);

/**
 * @brief Day 4 传感器数据采集服务 (独立任务入口)
 *
 * 以 10Hz 频率轮询读取 QMA7981 加速度数据，
 * 更新全局变量 g_sensor_data，并输出日志。
 * 设计为在 FreeRTOS 任务中调用，内部包含无限循环。
 *
 * @param arg 任务参数 (未使用)
 */
void day4_sensor_capture_service(void *arg);

/**
 * @brief 读取一次传感器数据 (非阻塞)
 *
 * @param data 输出数据指针
 * @return
 *   - ESP_OK: 成功
 *   - 其它: 对应错误码
 */
esp_err_t day4_sensor_read_once(day4_sensor_data_t *data);

/**
 * @brief 读取 QMA7981 设备 ID
 *
 * @param id 输出设备 ID
 * @return esp_err_t
 */
esp_err_t day4_sensor_read_device_id(uint8_t *id);

#ifdef __cplusplus
}
#endif

#endif /* DAY4_SENSOR_CAPTURE_H */