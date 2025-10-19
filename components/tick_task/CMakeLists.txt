idf_component_register(
    SRCS "tick_task.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_event freertos
    PRIV_REQUIRES events
)
