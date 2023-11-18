cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(app LANGUAGES CXX)

FILE(GLOB app_sources 
    src/*.cpp
    src/threads/*.cpp
    src/hw/*.cpp
    src/hw/ble/*.cpp
)

target_sources(app PRIVATE ${app_sources})
target_include_directories(app PRIVATE include)
target_include_directories(app PRIVATE src/threads/include)
target_include_directories(app PRIVATE src/hw/include)
target_include_directories(app PRIVATE src/hw/ble/include)
