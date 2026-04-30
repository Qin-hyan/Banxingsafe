@echo off
chcp 65001 >nul
echo ========================================
echo 配置 ESP32-S3 目标芯片
echo ========================================
echo.

REM 设置 ESP-IDF 环境变量
call "D:\esp\Espressif\frameworks\esp-idf-v5.4.3\export.bat"

REM 设置目标芯片为 ESP32-S3
idf.py set-target esp32s3

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ========================================
    echo ✓ 配置成功！
    echo ========================================
    echo.
) else (
    echo.
    echo ========================================
    echo ✗ 配置失败！
    echo ========================================
    echo.
    pause
)