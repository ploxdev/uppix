/* oturum: Uppix oturum penceresi (libX11; ui/derle.sh derler). Onay sorup oturumu kapatır
 * (jwm -exit: startx X'i durdurur, konsol metin kipine döner), bilgisayarı yeniden başlatır
 * ya da kapatır (reboot(2): çekirdek dosya sistemlerini boşaltır).
 *   oturum                       üç seçenek ve Vazgeç
 *   oturum cikis|yeniden|kapat   yalnızca o işin onayı
 * Enter ilk düğmeyi, Esc Vazgeç'i seçer. */
#include "arayuz.h"
#include <sys/reboot.h>

enum { CIKIS, YENIDEN, KAPAT, VAZGEC };

static const char* ADLAR[] = {"Oturumu kapat", "Yeniden başlat", "Kapat", "Vazgeç"};
static const char* SORULAR[] = {"Oturum kapatılsın mı? Metin konsoluna dönülür.", "Bilgisayar yeniden başlatılsın mı?",
                                "Bilgisayar kapatılsın mı?"};

static Window pencere;
static GC gc;
static AyDugme dugme[4];
static int islev[4];
static int dugme_n;
static int G = 420, Y = 132;
static const char* soru = "Ne yapmak istersiniz?";

static void ciz(void) {
    int i;
    ay_degrade(pencere, gc, 0, 0, G, 6, AY_VURGU1, AY_VURGU2);
    XSetForeground(ay_d, gc, ay_renk(AY_ZEMIN));
    XFillRectangle(ay_d, pencere, gc, 0, 6, (unsigned)G, (unsigned)(Y - 6));
    /* simge: vurgulu daire içinde güç işareti */
    XSetForeground(ay_d, gc, ay_renk(AY_VURGU2));
    XFillArc(ay_d, pencere, gc, 20, 22, 36, 36, 0, 360 * 64);
    XSetForeground(ay_d, gc, ay_renk(0xFFFFFF));
    XSetLineAttributes(ay_d, gc, 2, LineSolid, CapRound, JoinRound);
    XDrawArc(ay_d, pencere, gc, 29, 31, 18, 18, 120 * 64, 300 * 64);
    XDrawLine(ay_d, pencere, gc, 38, 28, 38, 39);
    XSetLineAttributes(ay_d, gc, 0, LineSolid, CapButt, JoinMiter);
    ay_metin(pencere, gc, 72, 26, "Uppix oturumu", AY_VURGU2);
    ay_metin(pencere, gc, 72, 44, soru, AY_YAZI);
    XSetForeground(ay_d, gc, ay_renk(AY_CIZGI));
    XDrawLine(ay_d, pencere, gc, 0, Y - 50, G, Y - 50);
    for (i = 0; i < dugme_n; i++) ay_dugme_ciz(pencere, gc, &dugme[i]);
}

static void yap(int is) {
    char* jwm[] = {"jwm", "-exit", NULL};
    if (is == VAZGEC) return;
    printf("oturum: %s\n", is == CIKIS ? "cikis" : is == YENIDEN ? "yeniden" : "kapat");
    XUnmapWindow(ay_d, pencere);
    XSync(ay_d, False);
    if (is == CIKIS) {
        ay_calistir("/usr/bin/jwm", jwm);
        return;
    }
    sync();
    if (reboot(is == YENIDEN ? RB_AUTOBOOT : RB_POWER_OFF) < 0) perror("oturum: reboot");
}

int main(int argc, char** argv) {
    XEvent e;
    int i, basili = -1, x;
    setvbuf(stdout, NULL, _IONBF, 0);
    if (!ay_ac()) {
        fprintf(stderr, "oturum: ekran açılamadı (DISPLAY?)\n");
        return 1;
    }
    if (argc > 1) {
        int is = !strcmp(argv[1], "cikis") ? CIKIS : !strcmp(argv[1], "yeniden") ? YENIDEN : !strcmp(argv[1], "kapat") ? KAPAT : -1;
        if (is < 0) {
            fprintf(stderr, "kullanım: oturum [cikis|yeniden|kapat]\n");
            return 2;
        }
        soru = SORULAR[is];
        islev[0] = is;
        islev[1] = VAZGEC;
        dugme_n = 2;
    } else {
        islev[0] = CIKIS;
        islev[1] = YENIDEN;
        islev[2] = KAPAT;
        islev[3] = VAZGEC;
        dugme_n = 4;
        G = 500;
    }
    /* düğmeler sağa yaslı; ilki vurgulu (Enter) */
    x = G - 14;
    for (i = dugme_n - 1; i >= 0; i--) {
        int g = ay_metin_gen(ADLAR[islev[i]]) + 28;
        if (g < 84) g = 84;
        x -= g;
        dugme[i].x = x;
        dugme[i].y = Y - 38;
        dugme[i].g = g;
        dugme[i].h = 28;
        dugme[i].ad = ADLAR[islev[i]];
        dugme[i].secili = i == 0;
        x -= 8;
    }
    pencere = ay_pencere("Oturum", "oturum", G, Y, AY_ZEMIN);
    gc = XCreateGC(ay_d, pencere, 0, NULL);
    if (ay_font) XSetFont(ay_d, gc, ay_font->fid);
    XMapRaised(ay_d, pencere);
    for (;;) {
        XNextEvent(ay_d, &e);
        if (e.type == MapNotify) {
            /* pencere yöneticisi yoksa (startx /usr/bin/oturum) odak kendiliğinden gelmez */
            XSetInputFocus(ay_d, pencere, RevertToParent, CurrentTime);
            XSync(ay_d, False);
            printf("oturum: soruldu (%s)\n", argc > 1 ? argv[1] : "hepsi");
        } else if (e.type == Expose && e.xexpose.count == 0) {
            ciz();
        } else if (e.type == ButtonPress && e.xbutton.button == 1) {
            for (i = 0; i < dugme_n; i++)
                if (ay_icinde(&dugme[i], e.xbutton.x, e.xbutton.y)) {
                    basili = i;
                    dugme[i].basili = 1;
                    ay_dugme_ciz(pencere, gc, &dugme[i]);
                }
        } else if (e.type == ButtonRelease && e.xbutton.button == 1 && basili >= 0) {
            dugme[basili].basili = 0;
            ay_dugme_ciz(pencere, gc, &dugme[basili]);
            if (ay_icinde(&dugme[basili], e.xbutton.x, e.xbutton.y)) {
                yap(islev[basili]);
                break;
            }
            basili = -1;
        } else if (e.type == KeyPress) {
            KeySym k = XLookupKeysym(&e.xkey, 0);
            if (k == XK_Escape) break;
            if (k == XK_Return || k == XK_KP_Enter) {
                yap(islev[0]);
                break;
            }
        } else if (ay_kapat_istegi(&e)) {
            break;
        }
    }
    XCloseDisplay(ay_d);
    return 0;
}
