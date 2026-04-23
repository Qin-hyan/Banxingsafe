/*
 * Day 2 - Wi-Fi 连接测试
 * 伴星安全防护系统
 * 
 * 功能：
 * 1. 初始化 Wi-Fi 模块
 * 2. 连接到配置的 Wi-Fi 网络
 * 3. 显示连接状态和网络信息
 */

#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_netif.h"

// Wi-Fi 配置 - 请修改为实际的 Wi-Fi 名称和密码
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// 事件组位定义
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

static const char *TAG = "Day2_WiFi";
static EventGroupHandle_t s_wifi_event_group;

static int s_retry_num = 0;

/* Wi-Fi 事件处理回调 */
static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        ESP_LOGI(TAG, "Wi-Fi 已启动，正在连接...");
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (s_retry_num < 5)
        {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "连接失败，重试次数：%d", s_retry_num);
        }
        else
        {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            ESP_LOGW(TAG, "连接失败，已达到最大重试次数");
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "获取到 IP 地址：" IPSTR, IP2STR(&event->ip_info.ip));
        ESP_LOGI(TAG, "子网掩码：" IPSTR, IP2STR(&event->ip_info.netmask));
        ESP_LOGI(TAG, "网关：" IPSTR, IP2STR(&event->ip_info.gw));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

/* 初始化 Wi-Fi */
static void wifi_init(void)
{
    s_wifi_event_group = xEventGroupCreate();

    // 初始化 NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_LOGW(TAG, "NVS 分区溢出，正在擦除...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "初始化 TCP/IP 协议栈");

    // 创建网络接口
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    // 初始化 Wi-Fi (ESP-IDF v5.x 使用默认配置)
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // 注册事件处理器
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        NULL));

    // 设置 Wi-Fi 模式为 STA
    wifi_config_t wifi_config = {0};
    strncpy((char *)wifi_config.sta.ssid, WIFI_SSID, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char *)wifi_config.sta.password, WIFI_PASSWORD, sizeof(wifi_config.sta.password) - 1);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.pmf_cfg.capable = true;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Wi-Fi 配置完成，SSID: %s", WIFI_SSID);
}

/* 等待 Wi-Fi 连接 */
static esp_err_t wait_wifi_connected(void)
{
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           pdMS_TO_TICKS(10000)); // 等待 10 秒

    if (bits & WIFI_CONNECTED_BIT)
    {
        ESP_LOGI(TAG, "Wi-Fi 连接成功");
        return ESP_OK;
    }
    else if (bits & WIFI_FAIL_BIT)
    {
        ESP_LOGE(TAG, "Wi-Fi 连接失败");
        return ESP_FAIL;
    }
    else
    {
        ESP_LOGW(TAG, "Wi-Fi 连接超时");
        return ESP_ERR_TIMEOUT;
    }
}

/* 显示 Wi-Fi 状态信息 */
static void show_wifi_status(void)
{
    wifi_ap_record_t ap_info;
    memset(&ap_info, 0, sizeof(wifi_ap_record_t));
    
    esp_err_t err = esp_wifi_sta_get_ap_info(&ap_info);
    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "信号强度 (RSSI): %d dBm", ap_info.rssi);
        ESP_LOGI(TAG, "信道：%d", ap_info.primary);
        
        // 计算信号质量百分比
        int quality = (ap_info.rssi + 100) * 100 / 40;
        if (quality > 100) quality = 100;
        if (quality < 0) quality = 0;
        ESP_LOGI(TAG, "信号质量：%d%%", quality);
    }

    // 显示 MAC 地址
    uint8_t mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, mac);
    ESP_LOGI(TAG, "STA MAC 地址：%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

void app_main(void)
{
    ESP_LOGI(TAG, "=================================");
    ESP_LOGI(TAG, "  伴星安全防护系统 - Day 2 任务");
    ESP_LOGI(TAG, "  Wi-Fi 连接测试");
    ESP_LOGI(TAG, "=================================");

    // 检查 Wi-Fi 配置
    if (strcmp(WIFI_SSID, "YOUR_WIFI_SSID") == 0)
    {
        ESP_LOGW(TAG, "⚠️  请修改代码中的 Wi-Fi 配置!");
        ESP_LOGW(TAG, "   将 WIFI_SSID 和 WIFI_PASSWORD 替换为实际值");
        ESP_LOGI(TAG, "设备启动完成，等待配置...");
        
        // 创建心跳任务
        while (1)
        {
            ESP_LOGI(TAG, "心跳 - 等待 Wi-Fi 配置 [系统运行中]");
            vTaskDelay(pdMS_TO_TICKS(5000));
        }
    }

    // 初始化 Wi-Fi
    ESP_LOGI(TAG, "正在初始化 Wi-Fi 模块...");
    wifi_init();

    // 等待连接
    esp_err_t result = wait_wifi_connected();

    if (result == ESP_OK)
    {
        // 显示连接状态
        show_wifi_status();
        
        ESP_LOGI(TAG, "Wi-Fi 连接测试完成!");
        ESP_LOGI(TAG, "设备已准备好进行网络通信");
        
        // 创建心跳任务
        while (1)
        {
            ESP_LOGI(TAG, "心跳 - Wi-Fi 连接正常 [系统运行中]");
            vTaskDelay(pdMS_TO_TICKS(5000));
        }
    }
    else
    {
        ESP_LOGE(TAG, "Wi-Fi 连接失败，请检查配置");
        
        // 错误状态下的心跳
        while (1)
        {
            ESP_LOGE(TAG, "心跳 - Wi-Fi 连接失败 [请检查配置]");
            vTaskDelay(pdMS_TO_TICKS(5000));
        }
    }
}