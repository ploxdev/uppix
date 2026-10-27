/* fbfare: grafik kipte fare imleci. /dev/fb0'ı mmap eder, konsolu KD_GRAPHICS'e alır,
 * ekranı boyar ve /dev/input/mice'tan (PS/2 paketleri) okuduğu harekete göre bir ok
 * imleci çizer. Sol tuş basılıyken iz bırakır; q ya da Esc (/dev/input/event0) ile
 * çıkar ve metin kipine döner. Çıkışta son imleç konumunu ve tıklama sayısını yazar.
 * --kipte-cik: KD_GRAPHICS ve K_RAW'a geçip geri almadan çıkar (konsolu çekirdek
 * geri almalı). */
#include "linux.h"

#define IMLEC_G 12
#define IMLEC_Y 19

/* ok imleci: '#' kenar, '.' iç */
static const char* imlec[IMLEC_Y] = {
    "#",           "##",          "#.#",         "#..#",        "#...#",
    "#....#",      "#.....#",     "#......#",    "#.......#",   "#........#",
    "#.........#", "#..........#", "#......#####", "#...#..#",    "#..# #..#",
    "#.#  #..#",   "##    #..#",  "#     #..#",  "       ##",
};

static struct fb_var_screeninfo v;
static struct fb_fix_screeninfo f;
static unsigned char* fb;
static unsigned int alt[IMLEC_Y][IMLEC_G];
static int ix, iy;

static unsigned int renk(unsigned r, unsigned g, unsigned b) {
    return ((r >> (8 - v.red.length)) << v.red.offset) | ((g >> (8 - v.green.length)) << v.green.offset) |
           ((b >> (8 - v.blue.length)) << v.blue.offset);
}

static unsigned int* piksel(int x, int y) { return (unsigned int*)(fb + (unsigned long)y * f.line_length) + x; }

static int ekranda(int x, int y) { return x >= 0 && y >= 0 && x < (int)v.xres && y < (int)v.yres; }

/* imlecin altındaki pikseller saklanır, sonra imleç çizilir */
static void imlec_ciz(void) {
    int x, y;
    for (y = 0; y < IMLEC_Y; y++) {
        int uz = (int)uzunluk(imlec[y]);
        for (x = 0; x < IMLEC_G; x++) {
            char c = x < uz ? imlec[y][x] : 0;
            if (!ekranda(ix + x, iy + y)) continue;
            alt[y][x] = *piksel(ix + x, iy + y);
            if (c == '#') *piksel(ix + x, iy + y) = renk(0, 0, 0);
            if (c == '.') *piksel(ix + x, iy + y) = renk(255, 255, 255);
        }
    }
}

static void imlec_sil(void) {
    int x, y;
    for (y = 0; y < IMLEC_Y; y++) {
        for (x = 0; x < IMLEC_G; x++) {
            if (ekranda(ix + x, iy + y)) *piksel(ix + x, iy + y) = alt[y][x];
        }
    }
}

/* iz: (x0,y0)'dan (x1,y1)'e 3 piksel kalınlığında çizgi */
static void iz_ciz(int x0, int y0, int x1, int y1) {
    int dx = x1 > x0 ? x1 - x0 : x0 - x1, dy = y1 > y0 ? y1 - y0 : y0 - y1;
    int n = dx > dy ? dx : dy, i, a, b;
    if (n == 0) n = 1;
    for (i = 0; i <= n; i++) {
        int x = x0 + (x1 - x0) * i / n, y = y0 + (y1 - y0) * i / n;
        for (a = -1; a <= 1; a++) {
            for (b = -1; b <= 1; b++) {
                if (ekranda(x + a, y + b)) *piksel(x + a, y + b) = renk(255, 200, 0);
            }
        }
    }
}

int main(int argc, char** argv, char** envp) {
    long fd, tty, kb, ms, r;
    unsigned char p[96];
    struct input_event ev[16];
    struct timespec bekle = {0, 5000000};
    int x, y, i, sol = 0, tiklama = 0, bitti = 0, n = 0;
    (void)envp;
    if (argc > 1 && esit(argv[1], "--kipte-cik")) {
        tty = sys_open("/dev/tty0", O_RDWR, 0);
        sys_ioctl_deger((int)tty, KDSETMODE, KD_GRAPHICS);
        sys_ioctl_deger((int)tty, KDSKBMODE, K_RAW);
        yaz("fbfare: kip değiştirildi, geri alınmadan çıkılıyor\n");
        return 0;
    }
    fd = sys_open("/dev/fb0", O_RDWR, 0);
    if (fd < 0 || sys_ioctl((int)fd, FBIOGET_VSCREENINFO, &v) < 0 || sys_ioctl((int)fd, FBIOGET_FSCREENINFO, &f) < 0 ||
        v.bits_per_pixel != 32) {
        hyaz("fbfare: 32 bpp /dev/fb0 gerekli\n");
        return 1;
    }
    fb = (unsigned char*)sys_mmap(0, f.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, (int)fd, 0);
    tty = sys_open("/dev/tty0", O_RDWR, 0);
    kb = sys_open("/dev/input/event0", O_RDONLY | O_NONBLOCK, 0);
    ms = sys_open("/dev/input/mice", O_RDONLY | O_NONBLOCK, 0);
    if (fb == MAP_FAILED || tty < 0 || kb < 0 || ms < 0) {
        hyaz("fbfare: aygıtlar açılamadı\n");
        return 1;
    }
    sys_ioctl_deger((int)tty, KDSETMODE, KD_GRAPHICS);
    for (y = 0; y < (int)v.yres; y++) {
        for (x = 0; x < (int)v.xres; x++) *piksel(x, y) = renk(20, 40, 90 + y * 100 / (int)v.yres);
    }
    ix = (int)v.xres / 2;
    iy = (int)v.yres / 2;
    imlec_ciz();
    yaz("fbfare: hazır\n");
    while (!bitti) {
        r = sys_read((int)ms, p + n, sizeof(p) - (size_t)n);
        if (r > 0) n += (int)r;
        /* PS/2 paketleri: 3 bayt, 0. baytın 3. biti hep 1 (kaymışsa atlanır) */
        i = 0;
        while (n - i >= 3) {
            int b0 = p[i], dx = p[i + 1], dy = p[i + 2], ex = ix, ey = iy, yeni_sol;
            if (!(b0 & 8)) {
                i++;
                continue;
            }
            if (b0 & 0x10) dx -= 256;
            if (b0 & 0x20) dy -= 256;
            i += 3;
            imlec_sil();
            ix += dx;
            iy -= dy;
            if (ix < 0) ix = 0;
            if (iy < 0) iy = 0;
            if (ix >= (int)v.xres) ix = (int)v.xres - 1;
            if (iy >= (int)v.yres) iy = (int)v.yres - 1;
            yeni_sol = b0 & 1;
            if (sol && yeni_sol) iz_ciz(ex, ey, ix, iy);
            if (yeni_sol && !sol) tiklama++;
            if (!yeni_sol && sol) yaz("fbfare: bırakıldı\n");
            sol = yeni_sol;
            imlec_ciz();
        }
        for (x = 0; i + x < n; x++) p[x] = p[i + x];
        n -= i;
        r = sys_read((int)kb, ev, sizeof(ev));
        for (i = 0; r > 0 && i < (int)(r / (long)sizeof(ev[0])); i++) {
            if (ev[i].type == EV_KEY && ev[i].value == 1 && (ev[i].code == KEY_Q || ev[i].code == KEY_ESC)) bitti = 1;
        }
        if (!bitti) sys_nanosleep(&bekle, 0);
    }
    sys_ioctl_deger((int)tty, KDSETMODE, KD_TEXT);
    yaz("fbfare: bitti x=");
    sayi_yaz(ix);
    yaz(" y=");
    sayi_yaz(iy);
    yaz(" tıklama=");
    sayi_yaz(tiklama);
    yaz("\n");
    return 0;
}
