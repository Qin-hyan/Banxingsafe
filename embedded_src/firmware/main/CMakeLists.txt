idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES bma400
    PRIV_REQUIRES nvs_flash
)
