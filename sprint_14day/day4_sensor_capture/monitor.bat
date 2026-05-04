@echo off
chcp 65001 >nul
echo ========================================
echo Day 4: 传感器数据采集 - 串口监视
echo ========================================
echo.

REM 设置 ESP-IDF 环境变量
call "D:\esp\Espressif\frameworks\esp-idf-v5.4.3\export.bat"

REM 启动串口监视器 (COM5, 115200bps)
idf.py -p COM5 monitor

echo.