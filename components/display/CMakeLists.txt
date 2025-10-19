set(srcs "display.c")
set(includes "include")

# Default: console driver
set(DISPLAY_DRIVER_HEADER "display_console.h")
set(DISPLAY_DRIVER_EXTERN "display_console_driver")
list(APPEND srcs "drivers/console/display_console.c")
list(APPEND includes "drivers/console")

idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS ${includes}
    REQUIRES ${requires}
    PRIV_REQUIRES events
)

# Generate header with driver configuration (after component registration)
set(config_header "${CMAKE_CURRENT_BINARY_DIR}/display_driver_config.h")
configure_file(
    "${COMPONENT_DIR}/display_driver_config.h.in"
    "${config_header}"
    @ONLY
)

# Add generated header to component's public includes
target_include_directories(${COMPONENT_LIB} PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")
