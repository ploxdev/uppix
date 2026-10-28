/* ayarlar: Uppix masaüstü ayarları (libX11; ui/derle.sh derler).
 * Tema (masaüstü degradesi, pencere başlıkları, şerit ve menü renkleri), klavye düzeni
 * (us / tr), şeritteki saatin biçimi ve sistem bilgisi (çekirdek, bellek, ekran, çalışma
 * süresi). "Uygula":
 *   /etc/jwm/tema.jwmrc   şerit ve görünüm (system.jwmrc bunu Include eder)
 *   /etc/jwm/ayarlar      seçimler (tema=, saat=, klavye=)
 *   /etc/X11/klavye       düzen; startx X'i bununla başlatır (-xkb-layout)
 * yazar, düzeni XKB ile hemen yükler ve "jwm -restart" çalıştırır (JWM 2.4'te -reload yalnızca
 * menüyü yeniden yükler; görünüm yeniden başlatmayla gelir, pencereler yerinde kalır).
 *   ayarlar                    pencere
 *   ayarlar --uygula T S K     penceresiz: tema T, saat S, klavye K (us|tr) yazılır, uygulanır
 *   ayarlar --tema-yaz DOSYA   varsayılan temayı DOSYA'ya yazar (kok/etc/jwm/tema.jwmrc böyle üretildi) */
#include "arayuz.h"
#include <X11/XKBlib.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <time.h>

typedef struct {
    const char* ad;
    unsigned long masa1, masa2;     /* masaüstü degradesi (üst, alt) */
    unsigned long serit1, serit2;   /* şerit */
    unsigned long baslik1, baslik2; /* etkin pencere başlığı */
    unsigned long pasif1, pasif2;   /* etkin olmayan pencere başlığı */
    unsigned long vurgu;            /* etkin görev, seçili menü öğesi */
} Tema;

static const Tema temalar[] = {
    {"Gece", 0x101B2E, 0x35679C, 0x2C3E58, 0x172234, 0x4A8FE7, 0x2A5D9A, 0x626C7A, 0x434A55, 0x3A7BD5},
    {"Orman", 0x0E2219, 0x3F7F5C, 0x2D4A3B, 0x16261D, 0x5FB27C, 0x2E7048, 0x626C66, 0x434A46, 0x3C9A63},
    {"Gün batımı", 0x281338, 0xC9694A, 0x4A2B52, 0x24142E, 0xE8875A, 0xAE4E38, 0x6E6270, 0x4A424E, 0xD0663F},
    {"Grafit", 0x1C1E21, 0x5A5F66, 0x36393E, 0x1D1F22, 0x7D848E, 0x4C5158, 0x60646A, 0x404348, 0x5E86C1},
};
#define TEMA_SAYI 4

static const char* saatler[] = {"%H:%M", "%H:%M:%S", "%d.%m %H:%M", "%d.%m.%Y %H:%M"};
#define SAAT_SAYI 4

static int tema = 0, saat = 0, klavye = 0; /* klavye 0: us, 1: tr */
static Window pencere;
static GC gc;
static AyDugme d_tema[TEMA_SAYI], d_saat[SAAT_SAYI], d_klavye[2], d_uygula, d_kapat;
static char durum_metni[160];
static const int G = 540, Y = 430;

/* ---------------------------------------------------------------- dosyalar */

static void ayarlari_oku(void) {
    char b[256];
    FILE* f = fopen("/etc/jwm/ayarlar", "r");
    if (!f) return;
    while (fgets(b, sizeof(b), f)) {
        if (!strncmp(b, "tema=", 5)) tema = atoi(b + 5);
        else if (!strncmp(b, "saat=", 5)) saat = atoi(b + 5);
        else if (!strncmp(b, "klavye=", 7)) klavye = !strncmp(b + 7, "tr", 2);
    }
    fclose(f);
    if (tema < 0 || tema >= TEMA_SAYI) tema = 0;
    if (saat < 0 || saat >= SAAT_SAYI) saat = 0;
}

static void renk_yaz(FILE* f, unsigned long r) { fprintf(f, "#%06lX", r & 0xFFFFFF); }

/* a'dan b'ye t/8 oranında karışım */
static unsigned long karistir(unsigned long a, unsigned long b, int t) {
    unsigned long r = 0;
    int k;
    for (k = 16; k >= 0; k -= 8) {
        long x = (long)((a >> k) & 255), y = (long)((b >> k) & 255);
        r |= (unsigned long)(x + (y - x) * t / 8) << k;
    }
    return r;
}

static void degrade_yaz(FILE* f, unsigned long a, unsigned long b) {
    renk_yaz(f, a);
    fputc(':', f);
    renk_yaz(f, b);
}

/* JWM'nin okuduğu tema dosyası: şerit, stiller ve masalar. */
static int tema_yaz(const char* yol) {
    const Tema* t = &temalar[tema];
    FILE* f = fopen(yol, "w");
    if (!f) return 0;
    fprintf(f, "<?xml version=\"1.0\"?>\n");
    fprintf(f, "<!-- ayarlar programinin yazdigi tema (%s); her \"Uygula\"da yeniden yazilir. -->\n", t->ad);
    fprintf(f, "<JWM>\n");
    fprintf(f, "    <Tray x=\"0\" y=\"-1\" height=\"26\" autohide=\"off\">\n");
    fprintf(f, "        <TrayButton label=\"Uppix\" popup=\"Uygulamalar\">root:1</TrayButton>\n");
    fprintf(f, "        <Spacer width=\"2\"/>\n");
    fprintf(f, "        <TrayButton label=\"Terminal\" popup=\"Terminal\">exec:/usr/bin/uterm</TrayButton>\n");
    fprintf(f, "        <Spacer width=\"4\"/>\n");
    fprintf(f, "        <Pager labeled=\"true\"/>\n");
    fprintf(f, "        <Spacer width=\"4\"/>\n");
    fprintf(f, "        <TaskList maxwidth=\"220\"/>\n");
    fprintf(f, "        <Clock format=\"%s\"/>\n", saatler[saat]);
    fprintf(f, "        <Spacer width=\"2\"/>\n");
    fprintf(f, "        <TrayButton label=\"Oturum\" popup=\"Oturumu kapat, yeniden baslat, kapat\">exec:/usr/bin/oturum</TrayButton>\n");
    fprintf(f, "    </Tray>\n\n");
    fprintf(f, "    <WindowStyle>\n        <Font>fixed</Font>\n        <Width>4</Width>\n        <Height>20</Height>\n");
    fprintf(f, "        <Corner>5</Corner>\n        <Foreground>#E6EAF0</Foreground>\n        <Background>");
    degrade_yaz(f, t->pasif1, t->pasif2);
    fprintf(f, "</Background>\n        <Outline>#10151E</Outline>\n        <Active>\n            <Foreground>#FFFFFF</Foreground>\n            <Background>");
    degrade_yaz(f, t->baslik1, t->baslik2);
    fprintf(f, "</Background>\n            <Outline>#0C1A2C</Outline>\n        </Active>\n    </WindowStyle>\n\n");
    fprintf(f, "    <TrayStyle>\n        <Font>fixed</Font>\n        <Background>");
    degrade_yaz(f, t->serit1, t->serit2);
    fprintf(f, "</Background>\n        <Foreground>#FFFFFF</Foreground>\n    </TrayStyle>\n\n");
    fprintf(f, "    <TrayButtonStyle>\n        <Font>fixed</Font>\n        <Foreground>#FFFFFF</Foreground>\n        <Background>");
    degrade_yaz(f, karistir(t->serit1, t->baslik1, 3), karistir(t->serit2, t->baslik2, 3));
    fprintf(f, "</Background>\n        <Active>\n            <Foreground>#FFFFFF</Foreground>\n            <Background>");
    renk_yaz(f, t->vurgu);
    fprintf(f, "</Background>\n        </Active>\n    </TrayButtonStyle>\n\n");
    fprintf(f, "    <TaskListStyle list=\"desktop\" group=\"false\">\n        <Font>fixed</Font>\n        <Foreground>#E6EAF0</Foreground>\n        <Background>");
    degrade_yaz(f, t->serit1, t->serit2);
    fprintf(f, "</Background>\n        <Active>\n            <Foreground>#FFFFFF</Foreground>\n            <Background>");
    degrade_yaz(f, t->baslik1, t->baslik2);
    fprintf(f, "</Background>\n        </Active>\n    </TaskListStyle>\n\n");
    fprintf(f, "    <PagerStyle>\n        <Font>fixed</Font>\n        <Outline>#0A0F18</Outline>\n        <Foreground>");
    renk_yaz(f, t->pasif2);
    fprintf(f, "</Foreground>\n        <Background>");
    renk_yaz(f, t->serit2);
    fprintf(f, "</Background>\n        <Text>#FFFFFF</Text>\n        <Active>\n            <Foreground>");
    renk_yaz(f, t->baslik1);
    fprintf(f, "</Foreground>\n            <Background>");
    renk_yaz(f, t->baslik2);
    fprintf(f, "</Background>\n        </Active>\n    </PagerStyle>\n\n");
    fprintf(f, "    <ClockStyle>\n        <Font>fixed</Font>\n        <Foreground>#FFFFFF</Foreground>\n        <Background>");
    degrade_yaz(f, t->serit1, t->serit2);
    fprintf(f, "</Background>\n    </ClockStyle>\n\n");
    fprintf(f, "    <MenuStyle>\n        <Font>fixed</Font>\n        <Foreground>#1C2430</Foreground>\n        <Background>#F3F5F8</Background>\n");
    fprintf(f, "        <Outline>#5A6575</Outline>\n        <Active>\n            <Foreground>#FFFFFF</Foreground>\n            <Background>");
    degrade_yaz(f, t->baslik1, t->baslik2);
    fprintf(f, "</Background>\n        </Active>\n    </MenuStyle>\n\n");
    fprintf(f, "    <PopupStyle>\n        <Font>fixed</Font>\n        <Foreground>#1C2430</Foreground>\n        <Background>#FFFBE6</Background>\n        <Outline>#5A6575</Outline>\n    </PopupStyle>\n\n");
    fprintf(f, "    <!-- Background, Desktop'lardan once: JWM masa sayisi kadar Desktop okuyunca gerisini atlar -->\n");
    fprintf(f, "    <Desktops width=\"2\" height=\"1\">\n        <Background type=\"gradient\">");
    degrade_yaz(f, t->masa1, t->masa2);
    fprintf(f, "</Background>\n        <Desktop name=\"1\"/>\n        <Desktop name=\"2\"/>\n    </Desktops>\n</JWM>\n");
    return fclose(f) == 0;
}

static int dosya_yaz(const char* yol, const char* icerik) {
    FILE* f = fopen(yol, "w");
    if (!f) return 0;
    fputs(icerik, f);
    return fclose(f) == 0;
}

/* Düzeni hemen yükler: sunucu bileşenleri xkbcomp ile derler (kurallar ui/xkb/evdev ile aynı). */
static int klavye_uygula(Display* d) {
    XkbComponentNamesRec ad;
    XkbDescPtr x;
    memset(&ad, 0, sizeof(ad));
    ad.keycodes = "evdev+aliases(qwerty)";
    ad.types = "complete";
    ad.compat = "complete";
    ad.symbols = klavye ? "pc+tr+inet(evdev)" : "pc+us+inet(evdev)";
    ad.geometry = "pc(pc105)";
    x = XkbGetKeyboardByName(d, XkbUseCoreKbd, &ad, XkbGBN_AllComponentsMask, XkbGBN_AllComponentsMask, True);
    if (!x) return 0;
    XkbFreeKeyboard(x, XkbAllComponentsMask, True);
    return 1;
}

static int uygula(Display* d) {
    char b[128];
    int ok = 1;
    char* jwm[] = {"jwm", "-restart", NULL};
    mkdir("/etc/jwm", 0755);
    mkdir("/etc/X11", 0755);
    ok &= tema_yaz("/etc/jwm/tema.jwmrc");
    snprintf(b, sizeof(b), "tema=%d\nsaat=%d\nklavye=%s\n", tema, saat, klavye ? "tr" : "us");
    ok &= dosya_yaz("/etc/jwm/ayarlar", b);
    ok &= dosya_yaz("/etc/X11/klavye", klavye ? "tr\n" : "us\n");
    if (d) {
        if (!klavye_uygula(d)) {
            snprintf(durum_metni, sizeof(durum_metni), "Klavye düzeni yüklenemedi (sonraki startx'te geçerli).");
            ok = 0;
        }
        ay_calistir("/usr/bin/jwm", jwm);
    }
    printf("ayarlar: tema %s, saat %s, klavye %s%s\n", temalar[tema].ad, saatler[saat], klavye ? "tr" : "us",
           ok ? "" : " (hata)");
    return ok;
}

/* ---------------------------------------------------------------- sistem bilgisi */

static void sistem_bilgisi(char satir[4][256]) {
    struct utsname u;
    char b[256];
    long top = 0, bos = 0;
    double sn = 0;
    FILE* f;
    if (uname(&u) == 0) snprintf(satir[0], 256, "Çekirdek:  %s %s %s", u.sysname, u.release, u.machine);
    else snprintf(satir[0], 256, "Çekirdek:  ?");
    f = fopen("/proc/meminfo", "r");
    if (f) {
        while (fgets(b, sizeof(b), f)) {
            if (!strncmp(b, "MemTotal:", 9)) top = atol(b + 9);
            else if (!strncmp(b, "MemFree:", 8)) bos = atol(b + 8);
        }
        fclose(f);
    }
    snprintf(satir[1], 256, "Bellek:    %ld MiB toplam, çekirdek yığınında %ld MiB boş", top / 1024, bos / 1024);
    snprintf(satir[2], 256, "Ekran:     %dx%d, %d bit renk", DisplayWidth(ay_d, ay_ekran), DisplayHeight(ay_d, ay_ekran),
             DefaultDepth(ay_d, ay_ekran));
    f = fopen("/proc/uptime", "r");
    if (f) {
        if (fscanf(f, "%lf", &sn) != 1) sn = 0;
        fclose(f);
    }
    snprintf(satir[3], 256, "Açık:      %ld sa %ld dk %ld sn", (long)sn / 3600, ((long)sn / 60) % 60, (long)sn % 60);
}

/* ---------------------------------------------------------------- çizim */

static void bolum(int y, const char* ad) {
    ay_metin(pencere, gc, 18, y, ad, AY_VURGU2);
    XSetForeground(ay_d, gc, ay_renk(AY_CIZGI));
    XDrawLine(ay_d, pencere, gc, 18 + ay_metin_gen(ad) + 8, y + 7, G - 18, y + 7);
}

static void ciz(void) {
    char bilgi[4][256];
    int i;
    ay_degrade(pencere, gc, 0, 0, G, 40, AY_VURGU1, AY_VURGU2);
    ay_metin(pencere, gc, 18, 8, "Uppix Ayarlar", 0xFFFFFF);
    ay_metin(pencere, gc, 18, 22, "görünüm, klavye, saat ve sistem bilgisi", 0xDCE8F8);
    XSetForeground(ay_d, gc, ay_renk(AY_ZEMIN));
    XFillRectangle(ay_d, pencere, gc, 0, 40, (unsigned)G, (unsigned)(Y - 40));

    bolum(52, "Tema");
    for (i = 0; i < TEMA_SAYI; i++) {
        const Tema* t = &temalar[i];
        AyDugme* b = &d_tema[i];
        int sec = i == tema;
        /* küçük önizleme: masaüstü degradesi, pencere başlığı ve şerit */
        ay_degrade(pencere, gc, b->x, b->y, b->g, b->h, t->masa1, t->masa2);
        ay_degrade(pencere, gc, b->x + 14, b->y + 10, b->g - 40, 7, t->baslik1, t->baslik2);
        XSetForeground(ay_d, gc, ay_renk(0xF3F5F8));
        XFillRectangle(ay_d, pencere, gc, b->x + 14, b->y + 17, (unsigned)(b->g - 40), 18);
        ay_degrade(pencere, gc, b->x, b->y + b->h - 8, b->g, 8, t->serit1, t->serit2);
        ay_cerceve(pencere, gc, b->x - 1, b->y - 1, b->g + 2, b->h + 2, sec ? AY_VURGU2 : AY_CIZGI);
        if (sec) ay_cerceve(pencere, gc, b->x - 2, b->y - 2, b->g + 4, b->h + 4, AY_VURGU1);
        ay_metin(pencere, gc, b->x + (b->g - ay_metin_gen(t->ad)) / 2, b->y + b->h + 5, t->ad, sec ? AY_VURGU2 : AY_YAZI);
    }

    bolum(150, "Klavye düzeni");
    for (i = 0; i < 2; i++) {
        d_klavye[i].secili = i == klavye;
        ay_dugme_ciz(pencere, gc, &d_klavye[i]);
    }

    bolum(208, "Saat biçimi");
    for (i = 0; i < SAAT_SAYI; i++) {
        char b[64];
        time_t z = time(NULL);
        strftime(b, sizeof(b), saatler[i], localtime(&z));
        d_saat[i].secili = i == saat;
        d_saat[i].ad = b;
        ay_dugme_ciz(pencere, gc, &d_saat[i]);
        d_saat[i].ad = NULL;
    }

    bolum(266, "Sistem");
    sistem_bilgisi(bilgi);
    XSetForeground(ay_d, gc, ay_renk(AY_PANEL));
    XFillRectangle(ay_d, pencere, gc, 18, 284, (unsigned)(G - 36), 74);
    ay_cerceve(pencere, gc, 18, 284, G - 36, 74, AY_CIZGI);
    for (i = 0; i < 4; i++) ay_metin(pencere, gc, 28, 290 + i * 16, bilgi[i], AY_YAZI);

    XSetForeground(ay_d, gc, ay_renk(AY_CIZGI));
    XDrawLine(ay_d, pencere, gc, 0, Y - 50, G, Y - 50);
    ay_metin(pencere, gc, 18, Y - 32, durum_metni, AY_SOLUK);
    d_uygula.secili = 1;
    ay_dugme_ciz(pencere, gc, &d_uygula);
    ay_dugme_ciz(pencere, gc, &d_kapat);
}

static void dugmeleri_kur(void) {
    int i;
    static const char* kl[2] = {"US (İngilizce)", "TR (Türkçe Q)"};
    for (i = 0; i < TEMA_SAYI; i++) {
        d_tema[i].x = 22 + i * 128;
        d_tema[i].y = 70;
        d_tema[i].g = 112;
        d_tema[i].h = 52;
    }
    for (i = 0; i < 2; i++) {
        d_klavye[i].x = 22 + i * 158;
        d_klavye[i].y = 168;
        d_klavye[i].g = 146;
        d_klavye[i].h = 26;
        d_klavye[i].ad = kl[i];
    }
    for (i = 0; i < SAAT_SAYI; i++) {
        d_saat[i].x = 22 + i * 126;
        d_saat[i].y = 226;
        d_saat[i].g = 116;
        d_saat[i].h = 26;
    }
    d_uygula.x = G - 222;
    d_uygula.y = Y - 38;
    d_uygula.g = 100;
    d_uygula.h = 28;
    d_uygula.ad = "Uygula";
    d_kapat.x = G - 112;
    d_kapat.y = Y - 38;
    d_kapat.g = 94;
    d_kapat.h = 28;
    d_kapat.ad = "Kapat";
}

static int tikla(int x, int y) {
    int i;
    for (i = 0; i < TEMA_SAYI; i++)
        if (ay_icinde(&d_tema[i], x, y)) tema = i;
    for (i = 0; i < 2; i++)
        if (ay_icinde(&d_klavye[i], x, y)) klavye = i;
    for (i = 0; i < SAAT_SAYI; i++)
        if (ay_icinde(&d_saat[i], x, y)) saat = i;
    if (ay_icinde(&d_uygula, x, y)) {
        durum_metni[0] = 0;
        if (uygula(ay_d) && !durum_metni[0]) snprintf(durum_metni, sizeof(durum_metni), "Uygulandı ve kaydedildi.");
    }
    if (ay_icinde(&d_kapat, x, y)) return 1;
    return 0;
}

int main(int argc, char** argv) {
    XEvent e;
    setvbuf(stdout, NULL, _IONBF, 0);
    if (argc == 3 && !strcmp(argv[1], "--tema-yaz")) return tema_yaz(argv[2]) ? 0 : 1;
    ayarlari_oku();
    if (argc == 5 && !strcmp(argv[1], "--uygula")) {
        tema = atoi(argv[2]) % TEMA_SAYI;
        saat = atoi(argv[3]) % SAAT_SAYI;
        klavye = !strcmp(argv[4], "tr");
        if (!ay_ac()) return uygula(NULL) ? 0 : 1;
        return uygula(ay_d) ? 0 : 1;
    }
    if (!ay_ac()) {
        fprintf(stderr, "ayarlar: ekran açılamadı (DISPLAY?)\n");
        return 1;
    }
    dugmeleri_kur();
    snprintf(durum_metni, sizeof(durum_metni), "Seçin ve \"Uygula\"ya basın.");
    pencere = ay_pencere("Ayarlar", "ayarlar", G, Y, AY_ZEMIN);
    gc = XCreateGC(ay_d, pencere, 0, NULL);
    if (ay_font) XSetFont(ay_d, gc, ay_font->fid);
    XMapRaised(ay_d, pencere);
    for (;;) {
        XNextEvent(ay_d, &e);
        if (e.type == Expose && e.xexpose.count == 0) ciz();
        else if (e.type == ButtonPress && e.xbutton.button == 1) {
            if (tikla(e.xbutton.x, e.xbutton.y)) break;
            ciz();
        } else if (e.type == KeyPress) {
            KeySym k = XLookupKeysym(&e.xkey, 0);
            if (k == XK_Escape) break;
            if (k == XK_Return) {
                durum_metni[0] = 0;
                if (uygula(ay_d) && !durum_metni[0]) snprintf(durum_metni, sizeof(durum_metni), "Uygulandı ve kaydedildi.");
                ciz();
            }
        } else if (ay_kapat_istegi(&e)) {
            break;
        }
    }
    XCloseDisplay(ay_d);
    return 0;
}
