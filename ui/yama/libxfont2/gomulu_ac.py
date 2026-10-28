#!/usr/bin/env python3
# libXfont2'nin gömülü yazı tipleri (src/builtins/fonts.c: "fixed" ve "cursor") gzip'li
# PCF olarak saklanır ve açılırken zlib ile çözülür. Uppix derlemesinde zlib bir
# yerine-koyucudur; bu betik dizileri derleme sırasında çözer ve ham PCF olarak geri yazar.
# Kullanım: gomulu_ac.py src/builtins/fonts.c
import gzip
import re
import sys

yol = sys.argv[1]
kaynak = open(yol, encoding="latin-1").read()
dizi = re.compile(r"(static const char (file_[A-Za-z0-9_]+)\[\] = \{)(.*?)(\};)", re.S)
oge = re.compile(r"'(\\[0-7]{1,3}|\\.|[^'\\])'")
kacis = {"n": 10, "t": 9, "r": 13, "0": 0, "\\": 92, "'": 39, '"': 34, "a": 7, "b": 8, "f": 12, "v": 11}


def bayt(t):
    if t.startswith("\\"):
        k = t[1:]
        if k and all(c in "01234567" for c in k):
            return int(k, 8)
        return kacis[k]
    return ord(t)


def degistir(m):
    veri = bytes(bayt(t) for t in oge.findall(m.group(3)))
    if veri[:2] != b"\x1f\x8b":
        return m.group(0)
    ham = gzip.decompress(veri)
    satirlar = []
    for i in range(0, len(ham), 12):
        satirlar.append("    " + ", ".join("(char)0x%02x" % b for b in ham[i:i + 12]) + ",")
    return m.group(1) + "\n" + "\n".join(satirlar) + "\n" + m.group(4)


yeni, n = dizi.subn(degistir, kaynak)
if n == 0:
    sys.exit("gomulu_ac: dizi bulunamadı")
open(yol, "w", encoding="latin-1").write(yeni)
print("gomulu_ac: %d dizi çözüldü" % n)
