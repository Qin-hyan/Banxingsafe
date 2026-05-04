@echo off
chcp 65001 >nul
echo ========================================
echo Day 4: 传感器数据采集 - 设置 ESP32-S3-EYE
echo ========================================
echo.

REM 设置 ESP-IDF 环境变量
call "D:\esp\Espressif\frameworks\esp-idf-v5.4.3\export.bat"

REM 设置目标芯片为 ESP32-S3
idf.py set-target esp32s3

echo.
echo ========================================
echo 芯片目标已设置为 ESP32-S3
echo 请运行 menuconfig 配置 Flash 大小 (8MB) 和 PSRAM
echo ========================================
pause