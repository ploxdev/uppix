# U++ compiler (pinned copy)

`uppc.c` is the single-file C bootstrap (`bootstrap/uppc.c`) of the
[U++](https://github.com/ploxdev/upp-lang) compiler. The kernel target (`--hedef cekirdek`) and the
freestanding runtime are built into the compiler, so nothing else is needed to build Uppix.

Source: `ploxdev/upp-lang`, `beta` branch, commit `c744c3c` (`bootstrap/uppc.c`; the
`src/runtime/upp_runtime.c` it includes is inlined into this file).
Compiler development happens in the U++ repository; when Uppix needs a newer version, this file is
replaced with U++'s `bootstrap/uppc.c`.

## Building

```bash
# Linux
gcc -std=gnu11 -O2 derleyici/uppc.c -o derleyici/uppc -pthread -lm
# Windows (MinGW-w64)
gcc -std=gnu11 -O2 derleyici/uppc.c -o derleyici/uppc.exe -luser32 -lwinmm -lgdi32
```

or `sh derleyici/kur.sh` (Linux/WSL) / `derleyici\kur.cmd` (Windows).
