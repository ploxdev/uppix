/* ush: kullanıcı süreçleri için küçük bir /bin/sh. Uppix'te /bin/sh çekirdek içindeki
 * kabuktur (uppsh), bir ELF değildir; bir süreç /bin/sh'ı çalıştırınca (system(), popen(),
 * X sunucusunun xkbcomp'u çağırması, JWM menüsü) çekirdek onun yerine bunu çalıştırır.
 *   ush -c 'komut'      ush betik [argüman...]
 * Desteklenenler: tek/çift tırnak, ters bölü, $AD ve ${AD}, $?, ; && || &, yönlendirmeler
 * (< > >> 2> 2>> 2>&1 >&2 &>), exec, cd, exit, true, false, :, echo [-n]. Komutlar PATH'te
 * aranır. echo yerleşiktir: /bin'deki yerleşik komutlar süreç olarak çalıştırılamaz. */
#include "linux.h"

#define EN_COK_SOZ 128
#define EN_COK_YON 8

static char** ortam;
static int son_kod;
static char havuz[65536];
static int havuz_n;

static char* sakla(const char* s, int n) {
    char* p = havuz + havuz_n;
    if (havuz_n + n + 1 > (int)sizeof(havuz)) return "";
    memcpy(p, s, (size_t)n);
    p[n] = 0;
    havuz_n += n + 1;
    return p;
}

static const char* ortam_al(const char* ad, int n) {
    int i, k;
    for (i = 0; ortam && ortam[i]; i++) {
        for (k = 0; k < n && ortam[i][k] == ad[k]; k++) {
        }
        if (k == n && ortam[i][n] == '=') return ortam[i] + n + 1;
    }
    return "";
}

static void hata(const char* a, const char* b) {
    hyaz("ush: ");
    hyaz(a);
    hyaz(b);
    hyaz("\n");
}

/* Bir sözcük: tırnaklar, ters bölü ve değişkenler açılır. Dönüş: sonraki konum. */
static const char* sozcuk(const char* p, char* cikti, int* n) {
    int tirnak = 0;
    *n = 0;
    while (*p) {
        char c = *p;
        if (tirnak == 0 && (c == ' ' || c == '\t' || c == '\n' || c == ';' || c == '&' || c == '|' || c == '<' || c == '>'))
            break;
        if (c == '\'' && tirnak != 2) {
            tirnak = tirnak == 1 ? 0 : 1;
            p++;
            continue;
        }
        if (c == '"' && tirnak != 1) {
            tirnak = tirnak == 2 ? 0 : 2;
            p++;
            continue;
        }
        if (c == '\\' && tirnak != 1 && p[1]) {
            cikti[(*n)++] = p[1];
            p += 2;
            continue;
        }
        if (c == '$' && tirnak != 1 && p[1]) {
            const char* ad = p + 1;
            int an = 0, paren = 0;
            const char* v;
            if (*ad == '?') {
                char b[12];
                int i = 11, k = son_kod;
                b[i] = 0;
                do {
                    b[--i] = (char)('0' + k % 10);
                    k /= 10;
                } while (k);
                while (b[i]) cikti[(*n)++] = b[i++];
                p += 2;
                continue;
            }
            if (*ad == '{') {
                paren = 1;
                ad++;
            }
            while ((ad[an] >= 'A' && ad[an] <= 'Z') || (ad[an] >= 'a' && ad[an] <= 'z') || (ad[an] >= '0' && ad[an] <= '9') || ad[an] == '_') an++;
            if (an == 0) {
                cikti[(*n)++] = c;
                p++;
                continue;
            }
            for (v = ortam_al(ad, an); *v; v++) cikti[(*n)++] = *v;
            p = ad + an + (paren && ad[an] == '}' ? 1 : 0);
            continue;
        }
        cikti[(*n)++] = c;
        p++;
    }
    return p;
}

/* Yönlendirme: hedef fd'ye dosya (kip: 0 okuma, 1 yazma, 2 ekleme) ya da başka fd. */
struct yon {
    int fd, kip, kaynak_fd;
    char* yol;
};

static int yonlendir(struct yon* y, int n) {
    int i;
    for (i = 0; i < n; i++) {
        if (y[i].kaynak_fd >= 0) {
            sys_dup2(y[i].kaynak_fd, y[i].fd);
            continue;
        }
        {
            int bayrak = y[i].kip == 0 ? O_RDONLY : (O_WRONLY | O_CREAT | (y[i].kip == 2 ? O_APPEND : O_TRUNC));
            long f = sys_open(y[i].yol, bayrak, 0644);
            if (f < 0) {
                hata(y[i].yol, ": açılamadı");
                return -1;
            }
            if (f != y[i].fd) {
                sys_dup2((int)f, y[i].fd);
                sys_close((int)f);
            }
        }
    }
    return 0;
}

static void calistir_yol(char** soz) {
    char yol[512];
    const char* p;
    if (soz[0][0] == '/' || (soz[0][0] == '.' && soz[0][1] == '/')) {
        sys_execve(soz[0], soz, ortam);
        return;
    }
    p = ortam_al("PATH", 4);
    if (!*p) p = "/bin:/usr/bin";
    while (*p) {
        int n = 0, k = 0;
        while (p[n] && p[n] != ':') n++;
        if (n > 0 && n + 1 + (int)uzunluk(soz[0]) < (int)sizeof(yol)) {
            memcpy(yol, p, (size_t)n);
            yol[n] = '/';
            for (k = 0; soz[0][k]; k++) yol[n + 1 + k] = soz[0][k];
            yol[n + 1 + k] = 0;
            sys_execve(yol, soz, ortam);
        }
        p += n;
        if (*p == ':') p++;
    }
}

/* echo [-n] sözcük...: yönlendirmeler uygulanmış çocukta çalışır. */
static int yankila(char** soz) {
    int satir = 1;
    if (soz[0] && esit(soz[0], "-n")) {
        satir = 0;
        soz++;
    }
    for (; *soz; soz++) {
        yaz(*soz);
        if (soz[1]) yaz(" ");
    }
    if (satir) yaz("\n");
    return 0;
}

/* Basit komut: sözcükler ve yönlendirmeler. arka: & ile bitti. */
static int komut(char** soz, int n, struct yon* y, int yn, int arka) {
    long c;
    int st;
    if (n == 0) return yonlendir(y, yn) < 0 ? 1 : 0;
    soz[n] = 0;
    if (esit(soz[0], ":") || esit(soz[0], "true")) return 0;
    if (esit(soz[0], "false")) return 1;
    if (esit(soz[0], "cd")) return sc1(SYS_chdir, n > 1 ? soz[1] : ortam_al("HOME", 4)) < 0 ? 1 : 0;
    if (esit(soz[0], "exit")) {
        int k = 0;
        const char* q = n > 1 ? soz[1] : "";
        while (*q >= '0' && *q <= '9') k = k * 10 + (*q++ - '0');
        sys_exit(n > 1 ? k : son_kod);
    }
    if (esit(soz[0], "exec")) {
        if (yonlendir(y, yn) < 0) return 1;
        if (n == 1) return 0;
        calistir_yol(soz + 1);
        hata(soz[1], ": çalıştırılamadı");
        sys_exit(127);
    }
    c = sys_fork();
    if (c == 0) {
        if (yonlendir(y, yn) < 0) sys_exit(1);
        if (esit(soz[0], "echo")) sys_exit(yankila(soz + 1));
        calistir_yol(soz);
        hata(soz[0], ": komut bulunamadı");
        sys_exit(127);
    }
    if (c < 0) {
        hata(soz[0], ": fork başarısız");
        return 126;
    }
    if (arka) return 0;
    if (sys_wait4(c, &st, 0) < 0) return 1;
    if ((st & 0x7f) == 0) return (st >> 8) & 0xff;
    return 128 + (st & 0x7f);
}

static int satir_calistir(const char* p) {
    static char tampon[8192];
    char* soz[EN_COK_SOZ + 1];
    struct yon y[EN_COK_YON];
    int n = 0, yn = 0, atla = 0;
    havuz_n = 0;
    for (;;) {
        int k;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == 0 || *p == '\n' || *p == ';' || *p == '&' || *p == '|') {
            int arka = 0, sonraki = 0; /* 0 ; 1 && 2 || */
            if (p[0] == '&' && p[1] == '&') {
                sonraki = 1;
                p += 2;
            } else if (p[0] == '|' && p[1] == '|') {
                sonraki = 2;
                p += 2;
            } else if (p[0] == '&') {
                arka = 1;
                p++;
            } else if (p[0] == '|') {
                hata("boru (|) desteklenmiyor", "");
                return 2;
            } else if (*p) {
                p++;
            }
            if (!atla) son_kod = komut(soz, n, y, yn, arka);
            atla = (sonraki == 1 && son_kod != 0) || (sonraki == 2 && son_kod == 0);
            n = 0;
            yn = 0;
            havuz_n = 0;
            if (*p == 0) return son_kod;
            continue;
        }
        /* yönlendirmeler */
        {
            int fd = -1, kip = -1;
            const char* q = p;
            if (*q == '&' && q[1] == '>') {
                fd = 1;
                kip = 1;
                q += 2;
            } else {
                if (*q >= '0' && *q <= '9' && (q[1] == '>' || q[1] == '<')) {
                    fd = *q - '0';
                    q++;
                }
                if (*q == '>') {
                    if (fd < 0) fd = 1;
                    kip = 1;
                    q++;
                    if (*q == '>') {
                        kip = 2;
                        q++;
                    }
                } else if (*q == '<') {
                    if (fd < 0) fd = 0;
                    kip = 0;
                    q++;
                }
            }
            if (kip >= 0) {
                if (yn >= EN_COK_YON) return 2;
                while (*q == ' ' || *q == '\t') q++;
                y[yn].fd = fd;
                y[yn].kip = kip;
                y[yn].kaynak_fd = -1;
                y[yn].yol = 0;
                if (*q == '&' && q[1] >= '0' && q[1] <= '9') {
                    y[yn].kaynak_fd = q[1] - '0';
                    q += 2;
                } else {
                    q = sozcuk(q, tampon, &k);
                    y[yn].yol = sakla(tampon, k);
                }
                if (p[0] == '&' && p[1] == '>') {
                    y[yn + 1].fd = 2;
                    y[yn + 1].kip = 1;
                    y[yn + 1].kaynak_fd = 1;
                    y[yn + 1].yol = 0;
                    yn++;
                }
                yn++;
                p = q;
                continue;
            }
        }
        p = sozcuk(p, tampon, &k);
        if (n < EN_COK_SOZ) soz[n++] = sakla(tampon, k);
    }
}

int main(int argc, char** argv, char** envp) {
    static char betik[65536];
    ortam = envp;
    if (argc >= 3 && esit(argv[1], "-c")) return satir_calistir(argv[2]);
    if (argc >= 2) {
        /* betik dosyası: satır satır */
        long fd = sys_open(argv[1], O_RDONLY, 0), n;
        char* p;
        if (fd < 0) {
            hata(argv[1], ": açılamadı");
            return 127;
        }
        n = sys_read((int)fd, betik, sizeof(betik) - 1);
        sys_close((int)fd);
        if (n < 0) n = 0;
        betik[n] = 0;
        p = betik;
        while (*p) {
            char* s = p;
            while (*p && *p != '\n') p++;
            if (*p) *p++ = 0;
            while (*s == ' ' || *s == '\t') s++;
            if (*s && *s != '#') satir_calistir(s);
        }
        return son_kod;
    }
    hata("kullanım: ush -c KOMUT | ush BETİK", "");
    return 2;
}
