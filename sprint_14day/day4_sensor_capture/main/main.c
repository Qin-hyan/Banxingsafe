/**
 * @file main.c
 * @brief Day 4: 传感器数据采集 - 主入口
 *
 * 整合 Day2 (Wi-Fi) 和 Day4 (传感器采集) 功能。
 * 采集任务以独立 FreeRTOS 任务运行，数据通过全局变量 g_sensor_data 共享。
 *
 * 调用顺序 (app_main):
 *   1. NVS 初始化
 *   2. day4_sensor_init()     --> 初始化 I2C 和 QMA7981
 *   3. wifi_init_sta()         --> Day 2 Wi-Fi 连接
 *   4. 创建传感器采集任务     --> day4_sensor_capture_service
 *   5. 进入主循环处理数据     --> 读取 g_sensor_data
 *
 * 引脚:
 *   SDA: GPIO8
 *   SCL: GPIO9
 *   I2C 地址: 0x12
 */

#include <stdio.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_err.h>
#include <nvs_flash.h>

/* Day4 传感器模块 */
#include "day4_sensor_capture.h"

/* Day2 Wi-Fi 模块声明 (来自 wifi.c) */
extern void wifi_init_sta(void);

/*============================================================================
 * 日志标签
 *============================================================================*/

static const char *TAG = "Day4_MAIN";

/*============================================================================
 * 资源定义
 *============================================================================*/

/** 传感器采集任务配置 */
#define CAPTURE_TASK_NAME       "sensor_cap"
#define CAPTURE_TASK_STACK      2048            /*!< 堆栈: 2048 字节 */
#define CAPTURE_TASK_PRIORITY   3               /*!< 优先级: 3 (低于主循环) */

/** 主循环任务配置 */
#define MAIN_LOOP_STACK         4096
#define MAIN_LOOP_PRIORITY      5

/*============================================================================
 * NVS 初始化
 *============================================================================*/

static esp_err_t init_nvs(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    return ret;
}

/*============================================================================
 * 主循环任务 - 处理传感器全局数据
 *============================================================================*/

static void main_loop_task(void *arg)
{
    ESP_LOGI(TAG, "主循环启动，等待传感器数据...");

    uint32_t last_print_ms = 0;

    while (1) {
        /* 直接读取全局变量 g_sensor_data (volatile) */
        day4_sensor_data_t data;
        data.accel_x = g_sensor_data.accel_x;
        data.accel_y = g_sensor_data.accel_y;
        data.accel_z = g_sensor_data.accel_z;
        data.timestamp_ms = g_sensor_data.timestamp_ms;
        data.valid = g_sensor_data.valid;

        if (data.valid) {
            /* 每秒打印一次 (避免和采集任务的日志重复) */
            uint32_t now = data.timestamp_ms;
            if (now - last_print_ms >= 1000) {
                ESP_LOGI(TAG, "[MAIN] ACCEL X=%+7d Y=%+7d Z=%+7d (ts=%lu ms)",
                         data.accel_x, data.accel_y, data.accel_z,
                         (unsigned long)data.timestamp_ms);
                last_print_ms = now;
            }

            /* TODO: Day5 在此处添加检测逻辑
             *   if (abs(data.accel_x) > THRESHOLD) { ESP_LOGW(TAG, "ALERT!"); }
             */
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

/*============================================================================
 * app_main - 入口
 *============================================================================*/

void app_main(void)
{
    ESP_LOGI(TAG, "==============================================");
    ESP_LOGI(TAG, "Day 4: 传感器数据采集");
    ESP_LOGI(TAG, "传感器: QMA7981 (I2C 0x12)");
    ESP_LOGI(TAG, "采样率: 10 Hz (100ms 周期)");
    ESP_LOGI(TAG, "==============================================");

    /* 1. 初始化 NVS */
    ESP_LOGI(TAG, "[Step 1] 初始化 NVS...");
    ESP_ERROR_CHECK(init_nvs());
    ESP_LOGI(TAG, "[Step 1] NVS 初始化完成");

    /* 2. Day 4: 初始化传感器 (I2C + QMA7981) */
    ESP_LOGI(TAG, "[Step 2] Day 4: 传感器初始化...");
    esp_err_t ret = day4_sensor_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "[Step 2] 传感器初始化失败: %s", esp_err_to_name(ret));
        ESP_LOGE(TAG, "请检查: 1) 硬件连接 2) I2C 地址 3) 上拉电阻");
        /* 不调用 ESP_ERROR_CHECK，避免无意义重启 */
    } else {
        ESP_LOGI(TAG, "[Step 2] 传感器初始化成功");
    }

    /* 3. Day 2: 初始化 Wi-Fi */
    ESP_LOGI(TAG, "[Step 3] Day 2: Wi-Fi 连接...");
    wifi_init_sta();
    ESP_LOGI(TAG, "[Step 3] Wi-Fi 初始化完成 (后台连接中)");

    /* 4. 创建传感器采集任务 (独立 FreeRTOS 任务) */
    ESP_LOGI(TAG, "[Step 4] 创建传感器采集任务 (堆栈=%d, 优先级=%d)...",
             CAPTURE_TASK_STACK, CAPTURE_TASK_PRIORITY);
    BaseType_t task_ret = xTaskCreate(
        day4_sensor_capture_service,    /* 任务函数 */
        CAPTURE_TASK_NAME,              /* 任务名称 */
        CAPTURE_TASK_STACK,             /* 堆栈大小 */
        NULL,                           /* 参数 */
        CAPTURE_TASK_PRIORITY,          /* 优先级 */
        NULL                            /* 任务句柄 (不需要) */
    );
    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "采集任务创建失败! 堆栈可能不足");
    } else {
        ESP_LOGI(TAG, "[Step 4] 采集任务创建成功");
    }

    /* 5. 进入主循环 (本任务作为主处理循环) */
    ESP_LOGI(TAG, "[Step 5] 进入主循环...");
    main_loop_task(NULL);
}