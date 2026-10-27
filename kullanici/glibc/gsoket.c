/* gsoket: glibc ile statik derlenmiş (cc -static) AF_UNIX istemci-sunucu: socket, bind
 * (soyut ad), listen, fork, connect, poll, accept4, epoll ve socketpair üzerinden
 * sendmsg/recvmsg ile tanımlayıcı geçirme. Çıktısı Linux'takiyle aynıdır. */
#define _GNU_SOURCE
#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

static socklen_t soyut(struct sockaddr_un* a, const char* ad) {
    memset(a, 0, sizeof(*a));
    a->sun_family = AF_UNIX;
    strcpy(a->sun_path + 1, ad);
    return (socklen_t)(sizeof(a->sun_family) + 1 + strlen(ad));
}

int main(void) {
    struct sockaddr_un a;
    struct pollfd pf;
    struct epoll_event e, ev;
    char tampon[256];
    int srv, k, ep, st, sv[2], p[2];
    ssize_t n;
    pid_t c;
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("glibc soket başla\n");
    srv = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    printf("bind: %d\n", bind(srv, (struct sockaddr*)&a, soyut(&a, "gsoket-sunucu")));
    printf("listen: %d\n", listen(srv, 4));
    c = fork();
    if (c == 0) {
        int s = socket(AF_UNIX, SOCK_STREAM, 0);
        if (connect(s, (struct sockaddr*)&a, soyut(&a, "gsoket-sunucu")) != 0) _exit(1);
        send(s, "glibc istemci", 13, 0);
        n = recv(s, tampon, sizeof(tampon), 0);
        _exit(n == 2 ? 5 : 2);
    }
    pf.fd = srv;
    pf.events = POLLIN;
    /* argümanların değerlendirme sırası belirsiz: sonuç önce alınır */
    n = poll(&pf, 1, 5000);
    printf("poll: %d revents=%d\n", (int)n, pf.revents);
    k = accept4(srv, NULL, NULL, SOCK_NONBLOCK);
    ep = epoll_create1(0);
    e.events = EPOLLIN;
    e.data.u64 = 99;
    epoll_ctl(ep, EPOLL_CTL_ADD, k, &e);
    n = epoll_wait(ep, &ev, 1, 5000);
    printf("epoll_wait: %d veri=%llu\n", (int)n, (unsigned long long)ev.data.u64);
    n = recv(k, tampon, sizeof(tampon) - 1, 0);
    tampon[n > 0 ? n : 0] = 0;
    printf("alındı: %s\n", tampon);
    send(k, "ok", 2, MSG_NOSIGNAL);
    waitpid(c, &st, 0);
    printf("istemci çıkış: %d\n", WEXITSTATUS(st));
    n = recv(k, tampon, sizeof(tampon), 0);
    printf("EOF: %d\n", (int)n);
    close(k);
    close(ep);
    close(srv);

    /* SCM_RIGHTS */
    socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
    if (pipe(p) != 0) return 1;
    {
        struct msghdr m;
        struct iovec v;
        union {
            char b[CMSG_SPACE(sizeof(int))];
            struct cmsghdr h;
        } kontrol;
        struct cmsghdr* cm;
        int gelen;
        memset(&m, 0, sizeof(m));
        v.iov_base = "f";
        v.iov_len = 1;
        m.msg_iov = &v;
        m.msg_iovlen = 1;
        m.msg_control = kontrol.b;
        m.msg_controllen = sizeof(kontrol.b);
        cm = CMSG_FIRSTHDR(&m);
        cm->cmsg_level = SOL_SOCKET;
        cm->cmsg_type = SCM_RIGHTS;
        cm->cmsg_len = CMSG_LEN(sizeof(int));
        memcpy(CMSG_DATA(cm), &p[1], sizeof(int));
        printf("sendmsg: %d\n", (int)sendmsg(sv[0], &m, 0));
        close(p[1]);
        memset(&kontrol, 0, sizeof(kontrol));
        v.iov_base = tampon;
        v.iov_len = sizeof(tampon);
        m.msg_controllen = sizeof(kontrol.b);
        printf("recvmsg: %d\n", (int)recvmsg(sv[1], &m, 0));
        cm = CMSG_FIRSTHDR(&m);
        printf("cmsg: %d %d %d\n", cm != NULL, cm ? cm->cmsg_level : -1, cm ? cm->cmsg_type : -1);
        memcpy(&gelen, CMSG_DATA(cm), sizeof(int));
        printf("yazma: %d\n", (int)write(gelen, "geçti", 6));
        close(gelen);
        n = read(p[0], tampon, sizeof(tampon) - 1);
        tampon[n > 0 ? n : 0] = 0;
        printf("boru: %s\n", tampon);
    }
    errno = 0;
    n = connect(socket(AF_UNIX, SOCK_STREAM, 0), (struct sockaddr*)&a, soyut(&a, "gsoket-yok"));
    printf("connect: %d %s\n", (int)n, strerror(errno));
    printf("glibc soket bitti\n");
    return 0;
}
