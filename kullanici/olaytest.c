/* olaytest: Linux evdev aygıtlarını sınar (/dev/input/event0 klavye, event1 fare):
 * ioctl'lar (sürüm, kimlik, ad, desteklenen olay bitleri, EVIOCGRAB), okuma hataları
 * (kısa tampon EINVAL, boş kuyrukta EAGAIN), konsol kipleri (KDGETMODE, KDGKBMODE,
 * VT_GETSTATE) ve gelen olayların tür/kod/değer dizisi. "hazır" satırından sonra
 * gelen olayları A tuşu bırakılana dek (en çok 10 sn) toplar, sonra aygıt aygıt yazar. */
#include "linux.h"

#define AZAMI 64

static int olay_n[2];
static struct input_event olaylar[2][AZAMI];

static void onaltilik(unsigned long v) {
    const char* h = "0123456789abcdef";
    char b[20];
    int i = 0;
    do {
        b[i++] = h[v % 16];
        v /= 16;
    } while (v);
    yaz("0x");
    while (i) sys_write(1, &b[--i], 1);
}

static int bit_var(const unsigned char* b, int n) { return (b[n / 8] >> (n % 8)) & 1; }

static void aygit_bilgi(const char* ad, int fd) {
    int surum = 0;
    struct input_id id;
    char isim[64];
    unsigned char bit[96];
    long n;
    yaz(ad);
    yaz(": sürüm ");
    sys_ioctl(fd, EVIOCGVERSION, &surum);
    onaltilik((unsigned long)surum);
    sys_ioctl(fd, EVIOCGID, &id);
    yaz(" kimlik ");
    onaltilik(id.bustype);
    yaz(":");
    onaltilik(id.vendor);
    yaz(":");
    onaltilik(id.product);
    yaz(":");
    onaltilik(id.version);
    n = sys_ioctl(fd, EVIOCGNAME(sizeof(isim)), isim);
    yaz(" ad \"");
    if (n > 0) yaz(isim);
    yaz("\" (");
    sayi_yaz(n);
    yaz(")\n");
    memset(bit, 0, sizeof(bit));
    n = sys_ioctl(fd, EVIOCGBIT(0, sizeof(bit)), bit);
    yaz(ad);
    yaz(": olay türleri ");
    onaltilik(bit[0]);
    yaz(" (");
    sayi_yaz(n);
    yaz(")");
    memset(bit, 0, sizeof(bit));
    n = sys_ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(bit)), bit);
    yaz(" EV_KEY ");
    sayi_yaz(n);
    yaz(" KEY_A ");
    sayi_yaz(bit_var(bit, KEY_A));
    yaz(" KEY_Q ");
    sayi_yaz(bit_var(bit, KEY_Q));
    yaz(" BTN_LEFT ");
    sayi_yaz(bit_var(bit, BTN_LEFT));
    memset(bit, 0, sizeof(bit));
    n = sys_ioctl(fd, EVIOCGBIT(EV_REL, sizeof(bit)), bit);
    yaz(" EV_REL ");
    sayi_yaz(n);
    yaz(" REL_X ");
    sayi_yaz(bit_var(bit, REL_X));
    yaz(" REL_WHEEL ");
    sayi_yaz(bit_var(bit, REL_WHEEL));
    yaz("\n");
}

static void olaylari_yaz(const char* ad, int d) {
    int i;
    for (i = 0; i < olay_n[d]; i++) {
        yaz(ad);
        yaz(": tür ");
        sayi_yaz(olaylar[d][i].type);
        yaz(" kod ");
        sayi_yaz(olaylar[d][i].code);
        yaz(" değer ");
        sayi_yaz(olaylar[d][i].value);
        yaz("\n");
    }
}

int main(int argc, char** argv, char** envp) {
    long kb, ms, ms2, tty, r;
    int kip = -1, i, d, bitti = 0, tur;
    unsigned short vt[3];
    struct input_event ev[8];
    struct timespec bekle = {0, 5000000};
    (void)envp;
    if (argc > 1 && esit(argv[1], "--kip")) {
        /* yalnızca konsol kipleri */
        tty = sys_open("/dev/tty0", O_RDWR, 0);
        sys_ioctl((int)tty, KDGETMODE, &kip);
        yaz("konsol: KDGETMODE ");
        sayi_yaz(kip);
        sys_ioctl((int)tty, KDGKBMODE, &kip);
        yaz(" KDGKBMODE ");
        sayi_yaz(kip);
        yaz("\n");
        return 0;
    }
    yaz("=== olaytest başla\n");
    kb = sys_open("/dev/input/event0", O_RDONLY | O_NONBLOCK, 0);
    ms = sys_open("/dev/input/event1", O_RDONLY | O_NONBLOCK, 0);
    ms2 = sys_open("/dev/input/event1", O_RDONLY | O_NONBLOCK, 0);
    tty = sys_open("/dev/tty0", O_RDWR, 0);
    satir("açılış", kb >= 0 && ms >= 0 && ms2 >= 0 && tty >= 0);
    aygit_bilgi("klavye", (int)kb);
    aygit_bilgi("fare", (int)ms);
    satir("kısa tampon", sys_read((int)ms, ev, 10));
    satir("boş kuyruk", sys_read((int)ms, ev, sizeof(ev)));
    satir("EVIOCGRAB", sys_ioctl_deger((int)ms2, EVIOCGRAB, 1));
    satir("EVIOCGRAB ikinci", sys_ioctl_deger((int)ms, EVIOCGRAB, 1));
    satir("EVIOCGRAB bırak", sys_ioctl_deger((int)ms2, EVIOCGRAB, 0));
    satir("EVIOCGRAB sahibi değil", sys_ioctl_deger((int)ms2, EVIOCGRAB, 0));
    sys_close((int)ms2);
    sys_ioctl((int)tty, KDGETMODE, &kip);
    satir("KDGETMODE", kip);
    sys_ioctl((int)tty, KDGKBMODE, &kip);
    satir("KDGKBMODE", kip);
    satir("KDSETMODE geçersiz", sys_ioctl_deger((int)tty, KDSETMODE, 7));
    sys_ioctl((int)tty, VT_GETSTATE, vt);
    satir("VT etkin", vt[0]);
    satir("bilinmeyen ioctl", sys_ioctl_deger((int)kb, 0x1234, 0));
    yaz("olaytest: hazır\n");
    for (i = 0; i < 2000 && !bitti; i++) {
        for (d = 0; d < 2; d++) {
            r = sys_read((int)(d ? ms : kb), ev, sizeof(ev));
            for (tur = 0; r > 0 && tur < (int)(r / (long)sizeof(ev[0])); tur++) {
                if (olay_n[d] < AZAMI) olaylar[d][olay_n[d]++] = ev[tur];
                if (d == 0 && ev[tur].type == EV_KEY && ev[tur].code == KEY_A && ev[tur].value == 0) bitti = 1;
            }
        }
        if (!bitti) sys_nanosleep(&bekle, 0);
    }
    /* A bırakıldıktan sonraki SYN de gelsin */
    r = sys_read((int)kb, ev, sizeof(ev));
    for (tur = 0; r > 0 && tur < (int)(r / (long)sizeof(ev[0])); tur++) {
        if (olay_n[0] < AZAMI) olaylar[0][olay_n[0]++] = ev[tur];
    }
    olaylari_yaz("fare", 1);
    olaylari_yaz("klavye", 0);
    yaz("=== olaytest bitti ===\n");
    return 0;
}
