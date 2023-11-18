cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(uart_test LANGUAGES CXX C)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/mock_hw/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../src/threads/include)

target_sources(
  app
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/main.c"
          "${CMAKE_CURRENT_SOURCE_DIR}/mock_kernel/uart.c"
          "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/threads/uart.cpp"
)
