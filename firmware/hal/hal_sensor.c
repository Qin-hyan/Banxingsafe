/**
 * @file hal_sensor.c
 * @brief HAL 传感器抽象层 — 占位实现
 *
 * 当前将传感器访问绑定到 QMA7981 驱动。
 * 待 OV2640/麦克风等其他驱动就绪后，再扩展为通用路由层。
 *
 * @author BanSafe Team
 * @version 0.1.0
 * @date 2026-05-13
 */

#include "hal_sensor.h"
#include "../drivers/sensors/qma7981.h"
#include "esp_log.h"

static const char *TAG = "HAL_SENSOR";
static bool s_initialized = false;

esp_err_t hal_sensor_init(void)
{
    if (s_initialized) {
        ESP_LOGW(TAG, "Already initialized");
        return ESP_OK;
    }

    esp_err_t ret = qma7981_init_default();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "qma7981_init_default failed: %s", esp_err_to_name(ret));
        return ret;
    }

    s_initialized = true;
    ESP_LOGI(TAG, "Sensor HAL initialized");
    return ESP_OK;
}

esp_err_t hal_sensor_deinit(void)
{
    if (!s_initialized) {
        return ESP_OK;
    }

    esp_err_t ret = qma7981_deinit();
    if (ret == ESP_OK) {
        s_initialized = false;
    }
    return ret;
}

bool hal_sensor_is_ready(uint32_t sensor_type)
{
    if (!s_initialized) {
        return false;
    }

    /* 当前仅支持加速度计和陀螺仪 */
    if (sensor_type & (HAL_SENSOR_TYPE_ACCEL | HAL_SENSOR_TYPE_GYRO)) {
        return qma7981_is_ready();
    }
    return false;
}

esp_err_t hal_sensor_read_accel(hal_accel_data_t *data)
{
    if (!data) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    qma7981_accel_data_t raw;
    esp_err_t ret = qma7981_read_accel(&raw);
    if (ret != ESP_OK) {
        return ret;
    }

    data->x = raw.x;
    data->y = raw.y;
    data->z = raw.z;
    return ESP_OK;
}

esp_err_t hal_sensor_read_gyro(hal_gyro_data_t *data)
{
    if (!data) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    qma7981_gyro_data_t raw;
    esp_err_t ret = qma7981_read_gyro(&raw);
    if (ret != ESP_OK) {
        return ret;
    }

    data->x = raw.x;
    data->y = raw.y;
    data->z = raw.z;
    return ESP_OK;
}

esp_err_t hal_sensor_read_all(hal_sensor_data_t *data)
{
    if (!data) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret;

    qma7981_accel_data_t raw_accel;
    ret = qma7981_read_accel(&raw_accel);
    if (ret != ESP_OK) {
        return ret;
    }
    data->accel.x = raw_accel.x;
    data->accel.y = raw_accel.y;
    data->accel.z = raw_accel.z;

    qma7981_gyro_data_t raw_gyro;
    ret = qma7981_read_gyro(&raw_gyro);
    if (ret != ESP_OK) {
        return ret;
    }
    data->gyro.x = raw_gyro.x;
    data->gyro.y = raw_gyro.y;
    data->gyro.z = raw_gyro.z;

    /* Timestamp 由调用方通过 FreeRTOS tick 或 esp_timer 补充 */
    data->timestamp_ms = 0;

    return ESP_OK;
}

esp_err_t hal_sensor_config_motion_interrupt(uint8_t threshold, uint16_t duration_ms)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    return qma7981_config_motion_interrupt(threshold, duration_ms);
}