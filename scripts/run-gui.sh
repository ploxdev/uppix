#!/bin/sh
# Boots Uppix in a QEMU window (for the X11/JWM desktop). This terminal is the serial console.
# Build the disk images first:  derleyici/uppc derle .
cd "$(dirname "$0")/.." || exit 1
if [ ! -f derleme/uppix.img ]; then
    echo "derleme/uppix.img not found. Build the images first:  derleyici/uppc derle ." >&2
    exit 1
fi
exec qemu-system-x86_64 -name Uppix -m 512 -serial stdio -display "${UPPIX_DISPLAY:-gtk}" \
    -drive file=derleme/uppix.img,format=raw \
    -drive file=derleme/usb-fat32.img,format=raw \
    -drive file=derleme/usb-exfat.img,format=raw
