# BanSafe ESP32-S3 固件项目 CMake 配置
cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS components)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(bansafe_firmware)
