/**
 * @file main.c
 * @brief BanSafe 固件主程序
 * 
 * ESP32-S3 嵌入式固件入口文件
 * 实现三重判定引擎的核心逻辑
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/i2c.h"

// 引入 QMA7981 姿态传感器驱动
#include "qma7981.h"

// 标签
static const char *TAG = "BanSafe";

// 事件组
static EventGroupHandle_t s_event_group;

// 事件标志
#define POSTURE_DETECT_BIT    (1 << 0)
#define AUDIO_DETECT_BIT      (1 << 1)
#define VISUAL_DETECT_BIT     (1 << 2)
#define ALERT_TRIGGERED_BIT   (1 << 3)

// 姿态评分阈值
#define POSTURE_SCORE_THRESHOLD     (40)    // 姿态异常阈值

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
 * @brief I2C 总线扫描，检测所有连接的设备
 */
void i2c_scan(void)
{
    i2c_port_t i2c_num = QMA7981_I2C_NUM;
    uint8_t address;
    int count = 0;
    
    ESP_LOGI(TAG, "开始 I2C 总线扫描...");
    
    for (address = 1; address < 127; address++) {
        esp_err_t ret = i2c_master_write_to_device(
            i2c_num,
            address,
            &address,
            0,
            pdMS_TO_TICKS(10)
        );
        
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "检测到设备：0x%02X (%d)", address, address);
            count++;
        } else if (ret == ESP_ERR_TIMEOUT) {
            // 无响应，跳过
        } else {
            ESP_LOGW(TAG, "扫描地址 0x%02X 失败：%s", address, esp_err_to_name(ret));
        }
    }
    
    if (count == 0) {
        ESP_LOGW(TAG, "未检测到任何 I2C 设备！");
    } else {
        ESP_LOGI(TAG, "扫描完成，共检测到 %d 个设备", count);
    }
}

/**
 * @brief 计算姿态评分
 * 
 * @param accel 加速度数据
 * @return int 评分 (0-100)
 */
static int calculate_posture_score(const qma7981_accel_data_t *accel)
{
    // 计算加速度矢量的大小 (单位：mg)
    float accel_magnitude = sqrtf(
        (float)accel->x * accel->x +
        (float)accel->y * accel->y +
        (float)accel->z * accel->z
    );
    
    // 正常状态：接近 1g (1000mg)
    // 异常状态：偏离 1g 越多，评分越高
    float deviation = fabsf(accel_magnitude - 1000.0f);
    
    // 将偏离值映射到 0-100 评分范围
    // 假设最大偏离 2000mg 时评分为 100
    int score = (int)(deviation / 20.0f);
    if (score > 100) score = 100;
    if (score < 0) score = 0;
    
    return score;
}

/**
 * @brief 姿态检测任务（阶段 1）
 */
static void posture_detection_task(void *arg)
{
    qma7981_accel_data_t accel_data;
    int posture_score = 0;
    
    while (1) {
        // 读取 QMA7981 姿态传感器
        if (qma7981_read_accel(&accel_data) == ESP_OK) {
            // 计算姿态评分
            posture_score = calculate_posture_score(&accel_data);
            
            ESP_LOGD(TAG, "姿态数据：X=%d, Y=%d, Z=%d mg | 评分：%d",
                     accel_data.x, accel_data.y, accel_data.z,
                     posture_score);
            
            // 如果评分 >= 阈值，设置事件标志触发下一阶段
            if (posture_score >= POSTURE_SCORE_THRESHOLD) {
                ESP_LOGI(TAG, "姿态异常检测：评分=%d >= 阈值=%d",
                         posture_score, POSTURE_SCORE_THRESHOLD);
                xEventGroupSetBits(s_event_group, POSTURE_DETECT_BIT);
            }
        } else {
            ESP_LOGW(TAG, "读取传感器数据失败");
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));  // 100ms 采样周期
    }
}

/**
 * @brief 音频分析任务（阶段 2）
 */
static void audio_analysis_task(void *arg)
{
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
    while (1) {
        // TODO: 读取 OV2640 摄像头数据
        // TODO: 运行 TFLite Micro 模型推理
        // TODO: 如果综合评分 >= 80，设置事件标志
        
        vTaskDelay(pdMS_TO_TICKS(1000));  // 1 秒窗口
    }
}

/**
 * @brief 警报处理任务
 */
static void alert_handler_task(void *arg)
{
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
    while (1) {
        EventBits_t bits = xEventGroupGetBits(s_event_group);
        
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
    // 初始化 NVS
    ESP_ERROR_CHECK(init_nvs());
    
    // 创建事件组
    s_event_group = xEventGroupCreate();
    
    // I2C 总线扫描，检测所有设备
    i2c_scan();
    
    // 初始化 QMA7981 姿态传感器
    esp_err_t ret = qma7981_init_default();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "QMA7981 初始化失败：%s", esp_err_to_name(ret));
    } else {
        // 读取设备 ID 确认
        uint8_t device_id;
        if (qma7981_read_device_id(&device_id) == ESP_OK) {
            ESP_LOGI(TAG, "QMA7981 设备 ID: 0x%02X", device_id);
        }
    }
    
    // 创建任务
    xTaskCreate(posture_detection_task, "posture", 4096, NULL, 5, NULL);
    xTaskCreate(audio_analysis_task, "audio", 8192, NULL, 4, NULL);
    xTaskCreate(visual_analysis_task, "visual", 8192, NULL, 3, NULL);
    xTaskCreate(alert_handler_task, "alert", 4096, NULL, 6, NULL);
    xTaskCreate(triple_detection_engine, "engine", 4096, NULL, 5, NULL);
}