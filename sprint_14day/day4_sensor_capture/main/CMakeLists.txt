# Day 4: 传感器数据采集 - 组件注册
# 依赖组件:
#   driver          --> I2C 总线驱动
#   nvs_flash       --> NVS 存储 (WiFi 需要)
#   esp_wifi        --> Wi-Fi 功能
#   esp_event       --> 事件循环
#   esp_timer       --> 时间戳
idf_component_register(
    SRCS
        "main.c"
        "day4_sensor_capture.c"
        "wifi.c"
    INCLUDE_DIRS
        "."
    REQUIRES
        driver
        nvs_flash
        esp_wifi
        esp_event
        esp_timer
)