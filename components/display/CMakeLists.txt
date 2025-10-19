set(srcs "display.c")
set(includes "include")

# MAX7219 driver (with mock for emulator, real library for hardware)
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
