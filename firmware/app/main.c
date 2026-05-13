/**
 * @file main.c
 * @brief BanSafe 固件主入口 — 应用层（Application Layer）
 *
 * 负责系统初始化、任务创建和顶层调度。
 * 所有传感器访问通过 HAL 接口，所有危险判定通过安全引擎。
 *
 * 架构分层：
 *   应用层 (app) → 算法与业务层 (algorithm) → HAL ← 驱动层 (drivers)
 *
 * @author BanSafe Team
 * @version 0.1.0
 * @date 2026-05-13
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "../hal/hal_sensor.h"
#include "../hal/hal_camera.h"
#include "../hal/hal_audio.h"
#include "../algorithm/safety_engine.h"

/*============================================================================
 * 常量与宏
 *============================================================================*/

static const char *TAG = "BanSafe-App";

/** 主安全监控循环周期 (ms) */
#define SAFETY_MONITOR_PERIOD_MS    (500)

/** 任务栈大小 */
#define TASK_STACK_SAFETY           (8192)
#define TASK_STACK_ALERT            (4096)
#define TASK_STACK_COMM             (8192)

/** 任务优先级 */
#define TASK_PRIO_SAFETY            (5)
#define TASK_PRIO_ALERT             (6)
#define TASK_PRIO_COMM              (4)

/*============================================================================
 * 内部状态
 *============================================================================*/

static bool s_alert_active = false;

/*============================================================================
 * 内部函数声明
 *============================================================================*/

static esp_err_t init_nvs(void);

/*============================================================================
 * 任务函数
 *============================================================================*/

/**
 * @brief 初始化 NVS 闪存存储
 */
static esp_err_t init_nvs(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    return ret;
}

/**
 * @brief 安全监控主任务
 *
 * 周期性调用安全引擎执行三重判定流程。
 * 当检测到危险行为时通知报警任务。
 */
static void safety_monitor_task(void *arg)
{
    ESP_LOGI(TAG, "Safety monitor task started");

    while (1) {
        safety_result_t result;
        esp_err_t ret = safety_engine_run(&result);

        if (ret == ESP_OK && result.alert_triggered) {
            if (!s_alert_active) {
                s_alert_active = true;
                ESP_LOGW(TAG, "ALERT: Dangerous behaviour detected! "
                         "Stage=%d, P=%d, A=%d, V=%d",
                         result.stage,
                         result.posture_score,
                         result.audio_score,
                         result.visual_score);
            }
        } else {
            if (s_alert_active) {
                s_alert_active = false;
                ESP_LOGI(TAG, "Alert cleared");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(SAFETY_MONITOR_PERIOD_MS));
    }
}

/**
 * @brief 报警处理任务
 *
 * 响应安全监控触发的报警，执行蜂鸣器/LED/云端上报等操作。
 */
static void alert_task(void *arg)
{
    ESP_LOGI(TAG, "Alert handler task started");

    while (1) {
        if (s_alert_active) {
            /*
             * TODO: 报警动作
             * - GPIO 控制蜂鸣器
             * - GPIO 控制 LED 闪烁
             * - 通过 Wi-Fi/4G 发送报警消息到云端
             */
            ESP_LOGW(TAG, "Alert is ACTIVE — implement buzzer/LED/cloud actions here");
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

/**
 * @brief 通信调度任务
 *
 * 负责 Wi-Fi 连接维护、数据上报、OTA 等。
 */
static void communication_task(void *arg)
{
    ESP_LOGI(TAG, "Communication task started");

    /*
     * TODO: 规划中功能
     * - Wi-Fi 初始化和连接保持
     * - MQTT/HTTP 数据上报
     * - OTA 固件升级
     * - BLE 室内定位
     */
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

/**
 * @brief 应用主入口
 *
 * 初始化顺序：
 *   1. NVS 存储
 *   2. HAL 各子系统（传感器、摄像头、音频）
 *   3. 安全引擎
 *   4. FreeRTOS 任务创建
 */
void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  BanSafe Firmware v0.1.0 (refactored)");
    ESP_LOGI(TAG, "  Platform: ESP32-S3-EYE");
    ESP_LOGI(TAG, "========================================");

    /* ---- 初始化 NVS ---- */
    ESP_ERROR_CHECK(init_nvs());
    ESP_LOGI(TAG, "NVS initialized");

    /* ---- 初始化 HAL 子系统 ---- */
    esp_err_t ret;

    ret = hal_sensor_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "hal_sensor_init failed: %s", esp_err_to_name(ret));
    }

    /*
     * 摄像头和音频 HAL 当前为占位实现（返回 ESP_ERR_NOT_SUPPORTED）。
     * 不阻塞启动流程，仅记录警告。
     */
    ret = hal_camera_init(NULL);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "hal_camera_init not available: %s", esp_err_to_name(ret));
    }

    ret = hal_audio_init(NULL);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "hal_audio_init not available: %s", esp_err_to_name(ret));
    }

    /* ---- 初始化安全引擎 ---- */
    ret = safety_engine_init();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "safety_engine_init: %s", esp_err_to_name(ret));
    }

    /* ---- 创建 FreeRTOS 任务 ---- */
    xTaskCreate(safety_monitor_task, "safety_mon",
                TASK_STACK_SAFETY, NULL, TASK_PRIO_SAFETY, NULL);

    xTaskCreate(alert_task, "alert",
                TASK_STACK_ALERT, NULL, TASK_PRIO_ALERT, NULL);

    xTaskCreate(communication_task, "comm",
                TASK_STACK_COMM, NULL, TASK_PRIO_COMM, NULL);

    ESP_LOGI(TAG, "All tasks created. System running.");
}