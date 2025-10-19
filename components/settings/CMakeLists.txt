idf_component_register(
    SRCS "settings.c"
    INCLUDE_DIRS "include"
    REQUIRES nvs_flash
    PRIV_REQUIRES events network clock_panel ntp_sync i18n
)
