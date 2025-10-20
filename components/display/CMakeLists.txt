set(srcs "display.c")
set(includes "include")

# Select driver based on Kconfig
if(CONFIG_TIMEMACHINE_DISPLAY_EMULATOR OR CONFIG_TIMEMACHINE_DISPLAY_MAX7219)
    # MAX7219 driver (with mock for emulator, real library for hardware)
    set(DISPLAY_DRIVER_HEADER "display_max7219.h")
    set(DISPLAY_DRIVER_EXTERN "display_max7219_driver")
    list(APPEND srcs
        "drivers/max7219/display_max7219.c"
        "drivers/max7219/mfactory_font.c"
    )
    list(APPEND includes "drivers/max7219")

    if(CONFIG_TIMEMACHINE_DISPLAY_EMULATOR)
        # Add mock implementation for console output
        list(APPEND srcs "drivers/max7219/mock/max7219.c")
        list(APPEND includes "drivers/max7219/mock")
    else()
        # Add real MAX7219 library dependency
        list(APPEND requires "esp-idf-lib__max7219")
    endif()
else()
    # Default: console
    set(DISPLAY_DRIVER_HEADER "display_console.h")
    set(DISPLAY_DRIVER_EXTERN "display_console_driver")
    list(APPEND srcs "drivers/console/display_console.c")
    list(APPEND includes "drivers/console")
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
