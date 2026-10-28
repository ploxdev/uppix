/* sctest3: süreç sistem çağrılarını sınar: fork, vfork, clone, wait4, waitid, kill,
 * pipe, pipe2, dup2 ve execve (ELF, #! betiği, O_CLOEXEC, ortam, hata kodları).
 * sctest/sctest2 gibi çıktısı ortamdan bağımsızdır (PID ya da adres yazılmaz):
 * gerçek Linux'taki ve Uppix'teki çıktı satır satır aynı olmalıdır. Çalışma
 * dizininde sct3_d dizinini kurar ve sonunda siler. execve ile kendisini ve aynı
 * dizindeki merhaba'yı çalıştırır (dizin /proc/self/exe'den bulunur). */
#include "linux.h"

#define D "sct3_d"
#define F_GETFD 1
#define SIGKILL 9
#define SIGPIPE 13
#define SIGTERM 15

static char dizin[512];
static char kendi[600];
static char merhaba_yol[600];
static char tampon[4096];
static char buyuk[200001];
static char betik[700];

static void kopyala(char* d, const char* a, const char* b) {
    while (*a) *d++ = *a++;
    while (*b) *d++ = *b++;
    *d = 0;
}

static void sayi_metin(char* b, long v) {
    char t[24];
    int i = 0, j = 0;
    do {
        t[i++] = (char)('0' + v % 10);
        v /= 10;
    } while (v);
    while (i) b[j++] = t[--i];
    b[j] = 0;
}

static long sayi_oku(const char* s) {
    long v = 0;
    while (*s >= '0' && *s <= '9') v = v * 10 + (*s++ - '0');
    return v;
}

/* wait durumu: "çıktı K" ya da "sinyal S" */
static void durum(const char* ad, int st) {
    yaz(ad);
    yaz(": ");
    if ((st & 0x7f) == 0) {
        yaz("çıktı ");
        sayi_yaz((st >> 8) & 0xff);
    } else {
        yaz("sinyal ");
        sayi_yaz(st & 0x7f);
    }
    yaz("\n");
}

static void uyu_sonsuz(void) {
    struct timespec t = {100, 0};
    for (;;) sys_nanosleep(&t, 0);
}

static void dosya_yaz(const char* yol, const char* icerik, int kip) {
    long fd = sys_open(yol, O_CREAT | O_WRONLY | O_TRUNC, kip);
    sys_write((int)fd, icerik, uzunluk(icerik));
    sys_close((int)fd);
}

static void sinyal_eylem(int sig, long isleyici) {
    long e[4] = {isleyici, 0, 0, 0};
    sc6(SYS_rt_sigaction, sig, (long)e, 0, 8, 0, 0);
}

/* execve sonrası (O_CLOEXEC sınaması): argv[2] kapanmış, argv[3] açık olmalı */
static int kip_fd(char** argv) {
    yaz("exec sonrası: cloexec ");
    sayi_yaz(sc2(SYS_fcntl, sayi_oku(argv[2]), F_GETFD));
    yaz(" dup ");
    sayi_yaz(sc2(SYS_fcntl, sayi_oku(argv[3]), F_GETFD));
    yaz("\n");
    return 0;
}

int main(int argc, char** argv, char** envp) {
    int st, p[2], q[2], i;
    long c, r, n, k, ana;
    struct stat sb;
    char a1[24], a2[24];

    if (argc >= 4 && esit(argv[1], "--fd")) return kip_fd(argv);
    if (argc >= 2 && esit(argv[1], "--betik")) {
        /* argv[0] yorumlayıcının mutlak yoludur (makineye göre değişir) */
        satir("betik argc", argc);
        for (i = 1; i < argc; i++) {
            yaz("betik argv: ");
            yaz(argv[i]);
            yaz("\n");
        }
        return 3;
    }
    if (argc >= 2 && esit(argv[1], "--ortam")) {
        n = 0;
        while (envp[n]) n++;
        satir("ortam sayısı", n);
        for (i = 0; i < n; i++) {
            yaz("ortam: ");
            yaz(envp[i]);
            yaz("\n");
        }
        return 4;
    }

    yaz("=== sctest3 başla\n");
    n = sc3(SYS_readlink, "/proc/self/exe", dizin, sizeof(dizin) - 1);
    if (n <= 0) n = 1;
    dizin[n] = 0;
    for (i = (int)n - 1; i > 0 && dizin[i] != '/'; i--) {
    }
    dizin[i] = 0;
    kopyala(kendi, dizin, "/sctest3");
    kopyala(merhaba_yol, dizin, "/merhaba");
    sc2(SYS_mkdir, D, 0755);

    /* fork, getppid, çıkış kodu */
    ana = sc0(SYS_getpid);
    c = sys_fork();
    if (c == 0) sys_exit(sc0(SYS_getppid) == ana ? 0 : 1);
    satir("fork pozitif", c > 0);
    satir("wait4 pid", sys_wait4(c, &st, 0) == c);
    durum("getppid ebeveyn mi", st);
    c = sys_fork();
    if (c == 0) sys_exit(42);
    sys_wait4(c, &st, 0);
    durum("çıkış kodu", st);
    k = sc0(SYS_getpgrp);
    c = sys_fork();
    if (c == 0) sys_exit(sc0(SYS_getpgrp) == k ? 0 : 1);
    sys_wait4(c, &st, 0);
    durum("getpgrp ebeveyninki", st);

    /* WNOHANG, kill, sinyal durumu, ESRCH */
    c = sys_fork();
    if (c == 0) uyu_sonsuz();
    satir("wait4 WNOHANG", sys_wait4(c, &st, WNOHANG));
    satir("kill 0", sys_kill(c, 0));
    satir("kill SIGTERM", sys_kill(c, SIGTERM));
    satir("wait4 öldürülen", sys_wait4(c, &st, 0) == c);
    durum("SIGTERM", st);
    satir("kill toplanmış", sys_kill(c, 0));
    c = sys_fork();
    if (c == 0) {
        sys_kill(sc0(SYS_getpid), SIGKILL);
        sys_exit(1);
    }
    sys_wait4(c, &st, 0);
    durum("kendine SIGKILL", st);

    /* ECHILD, grup (pid 0), herhangi biri (-1) */
    satir("wait4 çocuk yok", sys_wait4(-1, &st, 0));
    satir("wait4 çocuk yok WNOHANG", sys_wait4(-1, &st, WNOHANG));
    satir("wait4 geçersiz seçenek", sys_wait4(-1, &st, 0x100));
    c = sys_fork();
    if (c == 0) sys_exit(3);
    satir("wait4 grup", sys_wait4(0, &st, 0) == c);
    durum("grup", st);
    c = sys_fork();
    if (c == 0) sys_exit(4);
    satir("wait4 herhangi", sys_wait4(-1, &st, 0) == c);
    durum("herhangi", st);

    /* waitid */
    {
        int bilgi[32];
        c = sys_fork();
        if (c == 0) sys_exit(5);
        for (i = 0; i < 32; i++) bilgi[i] = -1;
        satir("waitid", sc6(SYS_waitid, P_PID, c, (long)bilgi, WEXITED, 0, 0));
        satir("waitid si_signo", bilgi[0]);
        satir("waitid si_code", bilgi[2]);
        satir("waitid si_pid", bilgi[4] == c);
        satir("waitid si_status", bilgi[6]);
        c = sys_fork();
        if (c == 0) uyu_sonsuz();
        for (i = 0; i < 32; i++) bilgi[i] = -1;
        satir("waitid WNOHANG", sc6(SYS_waitid, P_PID, c, (long)bilgi, WEXITED | WNOHANG, 0, 0));
        satir("waitid WNOHANG si_pid", bilgi[4]);
        sys_kill(c, SIGTERM);
        for (i = 0; i < 32; i++) bilgi[i] = -1;
        satir("waitid P_ALL", sc6(SYS_waitid, P_ALL, 0, (long)bilgi, WEXITED, 0, 0));
        satir("waitid öldürülen si_code", bilgi[2]);
        satir("waitid öldürülen si_status", bilgi[6]);
        satir("waitid seçeneksiz", sc6(SYS_waitid, P_ALL, 0, (long)bilgi, 0, 0, 0));
    }

    /* vfork ve fork'a denk clone */
    c = sc0(SYS_vfork);
    if (c == 0) sys_exit(6);
    sys_wait4(c, &st, 0);
    durum("vfork", st);
    c = sc6(SYS_clone, SIGCHLD, 0, 0, 0, 0, 0);
    if (c == 0) sys_exit(8);
    sys_wait4(c, &st, 0);
    durum("clone", st);

    /* pipe: çocuk yazar, ebeveyn EOF'a dek okur */
    satir("pipe", sys_pipe(p));
    c = sys_fork();
    if (c == 0) {
        sys_close(p[0]);
        sys_write(p[1], "boru verisi: ", 13);
        sys_write(p[1], "çocuktan\n", uzunluk("çocuktan\n"));
        sys_exit(0);
    }
    sys_close(p[1]);
    n = 0;
    while ((r = sys_read(p[0], tampon + n, sizeof(tampon) - (size_t)n)) > 0) n += r;
    yaz("okunan: ");
    sys_write(1, tampon, (size_t)n);
    satir("boru EOF", r);
    sys_close(p[0]);
    sys_wait4(c, &st, 0);
    durum("boru yazıcı", st);

    /* 64 KiB'tan büyük aktarım: dolu boruda yazıcı bekler */
    sys_pipe(p);
    c = sys_fork();
    if (c == 0) {
        long toplam = 0, adet = 0;
        sys_close(p[1]);
        while ((r = sys_read(p[0], tampon, sizeof(tampon))) > 0) {
            for (i = 0; i < r; i++) toplam += (unsigned char)tampon[i];
            adet += r;
        }
        sys_exit(adet == 100000 ? (int)(toplam % 200) : 255);
    }
    sys_close(p[0]);
    {
        long beklenen = 0;
        for (i = 0; i < 100000; i++) {
            buyuk[i] = (char)(i * 7 % 251);
            beklenen += (unsigned char)buyuk[i];
        }
        n = 0;
        while (n < 100000) {
            r = sys_write(p[1], buyuk + n, 100000 - (size_t)n);
            if (r <= 0) break;
            n += r;
        }
        satir("büyük yazılan", n);
        sys_close(p[1]);
        sys_wait4(c, &st, 0);
        satir("büyük aktarım doğru", ((st >> 8) & 0xff) == beklenen % 200);
    }

    /* okuyucusuz boru: SIGPIPE; yok sayılırsa EPIPE */
    sys_pipe(q);
    sys_close(q[0]);
    c = sys_fork();
    if (c == 0) {
        sys_write(q[1], "x", 1);
        sys_exit(1);
    }
    sys_wait4(c, &st, 0);
    durum("okuyucusuz yazma", st);
    sinyal_eylem(SIGPIPE, 1);
    satir("EPIPE", sys_write(q[1], "x", 1));
    sinyal_eylem(SIGPIPE, 0);
    sys_close(q[1]);

    /* boru tanımlayıcısının özellikleri */
    sys_pipe(p);
    sys_fstat(p[0], &sb);
    satir("fstat FIFO", (sb.st_mode & S_IFMT) == S_IFIFO);
    satir("lseek boru", sys_lseek(p[0], 0, SEEK_SET));
    satir("read yazma ucu", sys_read(p[1], tampon, 1));
    satir("write okuma ucu", sys_write(p[0], "x", 1));
    satir("dup2 boru", sys_dup2(p[1], 9));
    sys_close(p[1]);
    satir("dup ile yazan açık", sys_write(9, "ab", 2));
    sys_close(9);
    satir("kalan veri", sys_read(p[0], tampon, sizeof(tampon)));
    satir("boş boru EOF", sys_read(p[0], tampon, sizeof(tampon)));
    sys_close(p[0]);
    satir("pipe2 geçersiz bayrak", sys_pipe2(p, 1));
    satir("pipe2 O_CLOEXEC", sys_pipe2(p, O_CLOEXEC | O_NONBLOCK));
    satir("F_GETFD", sc2(SYS_fcntl, p[0], F_GETFD));
    satir("O_NONBLOCK okuma", sys_read(p[0], tampon, 1));
    sys_close(p[0]);
    sys_close(p[1]);
    satir("pipe EFAULT", sys_pipe((int*)1));

    /* dup2 ile stdout boruya, execve ile merhaba */
    sys_pipe(p);
    c = sys_fork();
    if (c == 0) {
        char* a[] = {"merhaba", "boru", "ile", 0};
        char* e[] = {"A=1", "B=2", 0};
        sys_dup2(p[1], 1);
        sys_close(p[0]);
        sys_close(p[1]);
        sys_execve(merhaba_yol, a, e);
        sys_exit(127);
    }
    sys_close(p[1]);
    n = 0;
    while ((r = sys_read(p[0], tampon + n, sizeof(tampon) - 1 - (size_t)n)) > 0) n += r;
    sys_close(p[0]);
    tampon[n] = 0;
    for (k = 0, i = 0; i < n; i++) {
        if (i == k) yaz("exec> ");
        if (tampon[i] == '\n') {
            sys_write(1, tampon + k, (size_t)(i - k + 1));
            k = i + 1;
        }
    }
    sys_wait4(c, &st, 0);
    durum("merhaba", st);

    /* O_CLOEXEC execve'de kapanır, dup'lanan kopya açık kalır */
    c = sys_fork();
    if (c == 0) {
        char* a[] = {"sctest3", "--fd", a1, a2, 0};
        sys_pipe2(p, O_CLOEXEC);
        sayi_metin(a1, p[0]);
        sayi_metin(a2, sc1(SYS_dup, p[0]));
        sys_execve(kendi, a, envp);
        sys_exit(127);
    }
    sys_wait4(c, &st, 0);
    durum("cloexec", st);

    /* #! betiği: yorumlayıcı + tek argüman + betiğin yolu + kalan argümanlar */
    kopyala(betik, "#!", kendi);
    kopyala(betik + uzunluk(betik), " --betik\n", "");
    dosya_yaz(D "/betik", betik, 0755);
    c = sys_fork();
    if (c == 0) {
        char* a[] = {"betik", "x", "y z", 0};
        sys_execve(D "/betik", a, envp);
        sys_exit(127);
    }
    sys_wait4(c, &st, 0);
    durum("betik", st);

    /* yeni ortam */
    c = sys_fork();
    if (c == 0) {
        char* a[] = {"sctest3", "--ortam", 0};
        char* e[] = {"X=1", "Y=iki", 0};
        sys_execve(kendi, a, e);
        sys_exit(127);
    }
    sys_wait4(c, &st, 0);
    durum("ortam", st);

    /* execve hataları (mevcut görüntü sürer) */
    {
        char* a[] = {"x", 0};
        char* b[] = {"x", buyuk, 0};
        for (i = 0; i < 200000; i++) buyuk[i] = 'a';
        buyuk[200000] = 0;
        dosya_yaz(D "/izinsiz", "#!/bin/sh\n", 0644);
        dosya_yaz(D "/bozuk", "bu bir program değil\n", 0755);
        satir("execve ENOENT", sys_execve(D "/yok", a, envp));
        satir("execve EACCES", sys_execve(D "/izinsiz", a, envp));
        satir("execve dizin", sys_execve(D, a, envp));
        satir("execve ENOEXEC", sys_execve(D "/bozuk", a, envp));
        satir("execve EFAULT argv", sys_execve(kendi, (char**)1, envp));
        satir("execve EFAULT yol", sys_execve((char*)1, a, envp));
        satir("execve E2BIG", sys_execve(kendi, b, envp));
    }

    /* oturum ve süreç grubu (çocukta: çağıran grup lideri değildir) */
    c = sys_fork();
    if (c == 0) {
        long ben = sc0(SYS_getpid);
        satir("setsid", sc0(SYS_setsid) == ben);
        satir("setsid lider", sc0(SYS_setsid));
        satir("getsid", sc1(SYS_getsid, 0) == ben);
        satir("getpgid", sc1(SYS_getpgid, 0) == ben);
        sys_exit(0);
    }
    sys_wait4(c, &st, 0);
    durum("setsid çocuğu", st);
    c = sys_fork();
    if (c == 0) {
        long ben = sc0(SYS_getpid);
        satir("setpgid", sc2(SYS_setpgid, 0, 0));
        satir("getpgrp yeni", sc0(SYS_getpgrp) == ben);
        satir("setpgid negatif", sc2(SYS_setpgid, 0, -1));
        satir("setpgid çocuk değil", sc2(SYS_setpgid, 1, 0));
        sys_exit(0);
    }
    sys_wait4(c, &st, 0);
    durum("setpgid çocuğu", st);

    /* kimlik: yalnızca kendi kimliğine kurmak (Linux'taki karşılaştırma root olmadan koşar) */
    {
        unsigned int k[3];
        long u = sc0(SYS_getuid);
        satir("setuid kendisi", sc1(SYS_setuid, u));
        satir("setresuid değişmeden", sc3(SYS_setresuid, -1, -1, -1));
        satir("getresuid", sc3(SYS_getresuid, k, k + 1, k + 2));
        satir("getresuid aynı", k[0] == u && k[1] == u && k[2] == u);
        satir("getresuid EFAULT", sc3(SYS_getresuid, 1, k + 1, k + 2));
    }

    sc1(SYS_unlink, D "/betik");
    sc1(SYS_unlink, D "/izinsiz");
    sc1(SYS_unlink, D "/bozuk");
    satir("rmdir", sc1(SYS_rmdir, D));
    satir("artık çocuk yok", sys_wait4(-1, &st, WNOHANG));
    yaz("=== sctest3 bitti ===\n");
    return 0;
}
