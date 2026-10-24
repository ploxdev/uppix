@echo off
rem U++ derleyicisini derleyici\uppc.exe olarak kurar (MinGW-w64 gcc gerekir).
cd /d "%~dp0"
gcc -std=gnu11 -O2 uppc.c -o uppc.exe -luser32 -lwinmm -lgdi32 || exit /b 1
echo kuruldu: derleyici\uppc.exe
