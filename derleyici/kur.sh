#!/bin/sh
# U++ derleyicisini derleyici/uppc olarak kurar.
set -e
cd "$(dirname "$0")"
gcc -std=gnu11 -O2 uppc.c -o uppc -pthread -lm
echo "kuruldu: derleyici/uppc"
