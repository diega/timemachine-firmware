idf_component_register(
    SRCS "settings.c"
    INCLUDE_DIRS "include"
    REQUIRES nvs_flash network clock_panel ntp_sync i18n
    PRIV_REQUIRES events
)
