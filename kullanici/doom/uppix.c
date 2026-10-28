/* doom: doomgeneric'in Uppix arka ucu (ui/derle.sh musl ile statik derler). İki kipi var:
 *
 *   X11     DISPLAY (yoksa :0) açılabiliyorsa 640x400'lük bir pencere; tuşlar KeyPress /
 *           KeyRelease'ten, kare XPutImage ile.
 *   fb      X yoksa (ya da -fb verilirse) /dev/fb0'a tam ekran: kare ekrana sığan en büyük
 *           tam sayı katıyla büyütülüp ortalanır, konsol KD_GRAPHICS'e alınır; tuşlar
 *           /dev/input/event0'dan (açıkken konsol klavyeyi kabuğa vermez, EVIOCGRAB ile
 *           yalnızca bize gelir). Çıkışta ve sinyalde konsol metin kipine döner.
 *
 * Tuşlar her iki kipte de konumlarına göre (Linux KEY_* kodu; X'te keycode - 8) çevrilir,
 * böylece klavye düzeni (us/tr) oyunu etkilemez. Ayar ve kayıt dosyaları $HOME'a (yoksa
 * /root) yazılır. -x11 / -fb kipi zorlar; kalan bütün seçenekler Doom'a gider (-iwad,
 * -warp, -skill ...). Ses yok. */
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "doomgeneric.h"
#include "doomkeys.h"

void I_Quit(void);

#define GEN DOOMGENERIC_RESX
#define YUK DOOMGENERIC_RESY

/* <linux/input.h> doomkeys.h ile çakışan KEY_* adları tanımladığından olay yapısı ve
 * ioctl'lar burada */
struct olay {
    struct timeval zaman;
    unsigned short tur, kod;
    int deger;
};
#define EV_KEY_TUR 1
#define EVIOCGRAB_ 0x40044590
#define KDSETMODE_ 0x4B3A
#define KD_TEXT_ 0
#define KD_GRAPHICS_ 1

/* Linux KEY_* kodu → Doom tuşu (0: yok sayılır) */
static unsigned char tus_tablo[128];

static void tablo_kur(void) {
    static const char sira1[] = "1234567890-=";
    static const char sira2[] = "qwertyuiop[]";
    static const char sira3[] = "asdfghjkl;'`";
    static const char sira4[] = "\\zxcvbnm,./";
    int i;
    for (i = 0; sira1[i]; i++) tus_tablo[2 + i] = (unsigned char)sira1[i];
    for (i = 0; sira2[i]; i++) tus_tablo[16 + i] = (unsigned char)sira2[i];
    for (i = 0; sira3[i]; i++) tus_tablo[30 + i] = (unsigned char)sira3[i];
    for (i = 0; sira4[i]; i++) tus_tablo[43 + i] = (unsigned char)sira4[i];
    tus_tablo[12] = KEY_MINUS;
    tus_tablo[13] = KEY_EQUALS;
    tus_tablo[1] = KEY_ESCAPE;
    tus_tablo[14] = KEY_BACKSPACE;
    tus_tablo[15] = KEY_TAB;
    tus_tablo[28] = KEY_ENTER;
    tus_tablo[96] = KEY_ENTER;        /* tuş takımı Enter */
    tus_tablo[29] = KEY_FIRE;         /* sol Ctrl */
    tus_tablo[97] = KEY_FIRE;         /* sağ Ctrl */
    tus_tablo[57] = KEY_USE;          /* boşluk */
    tus_tablo[42] = KEY_RSHIFT;       /* Shift: koş */
    tus_tablo[54] = KEY_RSHIFT;
    tus_tablo[56] = KEY_LALT;         /* Alt: yana kay */
    tus_tablo[100] = KEY_LALT;
    tus_tablo[58] = KEY_CAPSLOCK;
    for (i = 0; i < 10; i++) tus_tablo[59 + i] = (unsigned char)(KEY_F1 + i);
    tus_tablo[87] = KEY_F11;
    tus_tablo[88] = KEY_F12;
    tus_tablo[102] = KEY_HOME;
    tus_tablo[103] = KEY_UPARROW;
    tus_tablo[104] = KEY_PGUP;
    tus_tablo[105] = KEY_LEFTARROW;
    tus_tablo[106] = KEY_RIGHTARROW;
    tus_tablo[107] = KEY_END;
    tus_tablo[108] = KEY_DOWNARROW;
    tus_tablo[109] = KEY_PGDN;
    tus_tablo[110] = KEY_INS;
    tus_tablo[111] = KEY_DEL;
    tus_tablo[119] = KEY_PAUSE;
    /* tuş takımı oklar (NumLock kapalıyken) */
    tus_tablo[72] = KEY_UPARROW;
    tus_tablo[75] = KEY_LEFTARROW;
    tus_tablo[77] = KEY_RIGHTARROW;
    tus_tablo[80] = KEY_DOWNARROW;
}

/* ---- tuş kuyruğu ---- */
#define KUYRUK 64
static unsigned short kuyruk[KUYRUK];
static unsigned kuyruk_yaz, kuyruk_oku;
static unsigned char basili[128];

static void tus_ekle(unsigned kod, int bas) {
    unsigned char t;
    if (kod >= 128 || !(t = tus_tablo[kod])) return;
    if (basili[kod] == (bas != 0)) return; /* otomatik yineleme */
    basili[kod] = bas != 0;
    if ((kuyruk_yaz + 1) % KUYRUK == kuyruk_oku) return;
    kuyruk[kuyruk_yaz] = (unsigned short)((bas ? 0x100 : 0) | t);
    kuyruk_yaz = (kuyruk_yaz + 1) % KUYRUK;
}

/* odak gidince basılı kalan tuşlar bırakılır (yoksa oyuncu yürümeye devam eder) */
static void hepsini_birak(void) {
    unsigned k;
    for (k = 0; k < 128; k++) {
        if (basili[k]) tus_ekle(k, 0);
    }
}

/* ---- X11 ---- */
static Display* ekran;
static Window pencere;
static GC gc;
static XImage* resim;
static unsigned int* donusum; /* sunucunun piksel biçimi farklıysa çevrilmiş kare */
static Atom sil_atom;

static int maske_kayma(unsigned long m) {
    int k = 0;
    while (m && !(m & 1)) {
        m >>= 1;
        k++;
    }
    return k;
}

static int x_ac(void) {
    const char* ad = getenv("DISPLAY");
    XSizeHints boy;
    XClassHint sinif;
    Visual* gorsel;
    int derinlik, e;
    ekran = XOpenDisplay(ad && *ad ? ad : ":0");
    if (!ekran) return 0;
    e = DefaultScreen(ekran);
    gorsel = DefaultVisual(ekran, e);
    derinlik = DefaultDepth(ekran, e);
    pencere = XCreateSimpleWindow(ekran, RootWindow(ekran, e), 0, 0, GEN, YUK, 0, 0, BlackPixel(ekran, e));
    boy.flags = PMinSize | PMaxSize;
    boy.min_width = boy.max_width = GEN;
    boy.min_height = boy.max_height = YUK;
    XSetWMNormalHints(ekran, pencere, &boy);
    sinif.res_name = "doom";
    sinif.res_class = "Doom";
    XSetClassHint(ekran, pencere, &sinif);
    XStoreName(ekran, pencere, "Doom");
    sil_atom = XInternAtom(ekran, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(ekran, pencere, &sil_atom, 1);
    XSelectInput(ekran, pencere, KeyPressMask | KeyReleaseMask | FocusChangeMask | ExposureMask | StructureNotifyMask);
    gc = XCreateGC(ekran, pencere, 0, NULL);
    resim = XCreateImage(ekran, gorsel, (unsigned)derinlik, ZPixmap, 0, NULL, GEN, YUK, 32, 0);
    if (!resim) {
        fprintf(stderr, "doom: XImage oluşturulamadı\n");
        exit(1);
    }
    /* Doom'un karesi XRGB8888 (kırmızı 16. bitte); sunucu da öyleyse doğrudan gönderilir */
    if (resim->bits_per_pixel == 32 && gorsel->red_mask == 0xff0000 && gorsel->green_mask == 0xff00 &&
        gorsel->blue_mask == 0xff) {
        resim->data = (char*)DG_ScreenBuffer;
    } else {
        donusum = calloc(1, (size_t)resim->bytes_per_line * YUK);
        resim->data = (char*)donusum;
    }
    XMapWindow(ekran, pencere);
    XFlush(ekran);
    printf("doom: X penceresi %dx%d\n", GEN, YUK);
    fflush(stdout);
    return 1;
}

static void x_olaylar(void) {
    XEvent o;
    while (XPending(ekran)) {
        XNextEvent(ekran, &o);
        switch (o.type) {
        case KeyPress:
            tus_ekle(o.xkey.keycode - 8, 1);
            break;
        case KeyRelease:
            /* X'in otomatik yinelemesi bırak+bas çifti gönderir: hemen ardından aynı anda
             * gelen basış varsa ikisi de yutulur */
            if (XEventsQueued(ekran, QueuedAfterReading)) {
                XEvent s;
                XPeekEvent(ekran, &s);
                if (s.type == KeyPress && s.xkey.keycode == o.xkey.keycode && s.xkey.time == o.xkey.time) {
                    XNextEvent(ekran, &s);
                    break;
                }
            }
            tus_ekle(o.xkey.keycode - 8, 0);
            break;
        case FocusOut:
            hepsini_birak();
            break;
        case ClientMessage:
            if ((Atom)o.xclient.data.l[0] == sil_atom) I_Quit();
            break;
        }
    }
}

static void x_ciz(void) {
    if (donusum) {
        Visual* g = DefaultVisual(ekran, DefaultScreen(ekran));
        int kr = maske_kayma(g->red_mask), kg = maske_kayma(g->green_mask), kb = maske_kayma(g->blue_mask);
        unsigned long mr = g->red_mask >> kr, mg = g->green_mask >> kg, mb = g->blue_mask >> kb;
        int x, y;
        for (y = 0; y < YUK; y++) {
            for (x = 0; x < GEN; x++) {
                unsigned p = DG_ScreenBuffer[y * GEN + x];
                unsigned long d = (((p >> 16) & 255) * mr / 255) << kr | (((p >> 8) & 255) * mg / 255) << kg |
                                  ((p & 255) * mb / 255) << kb;
                XPutPixel(resim, x, y, d);
            }
        }
    }
    XPutImage(ekran, pencere, gc, resim, 0, 0, 0, 0, GEN, YUK);
    XFlush(ekran);
    x_olaylar();
}

/* ---- çerçeve tamponu ---- */
static int fb_tty = -1, fb_klavye = -1;
static unsigned char* fb;
static struct fb_var_screeninfo fv;
static struct fb_fix_screeninfo ff;
static int fb_olcek, fb_x, fb_y;

static void fb_kapat(void) {
    if (fb_tty >= 0) {
        ioctl(fb_tty, KDSETMODE_, KD_TEXT_);
        close(fb_tty);
        fb_tty = -1;
    }
    if (fb_klavye >= 0) {
        ioctl(fb_klavye, EVIOCGRAB_, 0);
        close(fb_klavye);
        fb_klavye = -1;
    }
}

static void sinyal(int s) {
    (void)s;
    fb_kapat();
    _exit(1);
}

static void fb_ac(void) {
    int d = open("/dev/fb0", O_RDWR);
    size_t boy;
    if (d < 0 || ioctl(d, FBIOGET_VSCREENINFO, &fv) < 0 || ioctl(d, FBIOGET_FSCREENINFO, &ff) < 0) {
        fprintf(stderr, "doom: /dev/fb0 açılamadı (%s)\n", strerror(errno));
        exit(1);
    }
    if (fv.bits_per_pixel != 32 && fv.bits_per_pixel != 16) {
        fprintf(stderr, "doom: %u bit/piksel desteklenmiyor\n", fv.bits_per_pixel);
        exit(1);
    }
    boy = (size_t)ff.line_length * fv.yres;
    fb = mmap(NULL, boy, PROT_READ | PROT_WRITE, MAP_SHARED, d, 0);
    if (fb == MAP_FAILED) {
        fprintf(stderr, "doom: /dev/fb0 eşlenemedi\n");
        exit(1);
    }
    fb_klavye = open("/dev/input/event0", O_RDONLY | O_NONBLOCK);
    if (fb_klavye < 0) {
        fprintf(stderr, "doom: /dev/input/event0 açılamadı\n");
        exit(1);
    }
    ioctl(fb_klavye, EVIOCGRAB_, 1);
    fb_tty = open("/dev/tty0", O_RDWR);
    atexit(fb_kapat);
    signal(SIGINT, sinyal);
    signal(SIGTERM, sinyal);
    if (fb_tty >= 0) ioctl(fb_tty, KDSETMODE_, KD_GRAPHICS_);
    fb_olcek = (int)(fv.xres / GEN < fv.yres / YUK ? fv.xres / GEN : fv.yres / YUK);
    if (fb_olcek < 1) fb_olcek = 1;
    fb_x = ((int)fv.xres - GEN * fb_olcek) / 2;
    fb_y = ((int)fv.yres - YUK * fb_olcek) / 2;
    if (fb_x < 0) fb_x = 0;
    if (fb_y < 0) fb_y = 0;
    memset(fb, 0, boy);
    printf("doom: çerçeve tamponu %ux%u, ölçek %d\n", fv.xres, fv.yres, fb_olcek);
    fflush(stdout);
}

static void fb_olaylar(void) {
    struct olay o[32];
    ssize_t n;
    int i;
    while ((n = read(fb_klavye, o, sizeof(o))) > 0) {
        for (i = 0; i < (int)(n / (ssize_t)sizeof(o[0])); i++) {
            if (o[i].tur == EV_KEY_TUR && o[i].deger != 2) tus_ekle(o[i].kod, o[i].deger);
        }
    }
}

static void fb_ciz(void) {
    int gen = (int)fv.xres - fb_x < GEN * fb_olcek ? ((int)fv.xres - fb_x) / fb_olcek : GEN;
    int yuk = (int)fv.yres - fb_y < YUK * fb_olcek ? ((int)fv.yres - fb_y) / fb_olcek : YUK;
    int x, y, k;
    for (y = 0; y < yuk; y++) {
        const unsigned* kaynak = DG_ScreenBuffer + y * GEN;
        for (k = 0; k < fb_olcek; k++) {
            unsigned char* satir = fb + (size_t)(fb_y + y * fb_olcek + k) * ff.line_length;
            if (fv.bits_per_pixel == 32) {
                unsigned* h = (unsigned*)satir + fb_x;
                if (fb_olcek == 1 && fv.red.offset == 16 && fv.green.offset == 8 && fv.blue.offset == 0) {
                    memcpy(h, kaynak, (size_t)gen * 4);
                    continue;
                }
                for (x = 0; x < gen * fb_olcek; x++) {
                    unsigned p = kaynak[x / fb_olcek];
                    h[x] = ((p >> 16 & 255) >> (8 - fv.red.length)) << fv.red.offset |
                           ((p >> 8 & 255) >> (8 - fv.green.length)) << fv.green.offset |
                           ((p & 255) >> (8 - fv.blue.length)) << fv.blue.offset;
                }
            } else {
                unsigned short* h = (unsigned short*)satir + fb_x;
                for (x = 0; x < gen * fb_olcek; x++) {
                    unsigned p = kaynak[x / fb_olcek];
                    h[x] = (unsigned short)(((p >> 16 & 255) >> (8 - fv.red.length)) << fv.red.offset |
                                            ((p >> 8 & 255) >> (8 - fv.green.length)) << fv.green.offset |
                                            ((p & 255) >> (8 - fv.blue.length)) << fv.blue.offset);
                }
            }
        }
    }
    fb_olaylar();
}

/* ---- doomgeneric arayüzü ---- */
static int x_kipi;
static int zorla_fb, zorla_x;

void DG_Init(void) {
    tablo_kur();
    if (!zorla_fb && x_ac()) {
        x_kipi = 1;
        return;
    }
    if (zorla_x) {
        fprintf(stderr, "doom: X sunucusuna bağlanılamadı (DISPLAY?)\n");
        exit(1);
    }
    fb_ac();
}

void DG_DrawFrame(void) {
    if (x_kipi)
        x_ciz();
    else
        fb_ciz();
}

void DG_SleepMs(uint32_t ms) { usleep(ms * 1000); }

uint32_t DG_GetTicksMs(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (uint32_t)(t.tv_sec * 1000 + t.tv_nsec / 1000000);
}

int DG_GetKey(int* basildi, unsigned char* tus) {
    if (kuyruk_oku == kuyruk_yaz) {
        if (x_kipi)
            x_olaylar();
        else
            fb_olaylar();
    }
    if (kuyruk_oku == kuyruk_yaz) return 0;
    *basildi = kuyruk[kuyruk_oku] >> 8;
    *tus = (unsigned char)(kuyruk[kuyruk_oku] & 0xff);
    kuyruk_oku = (kuyruk_oku + 1) % KUYRUK;
    return 1;
}

void DG_SetWindowTitle(const char* baslik) {
    if (x_kipi) XStoreName(ekran, pencere, baslik);
}

static void bitti(void) {
    printf("doom: bitti\n");
    fflush(stdout);
}

/* doomgeneric yalnızca çalışma dizininde arar (DOOMWADDIR ve /usr/share/games/doom kapalı):
 * -iwad verilmediyse oradaki ilk bölüm dosyası, Doom'un kendi sırasıyla seçilir */
#define WAD_DIZIN "/usr/share/games/doom/"
static char* wad_bul(void) {
    static const char* adlar[] = {"doom2.wad", "plutonia.wad", "tnt.wad", "doom.wad", "doom1.wad",
                                  "freedoom2.wad", "freedoom1.wad"};
    static char yol[64];
    unsigned i;
    for (i = 0; i < sizeof(adlar) / sizeof(adlar[0]); i++) {
        snprintf(yol, sizeof(yol), "%s%s", WAD_DIZIN, adlar[i]);
        if (access(yol, R_OK) == 0) return yol;
    }
    return NULL;
}

int main(int argc, char** argv) {
    const char* ev = getenv("HOME");
    char** arg = calloc((size_t)argc + 3, sizeof(char*));
    char* wad;
    int i, j, iwad = 0;
    atexit(bitti);
    /* -x11 / -fb bizim; Doom'a gitmez */
    arg[0] = argv[0];
    for (i = j = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-fb"))
            zorla_fb = 1;
        else if (!strcmp(argv[i], "-x11"))
            zorla_x = 1;
        else {
            if (!strcmp(argv[i], "-iwad")) iwad = 1;
            arg[j++] = argv[i];
        }
    }
    if (!iwad && (wad = wad_bul())) {
        arg[j++] = "-iwad";
        arg[j++] = wad;
    }
    /* default.cfg ve kayıtlar çalışma dizinine yazılır */
    if (!ev || !*ev || chdir(ev) != 0) {
        if (chdir("/root") != 0) chdir("/tmp");
    }
    doomgeneric_Create(j, arg);
    for (;;) doomgeneric_Tick();
    return 0;
}
