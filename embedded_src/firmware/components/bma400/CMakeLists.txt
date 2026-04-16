# BMA400 加速度计驱动组件

idf_component_register(
    SRCS "bma400.c"
    INCLUDE_DIRS "."
    REQUIRES driver
    PRIV_REQUIRES esp_timer
)
