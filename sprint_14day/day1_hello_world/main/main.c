/**
 * @file main.c
 * @brief Day 1 任务：Hello World
 * 
 * 验证 ESP32-S3-EYE 环境配置正确
 * 串口打印日志输出
 */

#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_system.h"
#include "esp_log.h"
#include "esp_chip_info.h"
#include "esp_efuse_table.h"
#include "esp_mac.h"

static const char *TAG = "Day1_HelloWorld";

void app_main(void)
{
    ESP_LOGI(TAG, "=================================");
    ESP_LOGI(TAG, "  伴星安全防护系统 - Day 1 任务");
    ESP_LOGI(TAG, "  Hello World 测试");
    ESP_LOGI(TAG, "=================================");
    ESP_LOGI(TAG, "ESP32-S3-EYE 环境配置成功!");
    
    // 获取芯片信息
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    ESP_LOGI(TAG, "芯片型号：ESP32-S3");
    ESP_LOGI(TAG, "CPU 核心数：%d", chip_info.cores);
    ESP_LOGI(TAG, "特性：WiFi+BT");
    
    // 获取 MAC 地址
    uint8_t mac_addr[6];
    esp_efuse_mac_get_default(mac_addr);
    ESP_LOGI(TAG, "MAC 地址：%02X:%02X:%02X:%02X:%02X:%02X", 
             mac_addr[0], mac_addr[1], mac_addr[2], 
             mac_addr[3], mac_addr[4], mac_addr[5]);
    
    ESP_LOGI(TAG, "设备启动完成，开始运行...");
    
    // 持续打印心跳日志
    while (1) {
        ESP_LOGI(TAG, "心跳 - 系统运行正常 [%lu ms]", xTaskGetTickCount() * portTICK_PERIOD_MS);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}