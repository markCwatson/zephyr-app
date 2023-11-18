# @file CMakeLists.txt
# @brief Top level CMakeLists.txt for the application.
#
# This CMake file is picked by the Zephyr build system because it is defined
# as the module CMake entry point (see zephyr/module.yml).

zephyr_include_directories(include)

add_subdirectory(drivers)
