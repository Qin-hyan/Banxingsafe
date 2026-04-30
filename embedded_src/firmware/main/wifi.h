/**
 * @file wifi.h
 * @brief Wi-Fi 连接模块头文件
 */

#ifndef WIFI_H
#define WIFI_H

/**
 * @brief 初始化 Wi-Fi 并连接到指定网络
 * 
 * @param ssid SSID
 * @param password 密码
 * @return esp_err_t ESP_OK 成功，其他为错误码
 */
esp_err_t wifi_init_and_connect(const char *ssid, const char *password);

/**
 * @brief 等待 Wi-Fi 连接完成并打印 IP 地址
 * 
 * @return esp_err_t ESP_OK 成功，其他为错误码
 */
esp_err_t wifi_wait_for_ip(void);

/**
 * @brief 获取 Wi-Fi 是否已连接
 * 
 * @return true 已连接
 * @return false 未连接
 */
bool wifi_is_connected(void);

#endif // WIFI_H