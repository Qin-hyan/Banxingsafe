# Day 4: 传感器数据采集 - 项目顶层 CMakeLists.txt
#
# 适配 ESP32-S3-EYE 开发板
# ESP-IDF 版本: v5.4.3+
#
# 项目结构:
#   day4_sensor_capture/
#   ├── CMakeLists.txt       (本文件)
#   ├── build.bat
#   ├── flash.bat
#   ├── monitor.bat
#   ├── config_esp32s3.bat
#   └── main/
#       ├── CMakeLists.txt
#       ├── main.c
#       ├── day4_sensor_capture.h
#       ├── day4_sensor_capture.c
#       ├── wifi.h
#       └── wifi.c

cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(day4_sensor_capture)