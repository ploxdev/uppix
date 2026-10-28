@echo off
rem Boots Uppix in a QEMU window (for the X11/JWM desktop). This console window is the serial console.
rem Build the disk images first:  derleyici\uppc.exe derle .
rem Needs WSL with qemu-system-x86_64 (WSLg shows the QEMU window).
rem Note: this file is ASCII on purpose; cmd misreads UTF-8 batch files line by line.
cd /d "%~dp0\.."
if not exist derleme\uppix.img (
    echo derleme\uppix.img not found. Build the images first:  derleyici\uppc.exe derle .
    pause
    exit /b 1
)
echo.
echo  Starting Uppix in QEMU. The screen opens in a separate "QEMU (Uppix)" window
echo  - it takes a few seconds; the window is moved to the top-left corner and raised.
echo.
echo  - This window is the serial console: Uppix output also appears here.
echo  - Type commands into the QEMU window - click into it first - or here.
echo  - Desktop:  startx
echo  - Doom:     doom   (full screen on the console, or from the desktop menu)
echo  - If QEMU grabs the mouse, release it with Ctrl+Alt+G
echo  - Shut down: poweroff  - or on the desktop: Session, Shut down
echo.
start "" /b powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0place-window.ps1"
wsl -e qemu-system-x86_64 -name Uppix -m 512 -serial stdio -display gtk -drive file=derleme/uppix.img,format=raw -drive file=derleme/usb-fat32.img,format=raw -drive file=derleme/usb-exfat.img,format=raw
if errorlevel 1 pause
