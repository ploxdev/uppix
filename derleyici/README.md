# U++ derleyicisi (sabitlenmiş kopya)

`uppc.c`, [U++](https://github.com/ploxdev/upp-lang) derleyicisinin tek dosyalık C
bootstrap'ıdır (`bootstrap/uppc.c`). Çekirdek hedefi (`--hedef cekirdek`) ve serbest
çalışma zamanı derleyiciye gömülüdür; Uppix'i derlemek için başka bir şey gerekmez.

Kaynak: `ploxdev/upp-lang`, beta dalı, `94efcb9` commit'i (`bootstrap/uppc.c`; içerdiği `src/runtime/upp_runtime.c` bu dosyaya gömüldü).
Derleyici geliştirmesi U++ deposunda yapılır; yeni bir sürüm gerektiğinde bu dosya
U++'ın `bootstrap/uppc.c` dosyasıyla değiştirilir.

## Derleme

```bash
# Linux
gcc -std=gnu11 -O2 derleyici/uppc.c -o derleyici/uppc -pthread -lm
# Windows (MinGW-w64)
gcc -std=gnu11 -O2 derleyici/uppc.c -o derleyici/uppc.exe -luser32 -lwinmm -lgdi32
```

ya da `sh derleyici/kur.sh` (Linux/WSL) / `derleyici\kur.cmd` (Windows).
