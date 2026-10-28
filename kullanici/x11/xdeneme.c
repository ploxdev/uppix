/* xdeneme: libX11 test istemcisi (ui/derle.sh musl ile statik derler). Ekranın ortasında
 * pencere yöneticisinin dokunmadığı (override-redirect) yeşil bir pencere açar, MapNotify'ı
 * bekler, içine kırmızı bir dikdörtgen çizer, girdi odağını alır ve "hazır" yazar. Bir tuş
 * ve bir fare tuşu olayını yazıp çıkar. Çıktısı deterministiktir (koordinat yazılmaz). */
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>

static unsigned long renk(Visual* v, unsigned r, unsigned g, unsigned b) {
    unsigned long p = 0;
    unsigned long m;
    int k;
    /* TrueColor: her bileşen kendi maskesinin en anlamlı bitlerine */
    for (m = v->red_mask, k = 0; m && !(m & 1); m >>= 1) k++;
    p |= ((unsigned long)r * m / 255) << k;
    for (m = v->green_mask, k = 0; m && !(m & 1); m >>= 1) k++;
    p |= ((unsigned long)g * m / 255) << k;
    for (m = v->blue_mask, k = 0; m && !(m & 1); m >>= 1) k++;
    p |= ((unsigned long)b * m / 255) << k;
    return p;
}

int main(void) {
    Display* d;
    Window w;
    GC gc;
    XEvent e;
    XSetWindowAttributes ozellik;
    Visual* v;
    int ekran, gen, yuk, tus = 0, dugme = 0;
    setvbuf(stdout, NULL, _IONBF, 0);
    d = XOpenDisplay(NULL);
    if (!d) {
        printf("xdeneme: ekran açılamadı\n");
        return 1;
    }
    ekran = DefaultScreen(d);
    v = DefaultVisual(d, ekran);
    gen = DisplayWidth(d, ekran);
    yuk = DisplayHeight(d, ekran);
    printf("xdeneme: ekran %dx%d derinlik %d\n", gen, yuk, DefaultDepth(d, ekran));
    ozellik.override_redirect = True;
    ozellik.background_pixel = renk(v, 0, 160, 0);
    ozellik.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask | ButtonPressMask;
    w = XCreateWindow(d, RootWindow(d, ekran), gen / 2 - 200, yuk / 2 - 150, 400, 300, 0, CopyFromParent,
                      InputOutput, CopyFromParent, CWOverrideRedirect | CWBackPixel | CWEventMask, &ozellik);
    XMapRaised(d, w);
    do {
        XNextEvent(d, &e);
    } while (e.type != MapNotify);
    printf("xdeneme: pencere eşlendi\n");
    gc = XCreateGC(d, w, 0, NULL);
    XSetForeground(d, gc, renk(v, 220, 0, 0));
    XFillRectangle(d, w, gc, 50, 50, 100, 80);
    XSetInputFocus(d, w, RevertToParent, CurrentTime);
    XSync(d, False);
    printf("xdeneme: hazır\n");
    while (!tus || !dugme) {
        XNextEvent(d, &e);
        if (e.type == Expose) {
            XFillRectangle(d, w, gc, 50, 50, 100, 80);
        } else if (e.type == KeyPress && !tus) {
            char ad[16];
            KeySym k = NoSymbol;
            XLookupString(&e.xkey, ad, sizeof(ad), &k, NULL);
            printf("xdeneme: tuş %s\n", k != NoSymbol ? XKeysymToString(k) : "?");
            tus = 1;
        } else if (e.type == ButtonPress && !dugme) {
            printf("xdeneme: fare tuşu %u\n", e.xbutton.button);
            dugme = 1;
        }
    }
    XFreeGC(d, gc);
    XDestroyWindow(d, w);
    XCloseDisplay(d);
    printf("xdeneme: bitti\n");
    return 0;
}
