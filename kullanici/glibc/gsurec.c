/* gsurec: glibc ile statik derlenmiş (cc -static) çoklu süreç programı: fork,
 * pipe, dup2, execv, waitpid, kill. glibc fork() clone ile, pipe() pipe2 ile
 * çağırır. Çıktısı Linux'takiyle aynıdır. execv ile aynı dizindeki merhaba'yı
 * çalıştırır (dizin /proc/self/exe'den bulunur). */
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void) {
    char yol[600], tampon[4096];
    char* satir;
    int p[2], st;
    pid_t c;
    ssize_t n, r;
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("glibc süreçler başla\n");
    n = readlink("/proc/self/exe", yol, 500);
    yol[n > 0 ? n : 0] = 0;
    *(strrchr(yol, '/') ? strrchr(yol, '/') : yol) = 0;
    strcat(yol, "/merhaba");

    c = fork();
    if (c == 0) _exit(getppid() > 0 ? 9 : 1);
    printf("fork: %d\n", waitpid(c, &st, 0) == c);
    printf("çıkış: %d %d\n", WIFEXITED(st), WEXITSTATUS(st));

    if (pipe(p) != 0) return 1;
    c = fork();
    if (c == 0) {
        char* a[] = {"merhaba", "glibc", "execv", NULL};
        dup2(p[1], 1);
        close(p[0]);
        close(p[1]);
        execv(yol, a);
        _exit(127);
    }
    close(p[1]);
    n = 0;
    while ((r = read(p[0], tampon + n, sizeof(tampon) - 1 - n)) > 0) n += r;
    tampon[n] = 0;
    close(p[0]);
    /* ortam sayısı makineye göre değişir: yalnızca argv satırları */
    for (satir = strtok(tampon, "\n"); satir; satir = strtok(NULL, "\n")) {
        if (strncmp(satir, "argv", 4) == 0 || strncmp(satir, "argc", 4) == 0) printf("execv> %s\n", satir);
    }
    waitpid(c, &st, 0);
    printf("execv çıkış: %d\n", WEXITSTATUS(st));

    c = fork();
    if (c == 0) {
        for (;;) sleep(100);
    }
    printf("kill: %d\n", kill(c, SIGTERM));
    waitpid(c, &st, 0);
    printf("öldürülen: %d %d\n", WIFSIGNALED(st), WTERMSIG(st));

    /* argümanların değerlendirme sırası belirsiz: errno çağrıdan sonra okunur */
    r = waitpid(-1, &st, 0);
    printf("waitpid: %d %s\n", (int)r, strerror(errno));
    {
        char* a[] = {"yok", NULL};
        r = execv("/yok/program", a);
        printf("execv: %d %s\n", (int)r, strerror(errno));
    }
    printf("glibc süreçler bitti\n");
    return 0;
}
