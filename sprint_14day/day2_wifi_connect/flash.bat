@echo off
echo Flashing Day 2 Wi-Fi Connect Project to ESP32-S3-EYE...
call D:\esp\Espressif\frameworks\esp-idf-v5.4.3\export.bat
idf.py -p COM5 flash
echo.
echo Flash completed.
pause