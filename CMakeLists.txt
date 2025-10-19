cmake_minimum_required(VERSION 3.16)

# Enable minimal build - only include required components
set(COMPONENTS main)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(timemachine)
