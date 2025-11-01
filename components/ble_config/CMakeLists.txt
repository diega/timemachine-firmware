idf_component_register(
    SRCS "ble_config.c"
    INCLUDE_DIRS "include"
    REQUIRES bt nvs_flash
    PRIV_REQUIRES events network clock_panel ntp_sync i18n
)
