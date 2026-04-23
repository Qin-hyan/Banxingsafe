@echo off
echo Building Day 2 Wi-Fi Connect Project...
call D:\esp\Espressif\frameworks\esp-idf-v5.4.3\export.bat
idf.py build
echo.
echo Build completed.
pause