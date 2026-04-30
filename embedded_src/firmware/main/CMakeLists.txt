idf_component_register(
    SRCS "wifi.c" "main.c"
    INCLUDE_DIRS "."
    REQUIRES qma7981
    PRIV_REQUIRES nvs_flash driver esp_wifi esp_event esp_netif
)
