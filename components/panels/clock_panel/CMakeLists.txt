idf_component_register(
    SRCS "clock_panel.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_event
    PRIV_REQUIRES events i18n
)
