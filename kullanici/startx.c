/* startx: Uppix'te grafik oturum. Xfbdev'i (:0) başlatır, X soketi kabul edene dek
 * bekler, DISPLAY=:0 ile pencere yöneticisini (jwm; ya da verilen komutu) çalıştırır.
 * Pencere yöneticisi bitince X'i durdurur; konsol metin kipine döner (X kendisi geri alır,
 * çekirdek de kipi değiştiren süreç bitince geri alır). uppsh'te döngü olmadığından
 * "soket görünene kadar bekle" bir kabuk betiği yerine bu programdır.
 *   startx                 Xfbdev + jwm
 *   startx /usr/bin/xdeneme
 * Sunucunun iletileri (stdout/stderr) /tmp/Xfbdev.0.log dosyasına gider. */
#include "linux.h"

#define SIGTERM 15
#define SIGKILL 9

static char ortam_tampon[4096];
static char* yeni_ortam[64];

static int x_hazir(void) {
    struct sockaddr_un a;
    const char* yol = "/tmp/.X11-unix/X0";
    long s = sc3(SYS_socket, AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    int i, r;
    memset(&a, 0, sizeof(a));
    a.sun_family = AF_UNIX;
    for (i = 0; yol[i]; i++) a.sun_path[1 + i] = yol[i];
    r = (int)sc3(SYS_connect, s, &a, 2 + 1 + i);
    sys_close((int)s);
    return r == 0;
}

int main(int argc, char** argv, char** envp) {
    /* TCP taşıması derlemede yok (yalnızca AF_UNIX); "-nolisten tcp" yalnızca uyarı basardı */
    char* x_arg[] = {"Xfbdev", ":0", "-dumbSched", "-noreset",
                     "-keybd", "evdev,,device=/dev/input/event0", "-mouse", "evdev,,device=/dev/input/event1", 0};
    char* wm_arg[] = {"jwm", 0};
    char** istemci = argc > 1 ? argv + 1 : wm_arg;
    const char* istemci_yol = argc > 1 ? argv[1] : "/usr/bin/jwm";
    struct timespec bekle = {0, 100000000};
    long x, w;
    int i, n = 0, o = 0, st;
    /* ortam + DISPLAY=:0 */
    for (i = 0; envp[i] && n < 60; i++) {
        if (envp[i][0] == 'D' && envp[i][1] == 'I' && envp[i][2] == 'S') continue;
        yeni_ortam[n++] = envp[i];
    }
    memcpy(ortam_tampon + o, "DISPLAY=:0", 11);
    yeni_ortam[n++] = ortam_tampon + o;
    yeni_ortam[n] = 0;
    sc2(SYS_mkdir, "/tmp/.X11-unix", 01777);
    x = sys_fork();
    if (x == 0) {
        long g = sys_open("/tmp/Xfbdev.0.log", O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (g >= 0) {
            sys_dup2((int)g, 1);
            sys_dup2((int)g, 2);
            if (g > 2) sys_close((int)g);
        }
        sys_execve("/usr/bin/Xfbdev", x_arg, yeni_ortam);
        hyaz("startx: /usr/bin/Xfbdev çalıştırılamadı\n");
        sys_exit(127);
    }
    for (i = 0; i < 300 && !x_hazir(); i++) {
        if (sys_wait4(x, &st, WNOHANG) == x) {
            hyaz("startx: X sunucusu açılamadı (ayrıntı: /tmp/Xfbdev.0.log)\n");
            return 1;
        }
        sys_nanosleep(&bekle, 0);
    }
    if (i == 300) {
        hyaz("startx: X sunucusu yanıt vermedi (ayrıntı: /tmp/Xfbdev.0.log)\n");
        sys_kill(x, SIGKILL);
        sys_wait4(x, &st, 0);
        return 1;
    }
    yaz("startx: X hazır (:0)\n");
    w = sys_fork();
    if (w == 0) {
        sys_execve(istemci_yol, istemci, yeni_ortam);
        hyaz("startx: istemci çalıştırılamadı\n");
        sys_exit(127);
    }
    sys_wait4(w, &st, 0);
    sys_kill(x, SIGTERM);
    for (i = 0; i < 50 && sys_wait4(x, &st, WNOHANG) == 0; i++) sys_nanosleep(&bekle, 0);
    if (i == 50) {
        sys_kill(x, SIGKILL);
        sys_wait4(x, &st, 0);
    }
    yaz("startx: oturum bitti\n");
    return 0;
}
