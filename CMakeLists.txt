cmake_minimum_required(VERSION 3.16)

# Enable minimal build - only include required components
set(COMPONENTS main)

# Include panel components from subdirectory
set(EXTRA_COMPONENT_DIRS components/panels)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(timemachine)
