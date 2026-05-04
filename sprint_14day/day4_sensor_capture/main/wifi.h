/**
 * @file wifi.h
 * @brief Wi-Fi 连接模块头文件 (Day2 复用)
 *
 * 提供 wifi_init_sta() 供 Day4 集成调用。
 */

#ifndef WIFI_H
#define WIFI_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CONFIG_WIFI_SSID     "YourWiFiName"
#define CONFIG_WIFI_PASSWORD "YourWiFiPassword"
#define CONFIG_WIFI_MAX_RETRY 5

/**
 * @brief 初始化并启动 Wi-Fi STA 连接
 *
 * 调用后会立即返回，Wi-Fi 连接在后台进行。
 * 连接状态通过串口日志输出。
 * 使用 CONFIG_WIFI_SSID / CONFIG_WIFI_PASSWORD 宏定义。
 */
void wifi_init_sta(void);

/**
 * @brief 获取 Wi-Fi 连接状态
 * @return true 已连接, false 未连接
 */
bool wifi_is_connected(void);

#ifdef __cplusplus
}
#endif

#endif /* WIFI_H */