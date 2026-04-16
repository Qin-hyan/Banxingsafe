@echo off
cd /d %~dp0
call D:\esp\Espressif\frameworks\esp-idf-v5.4.3\export.bat
if %errorlevel% neq 0 (
    echo Failed to export ESP-IDF environment
    exit /b %errorlevel%
)
D:\esp\Espressif\frameworks\esp-idf-v5.4.3\tools\idf.py build
if %errorlevel% neq 0 (
    echo Build failed
    exit /b %errorlevel%
)
echo Build completed successfully