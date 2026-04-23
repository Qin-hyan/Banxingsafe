@echo off
echo Starting serial monitor...
set IDF_PATH=D:\esp\Espressif\frameworks\esp-idf-v5.4.3
call "%IDF_PATH%\export.bat"
idf.py monitor
