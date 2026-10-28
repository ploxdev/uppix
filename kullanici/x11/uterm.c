/* uterm: Uppix'in X uçbirimi (libX11, musl ile statik; ui/derle.sh derler).
 * /dev/ptmx'ten bir PTY açar, çocukta yavru ucu stdin/stdout/stderr yapıp etkileşimli
 * /bin/sh çalıştırır: Uppix çekirdeği bunu o PTY'ye bağlı bir uppsh oturumu olarak başlatır
 * (yerleşik komutların hepsi, geçmiş ve Tab tamamlama çalışır). Gelen baytlar VT100/ANSI
 * olarak bir hücre ızgarasına işlenir: imleç hareketi, silme, satır ekleme/silme, kaydırma
 * bölgesi, 16 renk (ve 256 renkten yaklaştırma), kalın/ters/altı çizili, alternatif ekran,
 * imleç konumu sorgusu. UTF-8 çözülür; "fixed" yazı tipi ISO 8859-1 olduğundan Latin-1
 * dışındaki harfler en yakın harfe, kutu çizgileri yazı tipinin DEC çizgi glif'lerine iner.
 * Klavye: oklar, Home/End, PgUp/PgDn, Delete, F1-F12; Türkçe düzende ş, ğ, ı, İ UTF-8 gider.
 *   uterm              uçbirim penceresi
 *   uterm --dokum      odak gelince "uterm: hazır", kabuk bitince ekranın metnini stdout'a
 *                      yazar (testler için) */
#define _GNU_SOURCE
#include "arayuz.h"
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>

#define SAT_EN 100
#define SUT_EN 240
#define KENAR 3
#define ON_VARS 16
#define ARKA_VARS 17

#define OZ_KALIN 1
#define OZ_TERS 2
#define OZ_ALT 4

typedef struct {
    unsigned char c, on, arka, oz;
} Hucre;

static Hucre ekran[SAT_EN][SUT_EN], yedek[SAT_EN][SUT_EN];
static int satir_n = 24, sutun_n = 80;
static int ix, iy, sakli_x, sakli_y, ust_s, alt_s, sarma, imlec_gorunur = 1, alt_ekran;
static unsigned char on_r = ON_VARS, arka_r = ARKA_VARS, oz_r;
static int kirli[SAT_EN];
static int odak = 1;
static int ana = -1;
static Window pencere;
static GC gc;
static unsigned long palet[18];
static int glif_var;

static const unsigned long PALET[18] = {
    0x1B1F27, 0xE05561, 0x8CC265, 0xD18F52, 0x4AA5F0, 0xC162DE, 0x42B3C2, 0xD7DAE0,
    0x5C6370, 0xFF616E, 0xA5E075, 0xF0A45D, 0x6CB6FF, 0xDE73FF, 0x4CD1E0, 0xFFFFFF,
    0xD8DEE9, 0x141A22};

/* ---------------------------------------------------------------- ızgara */

static void satir_temizle(int y, int x0, int x1) {
    int x;
    if (y < 0 || y >= satir_n) return;
    for (x = x0; x < x1 && x < sutun_n; x++) {
        ekran[y][x].c = ' ';
        ekran[y][x].on = ON_VARS;
        ekran[y][x].arka = arka_r;
        ekran[y][x].oz = 0;
    }
    kirli[y] = 1;
}

static void kaydir_yukari(int ust, int alt, int n) {
    int y;
    if (n > alt - ust + 1) n = alt - ust + 1;
    for (y = ust; y <= alt - n; y++) {
        memcpy(ekran[y], ekran[y + n], sizeof(ekran[y]));
        kirli[y] = 1;
    }
    for (y = alt - n + 1; y <= alt; y++) satir_temizle(y, 0, sutun_n);
}

static void kaydir_asagi(int ust, int alt, int n) {
    int y;
    if (n > alt - ust + 1) n = alt - ust + 1;
    for (y = alt; y >= ust + n; y--) {
        memcpy(ekran[y], ekran[y - n], sizeof(ekran[y]));
        kirli[y] = 1;
    }
    for (y = ust; y < ust + n; y++) satir_temizle(y, 0, sutun_n);
}

static void yeni_satir(void) {
    if (iy == alt_s) kaydir_yukari(ust_s, alt_s, 1);
    else if (iy < satir_n - 1) iy++;
}

static void imlec_kur(int x, int y) {
    if (x < 0) x = 0;
    if (x >= sutun_n) x = sutun_n - 1;
    if (y < 0) y = 0;
    if (y >= satir_n) y = satir_n - 1;
    kirli[iy] = 1;
    ix = x;
    iy = y;
    kirli[iy] = 1;
    sarma = 0;
}

static void harf_koy(unsigned char c) {
    if (sarma) {
        ix = 0;
        yeni_satir();
        sarma = 0;
    }
    ekran[iy][ix].c = c;
    ekran[iy][ix].on = on_r;
    ekran[iy][ix].arka = arka_r;
    ekran[iy][ix].oz = oz_r;
    kirli[iy] = 1;
    if (ix == sutun_n - 1) sarma = 1;
    else ix++;
}

static void sifirla(void) {
    int y;
    on_r = ON_VARS;
    arka_r = ARKA_VARS;
    oz_r = 0;
    ust_s = 0;
    alt_s = satir_n - 1;
    imlec_gorunur = 1;
    for (y = 0; y < satir_n; y++) satir_temizle(y, 0, sutun_n);
    imlec_kur(0, 0);
}

/* Kutu çizgileri ve bloklar: yazı tipinin 1..31 arasındaki DEC çizgi glif'leri */
static unsigned char harf_eslem(unsigned u) {
    if (glif_var) {
        switch (u) {
        case 0x2500: case 0x2550: case 0x2501: return 0x12;
        case 0x2502: case 0x2551: case 0x2503: return 0x19;
        case 0x250C: case 0x2554: case 0x256D: return 0x0D;
        case 0x2510: case 0x2557: case 0x256E: return 0x0C;
        case 0x2514: case 0x255A: case 0x2570: return 0x0E;
        case 0x2518: case 0x255D: case 0x256F: return 0x0B;
        case 0x251C: case 0x2560: return 0x15;
        case 0x2524: case 0x2563: return 0x16;
        case 0x252C: case 0x2566: return 0x18;
        case 0x2534: case 0x2569: return 0x17;
        case 0x253C: case 0x256C: return 0x0F;
        case 0x2591: case 0x2592: case 0x2593: case 0x2588: case 0x2580: case 0x2584: return 0x02;
        case 0x25C6: return 0x01;
        default: break;
        }
    }
    switch (u) {
    case 0x25CF: case 0x2022: return 'o';
    case 0x2190: return '<';
    case 0x2192: return '>';
    case 0x2191: return '^';
    case 0x2193: return 'v';
    default: break;
    }
    if (u >= 0x2500 && u < 0x2600) return '+';
    return ay_latin1_harf(u);
}

/* ---------------------------------------------------------------- VT100/ANSI çözücü */

static int durum;   /* 0 düz, 1 ESC, 2 CSI, 3 OSC, 4 karakter kümesi seçimi */
static int param[16], np, ozel;
static unsigned utf_u;
static int utf_kalan;

static void yanitla(const char* s) {
    if (ana >= 0) {
        ssize_t r = write(ana, s, strlen(s));
        (void)r;
    }
}

static int p_al(int i, int vars) { return (i < np && param[i] > 0) ? param[i] : vars; }

static unsigned char renk256(int n) {
    static const unsigned char kup[6] = {0, 1, 1, 2, 2, 3};
    int r, g, b;
    if (n < 16) return (unsigned char)n;
    if (n >= 232) return n < 244 ? 8 : 7;
    n -= 16;
    r = kup[n / 36];
    g = kup[(n / 6) % 6];
    b = kup[n % 6];
    return (unsigned char)(((r > 1) ? 1 : 0) | ((g > 1) ? 2 : 0) | ((b > 1) ? 4 : 0) | ((r + g + b > 6) ? 8 : 0));
}

static void sgr(void) {
    int i;
    if (np == 0) {
        on_r = ON_VARS;
        arka_r = ARKA_VARS;
        oz_r = 0;
        return;
    }
    for (i = 0; i < np; i++) {
        int p = param[i];
        if (p == 0) {
            on_r = ON_VARS;
            arka_r = ARKA_VARS;
            oz_r = 0;
        } else if (p == 1) oz_r |= OZ_KALIN;
        else if (p == 4) oz_r |= OZ_ALT;
        else if (p == 7) oz_r |= OZ_TERS;
        else if (p == 22) oz_r &= (unsigned char)~OZ_KALIN;
        else if (p == 24) oz_r &= (unsigned char)~OZ_ALT;
        else if (p == 27) oz_r &= (unsigned char)~OZ_TERS;
        else if (p >= 30 && p <= 37) on_r = (unsigned char)(p - 30);
        else if (p == 39) on_r = ON_VARS;
        else if (p >= 40 && p <= 47) arka_r = (unsigned char)(p - 40);
        else if (p == 49) arka_r = ARKA_VARS;
        else if (p >= 90 && p <= 97) on_r = (unsigned char)(p - 90 + 8);
        else if (p >= 100 && p <= 107) arka_r = (unsigned char)(p - 100 + 8);
        else if ((p == 38 || p == 48) && i + 2 < np && param[i + 1] == 5) {
            if (p == 38) on_r = renk256(param[i + 2]);
            else arka_r = renk256(param[i + 2]);
            i += 2;
        } else if ((p == 38 || p == 48) && i + 4 < np && param[i + 1] == 2) {
            i += 4;
        }
    }
}

static void alternatif(int ac) {
    int y;
    if (ac && !alt_ekran) {
        memcpy(yedek, ekran, sizeof(ekran));
        sakli_x = ix;
        sakli_y = iy;
        for (y = 0; y < satir_n; y++) satir_temizle(y, 0, sutun_n);
        alt_ekran = 1;
    } else if (!ac && alt_ekran) {
        memcpy(ekran, yedek, sizeof(ekran));
        for (y = 0; y < satir_n; y++) kirli[y] = 1;
        imlec_kur(sakli_x, sakli_y);
        alt_ekran = 0;
    }
}

static void csi(unsigned char son) {
    int n = p_al(0, 1), y, x;
    char b[32];
    switch (son) {
    case 'A': imlec_kur(ix, iy - n); break;
    case 'B': imlec_kur(ix, iy + n); break;
    case 'C': imlec_kur(ix + n, iy); break;
    case 'D': imlec_kur(ix - n, iy); break;
    case 'E': imlec_kur(0, iy + n); break;
    case 'F': imlec_kur(0, iy - n); break;
    case 'G': case '`': imlec_kur(n - 1, iy); break;
    case 'd': imlec_kur(ix, n - 1); break;
    case 'H': case 'f': imlec_kur(p_al(1, 1) - 1, p_al(0, 1) - 1); break;
    case 'J':
        n = np ? param[0] : 0;
        if (n == 0) {
            satir_temizle(iy, ix, sutun_n);
            for (y = iy + 1; y < satir_n; y++) satir_temizle(y, 0, sutun_n);
        } else if (n == 1) {
            for (y = 0; y < iy; y++) satir_temizle(y, 0, sutun_n);
            satir_temizle(iy, 0, ix + 1);
        } else {
            for (y = 0; y < satir_n; y++) satir_temizle(y, 0, sutun_n);
        }
        break;
    case 'K':
        n = np ? param[0] : 0;
        if (n == 0) satir_temizle(iy, ix, sutun_n);
        else if (n == 1) satir_temizle(iy, 0, ix + 1);
        else satir_temizle(iy, 0, sutun_n);
        break;
    case 'L': if (iy >= ust_s && iy <= alt_s) kaydir_asagi(iy, alt_s, n); break;
    case 'M': if (iy >= ust_s && iy <= alt_s) kaydir_yukari(iy, alt_s, n); break;
    case 'S': kaydir_yukari(ust_s, alt_s, n); break;
    case 'T': kaydir_asagi(ust_s, alt_s, n); break;
    case 'P':
        for (x = ix; x < sutun_n; x++) {
            if (x + n < sutun_n) ekran[iy][x] = ekran[iy][x + n];
            else satir_temizle(iy, x, x + 1);
        }
        kirli[iy] = 1;
        break;
    case '@':
        for (x = sutun_n - 1; x >= ix + n; x--) ekran[iy][x] = ekran[iy][x - n];
        satir_temizle(iy, ix, ix + n);
        break;
    case 'X': satir_temizle(iy, ix, ix + n); break;
    case 'm': sgr(); break;
    case 'r':
        ust_s = p_al(0, 1) - 1;
        alt_s = p_al(1, satir_n) - 1;
        if (ust_s < 0 || alt_s >= satir_n || ust_s >= alt_s) {
            ust_s = 0;
            alt_s = satir_n - 1;
        }
        imlec_kur(0, 0);
        break;
    case 's': sakli_x = ix; sakli_y = iy; break;
    case 'u': imlec_kur(sakli_x, sakli_y); break;
    case 'n':
        if (np && param[0] == 6) {
            snprintf(b, sizeof(b), "\033[%d;%dR", iy + 1, ix + 1);
            yanitla(b);
        } else if (np && param[0] == 5) {
            yanitla("\033[0n");
        }
        break;
    case 'c': if (!ozel) yanitla("\033[?1;2c"); break;
    case 'h': case 'l':
        if (ozel) {
            int i;
            for (i = 0; i < np; i++) {
                if (param[i] == 25) {
                    imlec_gorunur = son == 'h';
                    kirli[iy] = 1;
                } else if (param[i] == 1049 || param[i] == 1047 || param[i] == 47) {
                    alternatif(son == 'h');
                }
            }
        }
        break;
    default: break;
    }
}

static void bayt_isle(unsigned char b) {
    if (durum == 0) {
        if (utf_kalan > 0) {
            if ((b & 0xC0) == 0x80) {
                utf_u = (utf_u << 6) | (b & 0x3Fu);
                if (--utf_kalan == 0) harf_koy(harf_eslem(utf_u));
                return;
            }
            utf_kalan = 0;
            harf_koy('?');
        }
        if (b >= 0xC0 && b < 0xF8) {
            if (b < 0xE0) { utf_u = b & 0x1Fu; utf_kalan = 1; }
            else if (b < 0xF0) { utf_u = b & 0x0Fu; utf_kalan = 2; }
            else { utf_u = b & 0x07u; utf_kalan = 3; }
            return;
        }
        if (b >= 0x80) { harf_koy('?'); return; }
        switch (b) {
        case 7: return;
        case 8: if (ix > 0) imlec_kur(ix - 1, iy); else sarma = 0; return;
        case 9: imlec_kur((ix / 8 + 1) * 8 < sutun_n ? (ix / 8 + 1) * 8 : sutun_n - 1, iy); return;
        case 10: case 11: case 12: kirli[iy] = 1; yeni_satir(); kirli[iy] = 1; sarma = 0; return;
        case 13: imlec_kur(0, iy); return;
        case 27: durum = 1; return;
        default: break;
        }
        if (b >= 32 && b < 127) harf_koy(b);
        return;
    }
    if (durum == 1) {
        durum = 0;
        switch (b) {
        case '[': durum = 2; np = 0; ozel = 0; memset(param, 0, sizeof(param)); return;
        case ']': durum = 3; return;
        case '(': case ')': case '*': case '+': durum = 4; return;
        case '7': sakli_x = ix; sakli_y = iy; return;
        case '8': imlec_kur(sakli_x, sakli_y); return;
        case 'c': sifirla(); return;
        case 'D': kirli[iy] = 1; yeni_satir(); kirli[iy] = 1; return;
        case 'E': imlec_kur(0, iy); yeni_satir(); kirli[iy] = 1; return;
        case 'M':
            if (iy == ust_s) kaydir_asagi(ust_s, alt_s, 1);
            else imlec_kur(ix, iy - 1);
            return;
        default: return;
        }
    }
    if (durum == 2) {
        if (b >= '0' && b <= '9') {
            if (np == 0) np = 1;
            if (np <= 16) param[np - 1] = param[np - 1] * 10 + (b - '0');
            return;
        }
        if (b == ';') {
            if (np == 0) np = 1;
            if (np < 16) np++;
            return;
        }
        if (b == '?' || b == '>' || b == '=') { ozel = 1; return; }
        if (b >= 0x40 && b <= 0x7E) {
            durum = 0;
            csi(b);
        }
        return;
    }
    if (durum == 3) {
        if (b == 7) durum = 0;
        else if (b == 27) durum = 1;
        return;
    }
    durum = 0; /* karakter kümesi seçimi yok sayılır */
}

/* ---------------------------------------------------------------- çizim */

static void satir_ciz(int y) {
    int x = 0;
    while (x < sutun_n) {
        int bas = x, ters, on, arka, oz;
        char b[SUT_EN];
        Hucre* h = &ekran[y][x];
        int imlecte = imlec_gorunur && odak && y == iy && x == ix;
        ters = ((h->oz & OZ_TERS) != 0) ^ imlecte;
        oz = h->oz;
        on = ters ? h->arka : h->on;
        arka = ters ? h->on : h->arka;
        if ((oz & OZ_KALIN) && on < 8) on += 8;
        while (x < sutun_n) {
            Hucre* k = &ekran[y][x];
            int ik = imlec_gorunur && odak && y == iy && x == ix;
            if (k->on != h->on || k->arka != h->arka || k->oz != h->oz || ik != imlecte) break;
            b[x - bas] = (char)k->c;
            x++;
            if (imlecte) break;
        }
        XSetForeground(ay_d, gc, palet[arka]);
        XFillRectangle(ay_d, pencere, gc, KENAR + bas * ay_harf_gen, KENAR + y * ay_harf_yuk,
                       (unsigned)((x - bas) * ay_harf_gen), (unsigned)ay_harf_yuk);
        XSetForeground(ay_d, gc, palet[on]);
        XDrawString(ay_d, pencere, gc, KENAR + bas * ay_harf_gen, KENAR + y * ay_harf_yuk + ay_harf_ust, b, x - bas);
        if (oz & OZ_KALIN)
            XDrawString(ay_d, pencere, gc, KENAR + bas * ay_harf_gen + 1, KENAR + y * ay_harf_yuk + ay_harf_ust, b, x - bas);
        if (oz & OZ_ALT)
            XDrawLine(ay_d, pencere, gc, KENAR + bas * ay_harf_gen, KENAR + y * ay_harf_yuk + ay_harf_yuk - 1,
                      KENAR + x * ay_harf_gen - 1, KENAR + y * ay_harf_yuk + ay_harf_yuk - 1);
    }
    /* odak yokken imleç içi boş kutu */
    if (imlec_gorunur && !odak && y == iy) {
        XSetForeground(ay_d, gc, palet[ON_VARS]);
        XDrawRectangle(ay_d, pencere, gc, KENAR + ix * ay_harf_gen, KENAR + y * ay_harf_yuk,
                       (unsigned)(ay_harf_gen - 1), (unsigned)(ay_harf_yuk - 1));
    }
    kirli[y] = 0;
}

static void kirlileri_ciz(void) {
    int y;
    for (y = 0; y < satir_n; y++)
        if (kirli[y]) satir_ciz(y);
}

static void hepsini_ciz(void) {
    int y;
    for (y = 0; y < satir_n; y++) kirli[y] = 1;
    kirlileri_ciz();
}

static void boyut_bildir(void) {
    struct winsize ws;
    memset(&ws, 0, sizeof(ws));
    ws.ws_row = (unsigned short)satir_n;
    ws.ws_col = (unsigned short)sutun_n;
    ws.ws_xpixel = (unsigned short)(sutun_n * ay_harf_gen);
    ws.ws_ypixel = (unsigned short)(satir_n * ay_harf_yuk);
    ioctl(ana, TIOCSWINSZ, &ws);
}

static void boyut_degisti(int g, int h) {
    int ys = (h - 2 * KENAR) / ay_harf_yuk, xs = (g - 2 * KENAR) / ay_harf_gen, y;
    if (ys < 2) ys = 2;
    if (xs < 10) xs = 10;
    if (ys > SAT_EN) ys = SAT_EN;
    if (xs > SUT_EN) xs = SUT_EN;
    if (ys == satir_n && xs == sutun_n) return;
    if (ys < satir_n && iy >= ys) {
        int k = iy - ys + 1;
        kaydir_yukari(0, satir_n - 1, k);
        iy -= k;
    }
    for (y = 0; y < SAT_EN; y++) {
        int x;
        for (x = (y < satir_n ? sutun_n : 0); x < SUT_EN; x++) {
            ekran[y][x].c = ' ';
            ekran[y][x].on = ON_VARS;
            ekran[y][x].arka = ARKA_VARS;
            ekran[y][x].oz = 0;
        }
    }
    satir_n = ys;
    sutun_n = xs;
    ust_s = 0;
    alt_s = satir_n - 1;
    imlec_kur(ix, iy);
    boyut_bildir();
    XClearWindow(ay_d, pencere);
    hepsini_ciz();
}

/* ---------------------------------------------------------------- klavye */

static void utf8_gonder(unsigned u) {
    char b[4];
    int n;
    if (u < 0x80) { b[0] = (char)u; n = 1; }
    else if (u < 0x800) { b[0] = (char)(0xC0 | (u >> 6)); b[1] = (char)(0x80 | (u & 0x3F)); n = 2; }
    else { b[0] = (char)(0xE0 | (u >> 12)); b[1] = (char)(0x80 | ((u >> 6) & 0x3F)); b[2] = (char)(0x80 | (u & 0x3F)); n = 3; }
    if (write(ana, b, (size_t)n) < 0) return;
}

static void tus(XKeyEvent* e) {
    char b[32];
    KeySym k = NoSymbol;
    int n = XLookupString(e, b, sizeof(b), &k, NULL), i;
    const char* s = NULL;
    switch (k) {
    case XK_Up: s = "\033[A"; break;
    case XK_Down: s = "\033[B"; break;
    case XK_Right: s = "\033[C"; break;
    case XK_Left: s = "\033[D"; break;
    case XK_Home: case XK_KP_Home: s = "\033[H"; break;
    case XK_End: case XK_KP_End: s = "\033[F"; break;
    case XK_Insert: s = "\033[2~"; break;
    case XK_Delete: case XK_KP_Delete: s = "\033[3~"; break;
    case XK_Prior: s = "\033[5~"; break;
    case XK_Next: s = "\033[6~"; break;
    case XK_F1: s = "\033OP"; break;
    case XK_F2: s = "\033OQ"; break;
    case XK_F3: s = "\033OR"; break;
    case XK_F4: s = "\033OS"; break;
    case XK_F5: s = "\033[15~"; break;
    case XK_F6: s = "\033[17~"; break;
    case XK_F7: s = "\033[18~"; break;
    case XK_F8: s = "\033[19~"; break;
    case XK_F9: s = "\033[20~"; break;
    case XK_F10: s = "\033[21~"; break;
    case XK_F11: s = "\033[23~"; break;
    case XK_F12: s = "\033[24~"; break;
    case XK_Return: case XK_KP_Enter: s = "\r"; break;
    case XK_BackSpace: s = "\177"; break;
    case XK_ISO_Left_Tab: s = "\033[Z"; break;
    /* Türkçe düzen: Latin-1'de olmayan harfler */
    case XK_scedilla: utf8_gonder(0x15F); return;
    case XK_Scedilla: utf8_gonder(0x15E); return;
    case XK_gbreve: utf8_gonder(0x11F); return;
    case XK_Gbreve: utf8_gonder(0x11E); return;
    case XK_idotless: utf8_gonder(0x131); return;
    case XK_Iabovedot: utf8_gonder(0x130); return;
    default: break;
    }
    if (s) {
        if (write(ana, s, strlen(s)) < 0) return;
        return;
    }
    if (k >= 0x1000100 && k <= 0x110FFFF) {
        utf8_gonder((unsigned)(k - 0x1000000));
        return;
    }
    for (i = 0; i < n; i++) {
        unsigned char c = (unsigned char)b[i];
        if (c >= 0x80) utf8_gonder(c);
        else if (write(ana, &b[i], 1) < 0) return;
    }
}

/* ---------------------------------------------------------------- ana döngü */

static void dokum(void) {
    int y, x;
    printf("uterm: ekran %dx%d\n", sutun_n, satir_n);
    for (y = 0; y < satir_n; y++) {
        int son = sutun_n;
        while (son > 0 && ekran[y][son - 1].c == ' ') son--;
        if (son == 0) continue;
        printf("uterm| ");
        for (x = 0; x < son; x++) {
            unsigned char c = ekran[y][x].c;
            if (c >= 0x80) printf("%c%c", 0xC0 | (c >> 6), 0x80 | (c & 0x3F));
            else putchar(c < 32 ? '+' : c);
        }
        putchar('\n');
    }
    printf("uterm: bitti\n");
}

int main(int argc, char** argv) {
    int dokum_istendi = argc > 1 && strcmp(argv[1], "--dokum") == 0, hazir_yazildi = 0;
    char* yavru;
    pid_t c;
    XEvent e;
    int i;
    setvbuf(stdout, NULL, _IONBF, 0);
    ana = posix_openpt(O_RDWR | O_NOCTTY);
    if (ana < 0 || grantpt(ana) < 0 || unlockpt(ana) < 0 || !(yavru = ptsname(ana))) {
        fprintf(stderr, "uterm: PTY açılamadı\n");
        return 1;
    }
    {
        struct winsize ws;
        memset(&ws, 0, sizeof(ws));
        ws.ws_row = (unsigned short)satir_n;
        ws.ws_col = (unsigned short)sutun_n;
        ioctl(ana, TIOCSWINSZ, &ws);
    }
    c = fork();
    if (c == 0) {
        int s;
        char* arg[] = {"sh", NULL};
        setsid();
        s = open(yavru, O_RDWR);
        if (s < 0) _exit(126);
        ioctl(s, TIOCSCTTY, 0);
        dup2(s, 0);
        dup2(s, 1);
        dup2(s, 2);
        if (s > 2) close(s);
        close(ana);
        setenv("TERM", "vt100", 1);
        execv("/bin/sh", arg);
        _exit(127);
    }
    if (c < 0) {
        fprintf(stderr, "uterm: fork başarısız\n");
        return 1;
    }
    if (!ay_ac()) {
        fprintf(stderr, "uterm: ekran açılamadı (DISPLAY?)\n");
        return 1;
    }
    glif_var = ay_font && ay_font->min_char_or_byte2 <= 1;
    for (i = 0; i < 18; i++) palet[i] = ay_renk(PALET[i]);
    for (i = 0; i < SAT_EN; i++) {
        int x;
        for (x = 0; x < SUT_EN; x++) {
            ekran[i][x].c = ' ';
            ekran[i][x].on = ON_VARS;
            ekran[i][x].arka = ARKA_VARS;
        }
    }
    sifirla();
    pencere = ay_pencere("Uçbirim", "uterm", sutun_n * ay_harf_gen + 2 * KENAR, satir_n * ay_harf_yuk + 2 * KENAR,
                         PALET[ARKA_VARS]);
    gc = XCreateGC(ay_d, pencere, 0, NULL);
    if (ay_font) XSetFont(ay_d, gc, ay_font->fid);
    XMapRaised(ay_d, pencere);
    for (;;) {
        struct pollfd p[2];
        while (XPending(ay_d)) {
            XNextEvent(ay_d, &e);
            if (e.type == Expose && e.xexpose.count == 0) hepsini_ciz();
            else if (e.type == FocusIn && dokum_istendi && !hazir_yazildi) {
                hazir_yazildi = 1;
                odak = 1;
                kirli[iy] = 1;
                printf("uterm: hazır\n");
            } else if (e.type == ConfigureNotify) boyut_degisti(e.xconfigure.width, e.xconfigure.height);
            else if (e.type == KeyPress) tus(&e.xkey);
            else if (e.type == FocusIn || e.type == FocusOut) {
                odak = e.type == FocusIn;
                kirli[iy] = 1;
            } else if (ay_kapat_istegi(&e)) {
                close(ana);
                return 0;
            }
        }
        kirlileri_ciz();
        XFlush(ay_d);
        p[0].fd = ConnectionNumber(ay_d);
        p[0].events = POLLIN;
        p[1].fd = ana;
        p[1].events = POLLIN;
        if (poll(p, 2, -1) < 0 && errno != EINTR) break;
        if (p[1].revents & (POLLIN | POLLHUP | POLLERR)) {
            unsigned char b[4096];
            ssize_t n = read(ana, b, sizeof(b));
            if (n <= 0) {
                if (n < 0 && (errno == EAGAIN || errno == EINTR)) continue;
                break;
            }
            for (i = 0; i < n; i++) bayt_isle(b[i]);
        }
    }
    if (dokum_istendi) dokum();
    XCloseDisplay(ay_d);
    return 0;
}
