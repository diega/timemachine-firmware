set(srcs "display.c")
set(includes "include")

# MAX7219 driver (with mock for emulator, real library for hardware)
set(DISPLAY_DRIVER_HEADER "display_max7219.h")
set(DISPLAY_DRIVER_EXTERN "display_max7219_driver")
list(APPEND srcs
    "max7219/display_max7219.c"
    "max7219/fonts/default_font.c"
    "max7219/fonts/md_max72xx_font.c"
    "max7219/fonts/dotmatrix_font.c"
    "max7219/fonts/dotmatrix_small_font.c"
)
list(APPEND includes "max7219" "max7219/fonts")

if(CONFIG_TIMEMACHINE_DISPLAY_EMULATOR)
    # Add mock implementation for testing
    list(APPEND srcs "max7219/mock/max7219.c")
    list(APPEND includes "max7219/mock")
else()
    # Add real MAX7219 library dependency
    list(APPEND requires "esp-idf-lib__max7219")
endif()

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
