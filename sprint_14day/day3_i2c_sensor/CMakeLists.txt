cmake_minimum_required(VERSION 3.16)

# 设置项目名称
set(PROJECT_NAME day3_i2c_sensor)

# 包含 ESP-IDF 构建系统
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(${PROJECT_NAME})