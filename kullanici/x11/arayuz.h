/* arayuz.h: Uppix X programlarının (uterm, ayarlar, oturum) ortak çizim yardımcıları.
 * Yazı tipi X sunucusunun gömülü "fixed"idir (6x13, ISO 8859-1; Xft yok): metinler UTF-8
 * yazılır, çizimden önce ISO 8859-1'e çevrilir (ş→s, ğ→g, ı→i, İ→I). Renkler TrueColor
 * görselin maskelerinden hesaplanır. */
#ifndef ARAYUZ_H
#define ARAYUZ_H

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static Display* ay_d;
static int ay_ekran;
static Visual* ay_v;
static XFontStruct* ay_font;
static Atom ay_sil_atom;
static int ay_harf_gen = 6, ay_harf_yuk = 13, ay_harf_ust = 11;

/* Arayüz renkleri (Uppix teması) */
#define AY_ZEMIN 0xEEF1F5
#define AY_PANEL 0xFFFFFF
#define AY_CIZGI 0xA9B4C2
#define AY_YAZI 0x1C2430
#define AY_SOLUK 0x5A6575
#define AY_VURGU1 0x4A8FE7
#define AY_VURGU2 0x2A5D9A
#define AY_DUGME1 0xFDFDFE
#define AY_DUGME2 0xDCE2EA

static unsigned long ay_renk(unsigned long rgb) {
    unsigned long p = 0, m;
    unsigned bil[3];
    unsigned long maske[3];
    int i, k;
    bil[0] = (unsigned)((rgb >> 16) & 255);
    bil[1] = (unsigned)((rgb >> 8) & 255);
    bil[2] = (unsigned)(rgb & 255);
    maske[0] = ay_v->red_mask;
    maske[1] = ay_v->green_mask;
    maske[2] = ay_v->blue_mask;
    for (i = 0; i < 3; i++) {
        for (m = maske[i], k = 0; m && !(m & 1); m >>= 1) k++;
        p |= ((unsigned long)bil[i] * m / 255) << k;
    }
    return p;
}

/* İki renk arasında t/n oranında ara renk */
static unsigned long ay_ara(unsigned long a, unsigned long b, int t, int n) {
    unsigned long r = 0;
    int k;
    if (n <= 0) return a;
    for (k = 16; k >= 0; k -= 8) {
        long x = (long)((a >> k) & 255), y = (long)((b >> k) & 255);
        r |= (unsigned long)(x + (y - x) * t / n) << k;
    }
    return r;
}

static int ay_ac(void) {
    ay_d = XOpenDisplay(NULL);
    if (!ay_d) return 0;
    ay_ekran = DefaultScreen(ay_d);
    ay_v = DefaultVisual(ay_d, ay_ekran);
    ay_font = XLoadQueryFont(ay_d, "fixed");
    if (ay_font) {
        ay_harf_gen = XTextWidth(ay_font, "M", 1);
        ay_harf_yuk = ay_font->ascent + ay_font->descent;
        ay_harf_ust = ay_font->ascent;
    }
    ay_sil_atom = XInternAtom(ay_d, "WM_DELETE_WINDOW", False);
    return 1;
}

/* Tek Unicode karakteri ISO 8859-1'e (yazı tipinin karakter kümesine) indirir. */
static unsigned char ay_latin1_harf(unsigned u) {
    switch (u) {
    case 0x15F: return 's';
    case 0x15E: return 'S';
    case 0x11F: return 'g';
    case 0x11E: return 'G';
    case 0x131: return 'i';
    case 0x130: return 'I';
    case 0x2018: case 0x2019: return '\'';
    case 0x201C: case 0x201D: return '"';
    case 0x2013: case 0x2014: case 0x2500: return '-';
    case 0x2502: return '|';
    case 0x2026: return '.';
    default: break;
    }
    return u < 0x100 ? (unsigned char)u : '?';
}

/* UTF-8 dizgiden bir karakter çözer; *p ilerler. Geçersiz baytta bayt değeri döner. */
static unsigned ay_utf8_coz(const unsigned char** p) {
    const unsigned char* s = *p;
    unsigned u;
    if (s[0] < 0x80) {
        *p = s + 1;
        return s[0];
    }
    if ((s[0] & 0xE0) == 0xC0 && (s[1] & 0xC0) == 0x80) {
        *p = s + 2;
        return ((s[0] & 0x1Fu) << 6) | (s[1] & 0x3Fu);
    }
    if ((s[0] & 0xF0) == 0xE0 && (s[1] & 0xC0) == 0x80 && (s[2] & 0xC0) == 0x80) {
        *p = s + 3;
        return ((s[0] & 0x0Fu) << 12) | ((s[1] & 0x3Fu) << 6) | (s[2] & 0x3Fu);
    }
    if ((s[0] & 0xF8) == 0xF0 && s[1] && s[2] && s[3]) {
        *p = s + 4;
        return 0xFFFD;
    }
    u = s[0];
    *p = s + 1;
    return u;
}

static int ay_latin1(const char* s, char* o, int en_cok) {
    const unsigned char* p = (const unsigned char*)s;
    int n = 0;
    while (*p && n < en_cok - 1) o[n++] = (char)ay_latin1_harf(ay_utf8_coz(&p));
    o[n] = 0;
    return n;
}

static int ay_metin_gen(const char* s) {
    char b[512];
    int n = ay_latin1(s, b, sizeof(b));
    return ay_font ? XTextWidth(ay_font, b, n) : n * ay_harf_gen;
}

/* (x, y) metnin sol üst köşesidir. */
static void ay_metin(Drawable w, GC gc, int x, int y, const char* s, unsigned long rgb) {
    char b[512];
    int n = ay_latin1(s, b, sizeof(b));
    XSetForeground(ay_d, gc, ay_renk(rgb));
    XDrawString(ay_d, w, gc, x, y + ay_harf_ust, b, n);
}

/* Dikey degrade dolgu (üstte a, altta b) */
static void ay_degrade(Drawable w, GC gc, int x, int y, int g, int h, unsigned long a, unsigned long b) {
    int i;
    for (i = 0; i < h; i++) {
        XSetForeground(ay_d, gc, ay_renk(ay_ara(a, b, i, h > 1 ? h - 1 : 1)));
        XDrawLine(ay_d, w, gc, x, y + i, x + g - 1, y + i);
    }
}

static void ay_cerceve(Drawable w, GC gc, int x, int y, int g, int h, unsigned long rgb) {
    XSetForeground(ay_d, gc, ay_renk(rgb));
    XDrawRectangle(ay_d, w, gc, x, y, (unsigned)(g - 1), (unsigned)(h - 1));
}

typedef struct {
    int x, y, g, h;
    const char* ad;
    int secili;   /* seçili seçenek düğmesi (vurgulu çizilir) */
    int basili;
} AyDugme;

static void ay_dugme_ciz(Drawable w, GC gc, AyDugme* b) {
    unsigned long ust = AY_DUGME1, alt = AY_DUGME2, yazi = AY_YAZI, kenar = AY_CIZGI;
    int tg;
    if (b->secili) {
        ust = AY_VURGU1;
        alt = AY_VURGU2;
        yazi = 0xFFFFFF;
        kenar = AY_VURGU2;
    }
    if (b->basili) {
        unsigned long t = ust;
        ust = alt;
        alt = t;
    }
    ay_degrade(w, gc, b->x + 1, b->y + 1, b->g - 2, b->h - 2, ust, alt);
    ay_cerceve(w, gc, b->x, b->y, b->g, b->h, kenar);
    tg = ay_metin_gen(b->ad);
    ay_metin(w, gc, b->x + (b->g - tg) / 2, b->y + (b->h - ay_harf_yuk) / 2, b->ad, yazi);
}

static int ay_icinde(const AyDugme* b, int x, int y) {
    return x >= b->x && y >= b->y && x < b->x + b->g && y < b->y + b->h;
}

/* Ekranın ortasında, pencere yöneticisinin süsleyeceği bir pencere. sinif: WM_CLASS. */
static Window ay_pencere(const char* baslik, const char* sinif, int g, int h, unsigned long arka) {
    XSetWindowAttributes oz;
    XClassHint ch;
    XSizeHints bh;
    char b[256];
    Window w;
    int eg = DisplayWidth(ay_d, ay_ekran), eh = DisplayHeight(ay_d, ay_ekran);
    oz.background_pixel = ay_renk(arka);
    oz.event_mask = ExposureMask | KeyPressMask | ButtonPressMask | ButtonReleaseMask | StructureNotifyMask | FocusChangeMask;
    w = XCreateWindow(ay_d, RootWindow(ay_d, ay_ekran), (eg - g) / 2, (eh - h) / 2 - 20, (unsigned)g, (unsigned)h, 0,
                      CopyFromParent, InputOutput, CopyFromParent, CWBackPixel | CWEventMask, &oz);
    ay_latin1(baslik, b, sizeof(b));
    XStoreName(ay_d, w, b);
    XChangeProperty(ay_d, w, XInternAtom(ay_d, "_NET_WM_NAME", False), XInternAtom(ay_d, "UTF8_STRING", False), 8,
                    PropModeReplace, (const unsigned char*)baslik, (int)strlen(baslik));
    ch.res_name = (char*)sinif;
    ch.res_class = (char*)sinif;
    XSetClassHint(ay_d, w, &ch);
    bh.flags = PPosition | PSize;
    bh.x = (eg - g) / 2;
    bh.y = (eh - h) / 2 - 20;
    bh.width = g;
    bh.height = h;
    XSetWMNormalHints(ay_d, w, &bh);
    XSetWMProtocols(ay_d, w, &ay_sil_atom, 1);
    return w;
}

/* Pencere yöneticisinin "kapat" isteği mi? */
static int ay_kapat_istegi(XEvent* e) {
    return e->type == ClientMessage && (Atom)e->xclient.data.l[0] == ay_sil_atom;
}

/* Programı arka planda çalıştırır (ebeveyn beklemez; çocuk çift fork'la sahipsiz kalır). */
static void ay_calistir(const char* yol, char* const arg[]) {
    pid_t c = fork();
    if (c == 0) {
        if (fork() == 0) {
            if (ay_d) close(ConnectionNumber(ay_d));
            execv(yol, arg);
            _exit(127);
        }
        _exit(0);
    }
    if (c > 0) {
        int st;
        waitpid(c, &st, 0);
    }
}

#endif
