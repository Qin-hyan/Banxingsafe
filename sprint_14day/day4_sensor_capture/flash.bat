@echo off
chcp 65001 >nul
echo ========================================
echo Day 4: 传感器数据采集 - 烧录
echo ========================================
echo.

REM 设置 ESP-IDF 环境变量
call "D:\esp\Espressif\frameworks\esp-idf-v5.4.3\export.bat"

REM 烧录到 ESP32-S3-EYE (COM5)
echo 正在烧录到 COM5...
idf.py -p COM5 flash

echo.
echo ========================================
echo 烧录完成!
echo ========================================
pause