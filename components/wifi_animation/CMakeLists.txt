idf_component_register(
    SRCS "wifi_animation.c"
    INCLUDE_DIRS "include" "assets"
    PRIV_REQUIRES events display esp_timer
)
