@echo off
chcp 65001 >nul
echo ========================================
echo Day 4: 传感器数据采集 - 编译
echo ========================================
echo.

REM 设置 ESP-IDF 环境变量
call "D:\esp\Espressif\frameworks\esp-idf-v5.4.3\export.bat"

REM 编译项目
idf.py build

echo.
echo ========================================
echo 编译完成!
echo ========================================
pause