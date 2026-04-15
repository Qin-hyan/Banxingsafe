/**
 * @file main.c
 * @brief BanSafe 固件主程序
 * 
 * ESP32-S3 嵌入式固件入口文件
 * 实现三重判定引擎的核心逻辑
 */

#include <stdio.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"

// 标签
static const char *TAG = "BanSafe";

// 事件组
static EventGroupHandle_t s_event_group;

// 事件标志
#define POSTURE_DETECT_BIT    (1 << 0)
#define AUDIO_DETECT_BIT      (1 << 1)
#define VISUAL_DETECT_BIT     (1 << 2)
#define ALERT_TRIGGERED_BIT   (1 << 3)

/**
 * @brief 初始化 NVS 存储
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
 * @brief 姿态检测任务（阶段 1）
 */
static void posture_detection_task(void *arg)
{
    ESP_LOGI(TAG, "姿态检测任务已启动");
    
    while (1) {
        // TODO: 读取 QMA7981 姿态传感器
        // TODO: 计算姿态评分
        // TODO: 如果评分 >= 40，设置事件标志
        
        vTaskDelay(pdMS_TO_TICKS(100));  // 100ms 采样周期
    }
}

/**
 * @brief 音频分析任务（阶段 2）
 */
static void audio_analysis_task(void *arg)
{
    ESP_LOGI(TAG, "音频分析任务已启动");
    
    while (1) {
        // TODO: 读取 INMP441 麦克风数据
        // TODO: 进行音频特征提取
        // TODO: 如果综合评分 >= 60，设置事件标志
        
        vTaskDelay(pdMS_TO_TICKS(1000));  // 1 秒窗口
    }
}

/**
 * @brief 视觉分析任务（阶段 3）
 */
static void visual_analysis_task(void *arg)
{
    ESP_LOGI(TAG, "视觉分析任务已启动");
    
    while (1) {
        // TODO: 读取 OV2640 摄像头数据
        // TODO: 运行 TFLite Micro 模型推理
        // TODO: 如果综合评分 >= 80，设置事件标志
        
        vTaskDelay(pdMS_TO_TICKS(1000));  // 15 秒窗口
    }
}

/**
 * @brief 警报处理任务
 */
static void alert_handler_task(void *arg)
{
    ESP_LOGI(TAG, "警报处理任务已启动");
    
    while (1) {
        // 等待警报触发事件
        EventBits_t bits = xEventGroupWaitBits(
            s_event_group,
            ALERT_TRIGGERED_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY
        );
        
        if (bits & ALERT_TRIGGERED_BIT) {
            ESP_LOGW(TAG, "🚨 警报已触发！");
            // TODO: 通过 4G 发送警报到云端
            // TODO: 本地蜂鸣器报警
            // TODO: LED 闪烁提示
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/**
 * @brief 三重判定引擎主循环
 */
static void triple_detection_engine(void *arg)
{
    ESP_LOGI(TAG, "三重判定引擎已启动");
    
    while (1) {
        EventBits_t bits = xEventGroupGetCurrent(s_event_group);
        
        // 阶段 1: 姿态检测
        if (bits & POSTURE_DETECT_BIT) {
            ESP_LOGI(TAG, "阶段 1 触发：姿态异常");
            // 唤醒阶段 2
            xEventGroupSetBits(s_event_group, AUDIO_DETECT_BIT);
        }
        
        // 阶段 2: 音频分析
        if (bits & AUDIO_DETECT_BIT) {
            ESP_LOGI(TAG, "阶段 2 触发：音频异常");
            // 唤醒阶段 3
            xEventGroupSetBits(s_event_group, VISUAL_DETECT_BIT);
        }
        
        // 阶段 3: 视觉分析
        if (bits & VISUAL_DETECT_BIT) {
            ESP_LOGI(TAG, "阶段 3 触发：视觉异常");
            // 触发警报
            xEventGroupSetBits(s_event_group, ALERT_TRIGGERED_BIT);
        }
        
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

/**
 * @brief Arduino 风格初始化函数
 */
void app_main(void)
{
    ESP_LOGI(TAG, "=================================");
    ESP_LOGI(TAG, "  BanSafe 固件 v0.4.0");
    ESP_LOGI(TAG, "  ESP32-S3 安全防护系统");
    ESP_LOGI(TAG, "=================================");
    
    // 初始化 NVS
    ESP_ERROR_CHECK(init_nvs());
    
    // 创建事件组
    s_event_group = xEventGroupCreate();
    
    // 创建任务
    xTaskCreate(posture_detection_task, "posture", 4096, NULL, 5, NULL);
    xTaskCreate(audio_analysis_task, "audio", 8192, NULL, 4, NULL);
    xTaskCreate(visual_analysis_task, "visual", 8192, NULL, 3, NULL);
    xTaskCreate(alert_handler_task, "alert", 4096, NULL, 6, NULL);
    xTaskCreate(triple_detection_engine, "engine", 4096, NULL, 5, NULL);
    
    ESP_LOGI(TAG, "所有任务已启动，系统运行中...");
}