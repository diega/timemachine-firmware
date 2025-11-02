idf_component_register(
    SRCS "weather_panel.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_event freertos weather panel_manager display
)
