@echo off
chcp 65001 >nul
echo ========================================
echo 第三天任务：I2C 传感器驱动测试
echo Building day3_i2c_sensor...
echo ========================================
echo.

REM 设置 ESP-IDF 环境变量
call "D:\esp\Espressif\frameworks\esp-idf-v5.4.3\export.bat"

REM 构建项目
idf.py build

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ========================================
    echo ✓ 编译成功！
    echo ========================================
    echo.
) else (
    echo.
    echo ========================================
    echo ✗ 编译失败！
    echo ========================================
    echo.
    pause
)