#!/usr/bin/env python3
# JWM (Xft'siz) metni X çekirdek yazı tipiyle dönüştürmeden çizer; Uppix'teki tek yazı tipi
# "fixed" ISO 8859-1'dir. Bu yama src/font.c'de çizimden ve genişlik ölçmeden önce UTF-8'i
# ISO 8859-1'e çevirir: ü, ö, ç doğru görünür; Latin-1'de olmayan Türkçe harfler en yakın
# harfe iner (ş→s, ğ→g, ı→i, İ→I), başka karakterler '?' olur.
# Kullanım: latin1.py src/font.c
import sys

yol = sys.argv[1]
k = open(yol, encoding="latin-1").read()

islev = r'''
/* Uppix: UTF-8 -> ISO 8859-1 (ui/yama/jwm/latin1.py) */
static char *UppixLatin1(const char *s)
{
   const unsigned char *p = (const unsigned char*)s;
   char *o = Allocate(strlen(s) + 1);
   char *q = o;
   while(*p) {
      unsigned u;
      /* gecerli UTF-8 dizisi cozulur; gecersiz bayt Latin-1 sayilir (eski WM_NAME) */
      if(*p < 0x80) { u = *p++; }
      else if((*p & 0xE0) == 0xC0 && (p[1] & 0xC0) == 0x80) { u = ((p[0] & 0x1F) << 6) | (p[1] & 0x3F); p += 2; }
      else if((*p & 0xF0) == 0xE0 && (p[1] & 0xC0) == 0x80 && (p[2] & 0xC0) == 0x80) { u = ((p[0] & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F); p += 3; }
      else if((*p & 0xF8) == 0xF0 && (p[1] & 0xC0) == 0x80 && (p[2] & 0xC0) == 0x80 && (p[3] & 0xC0) == 0x80) { u = 0xFFFD; p += 4; }
      else { u = *p++; }
      switch(u) {
      case 0x15F: u = 's'; break;
      case 0x15E: u = 'S'; break;
      case 0x11F: u = 'g'; break;
      case 0x11E: u = 'G'; break;
      case 0x131: u = 'i'; break;
      case 0x130: u = 'I'; break;
      case 0x2018: case 0x2019: u = '\''; break;
      case 0x201C: case 0x201D: u = '"'; break;
      case 0x2013: case 0x2014: u = '-'; break;
      case 0x2026: u = '.'; break;
      default: break;
      }
      *q++ = (char)(u < 0x100 ? u : '?');
   }
   *q = 0;
   return o;
}

/** Convert a string to UTF-8. */'''

degisimler = [
    ("\n/** Convert a string to UTF-8. */", islev),
    ("""char *GetUTF8String(const char *str)
{
   char *utf8String = (char*)str;
""", """char *GetUTF8String(const char *str)
{
   char *utf8String = (char*)str;
   return UppixLatin1(str);
"""),
    ("""void ReleaseUTF8String(char *utf8String)
{
""", """void ReleaseUTF8String(char *utf8String)
{
   Release(utf8String);
   return;
"""),
]
for eski, yeni in degisimler:
    if k.count(eski) != 1:
        sys.exit("latin1: bulunamadı: " + eski.strip().splitlines()[0])
    k = k.replace(eski, yeni)
open(yol, "w", encoding="latin-1").write(k)
print("latin1: font.c yamalandı")
