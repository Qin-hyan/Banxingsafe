@echo off
chcp 65001 >nul
echo ========================================
echo 第三天任务：I2C 传感器驱动测试
echo Serial Monitor
echo ========================================
echo.

REM 设置 ESP-IDF 环境变量
call "D:\esp\Espressif\frameworks\esp-idf-v5.4.3\export.bat"

REM 启动串口监视器
idf.py -p COM5 monitor

echo.