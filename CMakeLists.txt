cmake_minimum_required(VERSION 3.6)
project(ledslie)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
#    refresher/src/main.cpp
#    tester/tester.c
    wifiserial/src/main.cpp
    wifiserial/src/secret.h
#    wifiserial/main.cpp
        )
include_directories("/home/marijn/esp/arduino-esp32/cores/esp32")
include_directories("/home/marijn/esp/arduino-esp32/tools/sdk/include/freertos")
include_directories("/home/marijn/esp/arduino-esp32/tools/sdk/include/newlib")
include_directories("/home/marijn/esp/arduino-esp32/tools/sdk/include/config")
include_directories("/home/marijn/esp/arduino-esp32/tools/sdk/include/esp32")
include_directories("/home/marijn/esp/arduino-esp32/tools/sdk/include/soc")
include_directories("/home/marijn/esp/arduino-esp32/tools/sdk/include/driver")
include_directories("/home/marijn/esp/arduino-esp32/tools/sdk/include/tcpip_adapter")
include_directories("/home/marijn/esp/arduino-esp32/tools/sdk/include/lwip")
include_directories("/home/marijn/esp/arduino-esp32/variants/esp32thing")
include_directories("/home/marijn/esp/arduino-esp32/libraries/WiFi/src")

add_executable(ledslie ${SOURCE_FILES})