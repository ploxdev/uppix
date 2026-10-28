/* sctest4: AF_UNIX soketleri ve hazırlık çağrılarını sınar: socketpair, dinleyen soket
 * (dosya yolu ve soyut ad), connect/accept, SCM_RIGHTS, SO_PEERCRED, SO_PASSCRED,
 * shutdown/EOF, EPIPE; poll, select, epoll (seviye, EPOLLET, EPOLLONESHOT); eventfd,
 * timerfd, setitimer/alarm; fork ile istemci-sunucu. Çıktısı ortamdan bağımsızdır
 * (PID, adres, zaman yazılmaz): gerçek Linux'taki ve Uppix'teki çıktı satır satır aynı
 * olmalıdır. /tmp/sct4.* dosyalarını kurar ve sonunda siler. */
#include "linux.h"

#define F_GETFD 1
#define F_SETFL 4
#define SIGPIPE 13
#define SIGALRM 14

#define YOL "/tmp/sct4.sock"
#define DOSYA "/tmp/sct4.dosya"
#define SOYUT "sct4-soyut"

static char tampon[4096];
static char buyuk[300000];

static long sc4(long n, long a, long b, long c, long d) { return sc6(n, a, b, c, d, 0, 0); }

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

static void metin_satir(const char* ad, const char* b, long n) {
    yaz(ad);
    yaz(": \"");
    if (n > 0) sys_write(1, b, (size_t)n);
    yaz("\"\n");
}

static int adres_kur(struct sockaddr_un* a, const char* ad, int soyut) {
    int i = 0;
    memset(a, 0, sizeof(*a));
    a->sun_family = AF_UNIX;
    if (soyut) {
        while (ad[i]) {
            a->sun_path[1 + i] = ad[i];
            i++;
        }
        return 2 + 1 + i;
    }
    while (ad[i]) {
        a->sun_path[i] = ad[i];
        i++;
    }
    return 2 + i + 1;
}

static void sinyal_eylem(int sig, long isleyici) {
    long e[4] = {isleyici, 0, 0, 0};
    sc6(SYS_rt_sigaction, sig, (long)e, 0, 8, 0, 0);
}

/* hiçbir tanımlayıcı beklemeden uyur (fd -1 yok sayılır) */
static long uyu_poll(int ms) {
    struct pollfd p;
    p.fd = -1;
    p.events = 0;
    p.revents = 0;
    return sc3(SYS_poll, &p, 1, ms);
}

static long poll1(int fd, int olay, int ms) {
    struct pollfd p;
    p.fd = fd;
    p.events = (short)olay;
    p.revents = 0;
    long r = sc3(SYS_poll, &p, 1, ms);
    return r < 0 ? r : p.revents;
}

/* SCM_RIGHTS ile fd gönderir */
static long fd_gonder(int sk, const int* fdler, int n, const char* veri) {
    struct msghdr m;
    struct iovec v;
    long kontrol[8];
    struct cmsghdr* c = (struct cmsghdr*)kontrol;
    memset(&m, 0, sizeof(m));
    memset(kontrol, 0, sizeof(kontrol));
    v.iov_base = (void*)veri;
    v.iov_len = uzunluk(veri);
    m.msg_iov = &v;
    m.msg_iovlen = 1;
    m.msg_control = kontrol;
    m.msg_controllen = (sizeof(struct cmsghdr) + 4 * (size_t)n + 7) / 8 * 8;
    c->cmsg_len = sizeof(struct cmsghdr) + 4 * (size_t)n;
    c->cmsg_level = SOL_SOCKET;
    c->cmsg_type = SCM_RIGHTS;
    memcpy((char*)(c + 1), fdler, 4 * (size_t)n);
    return sc3(SYS_sendmsg, sk, &m, 0);
}

int main(int argc, char** argv, char** envp) {
    int sv[2], p[2], st, i;
    long r, fd, srv, cli, acc, c;
    struct sockaddr_un a, b;
    unsigned int alen;
    struct stat sb;
    struct ucred kim;
    unsigned int olen;
    int deger;
    (void)argc;
    (void)argv;
    (void)envp;
    yaz("=== sctest4 başla\n");
    sc1(SYS_unlink, YOL);
    sc1(SYS_unlink, DOSYA);

    /* ---- socketpair (akış) ---- */
    satir("socketpair", sc4(SYS_socketpair, AF_UNIX, SOCK_STREAM, 0, (long)sv));
    satir("write", sys_write(sv[0], "merhaba", 7));
    sys_ioctl(sv[1], FIONREAD, &deger);
    satir("FIONREAD", deger);
    r = sc6(SYS_recvfrom, sv[1], (long)tampon, 3, MSG_PEEK, 0, 0);
    metin_satir("MSG_PEEK", tampon, r);
    r = sys_read(sv[1], tampon, sizeof(tampon));
    metin_satir("read", tampon, r);
    sys_fstat(sv[0], &sb);
    satir("fstat S_IFSOCK", (sb.st_mode & S_IFMT) == S_IFSOCK);
    satir("lseek", sys_lseek(sv[0], 0, SEEK_SET));
    olen = 4;
    sc6(SYS_getsockopt, sv[0], SOL_SOCKET, SO_TYPE, (long)&deger, (long)&olen, 0);
    satir("SO_TYPE", deger);
    satir("SO_TYPE uzunluk", olen);
    olen = sizeof(kim);
    sc6(SYS_getsockopt, sv[0], SOL_SOCKET, SO_PEERCRED, (long)&kim, (long)&olen, 0);
    satir("SO_PEERCRED pid", kim.pid == sc0(SYS_getpid));
    satir("SO_PEERCRED uid", kim.uid == (unsigned)sc0(102));
    satir("SO_PEERCRED uzunluk", olen);
    olen = 4;
    satir("SO_ERROR", sc6(SYS_getsockopt, sv[0], SOL_SOCKET, SO_ERROR, (long)&deger, (long)&olen, 0) == 0 && deger == 0);
    satir("bilinmeyen seçenek", sc6(SYS_getsockopt, sv[0], SOL_SOCKET, 999, (long)&deger, (long)&olen, 0));
    deger = 8192;
    sc6(SYS_setsockopt, sv[0], SOL_SOCKET, SO_SNDBUF, (long)&deger, 4, 0);
    olen = 4;
    sc6(SYS_getsockopt, sv[0], SOL_SOCKET, SO_SNDBUF, (long)&deger, (long)&olen, 0);
    satir("SO_SNDBUF 8192 →", deger);
    satir("poll OUT", poll1(sv[0], POLLIN | POLLOUT, 0));
    satir("shutdown SHUT_WR", sc2(SYS_shutdown, sv[0], SHUT_WR));
    satir("eş EOF", sys_read(sv[1], tampon, sizeof(tampon)));
    satir("eş poll", poll1(sv[1], POLLIN | POLLOUT | POLLRDHUP, 0));
    satir("yazma kapalı EPIPE", sc6(SYS_sendto, sv[0], (long)"x", 1, MSG_NOSIGNAL, 0, 0));
    satir("eş hâlâ yazar", sys_write(sv[1], "geri", 4));
    r = sys_read(sv[0], tampon, sizeof(tampon));
    metin_satir("ters yön", tampon, r);
    sys_close(sv[0]);
    satir("kapanınca poll", poll1(sv[1], POLLIN | POLLOUT | POLLRDHUP, 0));
    satir("kapanınca read", sys_read(sv[1], tampon, sizeof(tampon)));
    satir("kapanınca EPIPE", sc6(SYS_sendto, sv[1], (long)"x", 1, MSG_NOSIGNAL, 0, 0));
    sys_close(sv[1]);

    /* ---- socketpair (datagram), readv/writev, MSG_TRUNC ---- */
    sc4(SYS_socketpair, AF_UNIX, SOCK_DGRAM, 0, (long)sv);
    sys_write(sv[0], "bir", 3);
    sys_write(sv[0], "ikinci", 6);
    {
        struct iovec v[2];
        v[0].iov_base = "par";
        v[0].iov_len = 3;
        v[1].iov_base = "ça";
        v[1].iov_len = 3;
        satir("writev tek ileti", sc3(SYS_writev, sv[0], v, 2));
    }
    r = sys_read(sv[1], tampon, sizeof(tampon));
    metin_satir("ileti 1", tampon, r);
    r = sc6(SYS_recvfrom, sv[1], (long)tampon, 2, MSG_TRUNC, 0, 0);
    satir("MSG_TRUNC gerçek boy", r);
    r = sys_read(sv[1], tampon, sizeof(tampon));
    metin_satir("ileti 3", tampon, r);
    satir("boş kuyruk EAGAIN", sc6(SYS_recvfrom, sv[1], (long)tampon, 10, MSG_DONTWAIT, 0, 0));
    sys_close(sv[0]);
    sys_close(sv[1]);

    /* ---- hata kodları ---- */
    satir("desteklenmeyen aile", sc3(SYS_socket, 200, SOCK_STREAM, 0));
    satir("protokol", sc3(SYS_socket, AF_UNIX, SOCK_STREAM, 7));
    fd = sc3(SYS_socket, AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    satir("SOCK_CLOEXEC", sc2(SYS_fcntl, fd, F_GETFD));
    satir("bağlısız read", sys_read((int)fd, tampon, 1));
    satir("bağlısız write", sc6(SYS_sendto, fd, (long)"x", 1, MSG_NOSIGNAL, 0, 0));
    satir("adsız listen", sc2(SYS_listen, fd, 1));
    alen = sizeof(b);
    satir("getpeername", sc3(SYS_getpeername, fd, &b, &alen));
    alen = sizeof(b);
    satir("getsockname adsız", sc3(SYS_getsockname, fd, &b, &alen));
    satir("adsız uzunluk", alen);
    satir("yeni soket poll", poll1((int)fd, POLLIN | POLLOUT, 0));
    satir("shutdown bağlısız", sc2(SYS_shutdown, fd, SHUT_RDWR));
    satir("soket değil", sc2(SYS_listen, 0, 1) == -88 || sc2(SYS_listen, 0, 1) == -25);
    sys_close((int)fd);

    /* ---- dosya yolu adresi ---- */
    srv = sc3(SYS_socket, AF_UNIX, SOCK_STREAM, 0);
    alen = (unsigned)adres_kur(&a, YOL, 0);
    satir("bind yol", sc3(SYS_bind, srv, &a, alen));
    satir("stat S_IFSOCK", sys_stat(YOL, &sb) == 0 && (sb.st_mode & S_IFMT) == S_IFSOCK);
    satir("bind ikinci", sc3(SYS_bind, srv, &a, alen));
    fd = sc3(SYS_socket, AF_UNIX, SOCK_STREAM, 0);
    satir("bind kullanımda", sc3(SYS_bind, fd, &a, alen));
    sys_close((int)fd);
    alen = sizeof(b);
    sc3(SYS_getsockname, srv, &b, &alen);
    satir("getsockname uzunluk", alen);
    metin_satir("getsockname yol", b.sun_path, (long)uzunluk(b.sun_path));
    cli = sc3(SYS_socket, AF_UNIX, SOCK_STREAM, 0);
    satir("dinlemeyene connect", sc3(SYS_connect, cli, &a, adres_kur(&a, YOL, 0)));
    satir("listen", sc2(SYS_listen, srv, 4));
    satir("dinleyen poll", poll1((int)srv, POLLIN | POLLOUT, 0));
    {
        struct sockaddr_un yok;
        long dfd = sys_open(DOSYA, O_CREAT | O_WRONLY, 0644);
        sys_close((int)dfd);
        satir("connect yok", sc3(SYS_connect, cli, &yok, adres_kur(&yok, "/tmp/sct4.yok", 0)));
        satir("connect dosya", sc3(SYS_connect, cli, &yok, adres_kur(&yok, DOSYA, 0)));
    }
    satir("connect", sc3(SYS_connect, cli, &a, adres_kur(&a, YOL, 0)));
    satir("connect ikinci", sc3(SYS_connect, cli, &a, adres_kur(&a, YOL, 0)));
    satir("bekleyen poll", poll1((int)srv, POLLIN, 0));
    alen = sizeof(b);
    acc = sc6(SYS_accept4, srv, (long)&b, (long)&alen, SOCK_CLOEXEC, 0, 0);
    satir("accept4", acc >= 0);
    satir("istemci adresi uzunluk", alen);
    satir("accept4 CLOEXEC", sc2(SYS_fcntl, acc, F_GETFD));
    alen = sizeof(b);
    sc3(SYS_getpeername, cli, &b, &alen);
    metin_satir("getpeername", b.sun_path, (long)uzunluk(b.sun_path));
    alen = sizeof(b);
    sc3(SYS_getsockname, acc, &b, &alen);
    metin_satir("kabul edilenin adı", b.sun_path, (long)uzunluk(b.sun_path));
    olen = sizeof(kim);
    sc6(SYS_getsockopt, acc, SOL_SOCKET, SO_PEERCRED, (long)&kim, (long)&olen, 0);
    satir("kabul SO_PEERCRED", kim.pid == sc0(SYS_getpid));
    sc3(SYS_fcntl, srv, F_SETFL, O_NONBLOCK);
    satir("boş accept EAGAIN", sc3(SYS_accept, srv, 0, 0));
    sys_write((int)cli, "yol üzerinden", uzunluk("yol üzerinden"));
    r = sys_read((int)acc, tampon, sizeof(tampon));
    metin_satir("sunucu aldı", tampon, r);
    sys_close((int)cli);
    sys_close((int)acc);
    sys_close((int)srv);
    satir("kapalı sokete connect", sc3(SYS_connect, sc3(SYS_socket, AF_UNIX, SOCK_STREAM, 0), &a, adres_kur(&a, YOL, 0)));
    satir("unlink", sc1(SYS_unlink, YOL));
    sc1(SYS_unlink, DOSYA);

    /* ---- soyut ad ve fork ile istemci-sunucu ---- */
    srv = sc3(SYS_socket, AF_UNIX, SOCK_STREAM, 0);
    alen = (unsigned)adres_kur(&a, SOYUT, 1);
    satir("bind soyut", sc3(SYS_bind, srv, &a, alen));
    alen = sizeof(b);
    sc3(SYS_getsockname, srv, &b, &alen);
    satir("soyut uzunluk", alen);
    metin_satir("soyut ad", b.sun_path + 1, (long)alen - 3);
    satir("soyut stat yok", sys_stat(SOYUT, &sb));
    sc2(SYS_listen, srv, 2);
    {
        struct sockaddr_un yok;
        long k = sc3(SYS_socket, AF_UNIX, SOCK_STREAM, 0);
        satir("soyut yok", sc3(SYS_connect, k, &yok, adres_kur(&yok, "sct4-yok", 1)));
        sys_close((int)k);
    }
    c = sys_fork();
    if (c == 0) {
        long k = sc3(SYS_socket, AF_UNIX, SOCK_STREAM, 0);
        long n;
        sys_close((int)srv);
        if (sc3(SYS_connect, k, &a, adres_kur(&a, SOYUT, 1)) != 0) sys_exit(1);
        sys_write((int)k, "çocuktan istek", uzunluk("çocuktan istek"));
        n = sys_read((int)k, tampon, sizeof(tampon));
        sys_exit(n == 5 && tampon[0] == 'y' ? 42 : 2);
    }
    satir("çocuk bekleniyor", poll1((int)srv, POLLIN, 5000));
    acc = sc3(SYS_accept, srv, 0, 0);
    r = sys_read((int)acc, tampon, sizeof(tampon));
    metin_satir("sunucu aldı", tampon, r);
    sys_write((int)acc, "yanıt", 5);
    sys_wait4(c, &st, 0);
    durum("istemci", st);
    satir("istemci gitti", sys_read((int)acc, tampon, sizeof(tampon)));
    sys_close((int)acc);
    sys_close((int)srv);

    /* ---- SCM_RIGHTS ---- */
    sc4(SYS_socketpair, AF_UNIX, SOCK_STREAM, 0, (long)sv);
    sys_pipe(p);
    satir("sendmsg", fd_gonder(sv[0], &p[0], 1, "fd"));
    sys_close(p[0]);
    {
        struct msghdr m;
        struct iovec v;
        long kontrol[8];
        struct cmsghdr* cm = (struct cmsghdr*)kontrol;
        int gelen;
        memset(&m, 0, sizeof(m));
        memset(kontrol, 0, sizeof(kontrol));
        v.iov_base = tampon;
        v.iov_len = sizeof(tampon);
        m.msg_iov = &v;
        m.msg_iovlen = 1;
        m.msg_control = kontrol;
        m.msg_controllen = sizeof(kontrol);
        r = sc3(SYS_recvmsg, sv[1], &m, 0);
        metin_satir("recvmsg veri", tampon, r);
        satir("msg_controllen", (long)m.msg_controllen);
        satir("cmsg_len", (long)cm->cmsg_len);
        satir("cmsg_level", cm->cmsg_level);
        satir("cmsg_type", cm->cmsg_type);
        gelen = *(int*)(cm + 1);
        sys_write(p[1], "boru", 4);
        r = sys_read(gelen, tampon, sizeof(tampon));
        metin_satir("gelen fd'den", tampon, r);
        sys_close(gelen);
        /* iki fd, tek fd'lik yer: MSG_CTRUNC */
        {
            int iki[2];
            iki[0] = p[1];
            iki[1] = p[1];
            fd_gonder(sv[0], iki, 2, "x");
        }
        memset(kontrol, 0, sizeof(kontrol));
        m.msg_control = kontrol;
        m.msg_controllen = 20;
        v.iov_len = sizeof(tampon);
        r = sc3(SYS_recvmsg, sv[1], &m, 0);
        satir("dar kontrol MSG_CTRUNC", (m.msg_flags & MSG_CTRUNC) != 0);
        satir("dar kontrol fd sayısı", ((long)cm->cmsg_len - 16) / 4);
        sys_close(*(int*)(cm + 1));
        /* read ile alınan haklar atılır */
        fd_gonder(sv[0], &p[1], 1, "y");
        satir("read haklı ileti", sys_read(sv[1], tampon, sizeof(tampon)));
    }
    sys_close(p[1]);
    sys_close(sv[0]);
    sys_close(sv[1]);

    /* ---- SO_PASSCRED ---- */
    sc4(SYS_socketpair, AF_UNIX, SOCK_DGRAM, 0, (long)sv);
    deger = 1;
    sc6(SYS_setsockopt, sv[1], SOL_SOCKET, SO_PASSCRED, (long)&deger, 4, 0);
    sys_write(sv[0], "k", 1);
    {
        struct msghdr m;
        struct iovec v;
        long kontrol[8];
        struct cmsghdr* cm = (struct cmsghdr*)kontrol;
        memset(&m, 0, sizeof(m));
        memset(kontrol, 0, sizeof(kontrol));
        v.iov_base = tampon;
        v.iov_len = 8;
        m.msg_iov = &v;
        m.msg_iovlen = 1;
        m.msg_control = kontrol;
        m.msg_controllen = sizeof(kontrol);
        sc3(SYS_recvmsg, sv[1], &m, 0);
        satir("SCM_CREDENTIALS tür", cm->cmsg_type);
        satir("SCM_CREDENTIALS uzunluk", (long)cm->cmsg_len);
        satir("SCM_CREDENTIALS pid", ((struct ucred*)(cm + 1))->pid == sc0(SYS_getpid));
    }
    sys_close(sv[0]);
    sys_close(sv[1]);

    /* ---- poll ---- */
    sys_pipe(p);
    satir("poll zaman aşımı", poll1(p[0], POLLIN, 30));
    sys_write(p[1], "a", 1);
    satir("poll veri", poll1(p[0], POLLIN, 1000));
    satir("poll yazma ucu", poll1(p[1], POLLOUT, 0));
    sys_close(p[1]);
    satir("poll yazıcısız", poll1(p[0], POLLIN, 0));
    sys_read(p[0], tampon, 1);
    satir("poll EOF", poll1(p[0], POLLIN, 0));
    sys_close(p[0]);
    satir("poll kapalı fd", poll1(p[0], POLLIN, 0));
    {
        struct pollfd pf[2];
        pf[0].fd = -1;
        pf[0].events = POLLIN;
        pf[1].fd = 0x7fff;
        pf[1].events = POLLIN;
        satir("poll -1 ve geçersiz", sc3(SYS_poll, pf, 2, 0));
        satir("poll -1 revents", pf[0].revents);
        satir("poll geçersiz revents", pf[1].revents);
    }

    /* ---- select ---- */
    sys_pipe(p);
    {
        unsigned long r_k[16], w_k[16];
        struct timeval tv = {0, 0};
        memset(r_k, 0, sizeof(r_k));
        r_k[0] = 1UL << p[0];
        satir("select boş", sc6(SYS_select, p[0] + 1, (long)r_k, 0, 0, (long)&tv, 0));
        sys_write(p[1], "s", 1);
        r_k[0] = 1UL << p[0];
        memset(w_k, 0, sizeof(w_k));
        w_k[0] = 1UL << p[1];
        tv.tv_sec = 1;
        satir("select hazır", sc6(SYS_select, p[1] + 1, (long)r_k, (long)w_k, 0, (long)&tv, 0));
        satir("select okuma biti", (r_k[0] >> p[0]) & 1);
        satir("select yazma biti", (w_k[0] >> p[1]) & 1);
        r_k[0] = 1UL << 20;
        tv.tv_sec = 0;
        satir("select EBADF", sc6(SYS_select, 21, (long)r_k, 0, 0, (long)&tv, 0));
    }
    sys_close(p[0]);
    sys_close(p[1]);

    /* ---- epoll ---- */
    {
        struct epoll_event e, ev[4];
        long ep = sc1(SYS_epoll_create1, EPOLL_CLOEXEC);
        long dfd;
        satir("epoll CLOEXEC", sc2(SYS_fcntl, ep, F_GETFD));
        satir("epoll_create 0", sc1(SYS_epoll_create, 0));
        sys_pipe(p);
        e.events = EPOLLIN;
        e.data = 7;
        satir("epoll ADD", sc4(SYS_epoll_ctl, ep, EPOLL_CTL_ADD, p[0], (long)&e));
        satir("epoll ADD ikinci", sc4(SYS_epoll_ctl, ep, EPOLL_CTL_ADD, p[0], (long)&e));
        satir("epoll boş", sc4(SYS_epoll_wait, ep, (long)ev, 4, 0));
        sys_write(p[1], "e", 1);
        r = sc4(SYS_epoll_wait, ep, (long)ev, 4, 1000);
        satir("epoll hazır", r);
        satir("epoll veri", (long)ev[0].data);
        satir("epoll olay", ev[0].events);
        satir("epoll seviye tekrar", sc4(SYS_epoll_wait, ep, (long)ev, 4, 0));
        satir("epoll fd poll", poll1((int)ep, POLLIN, 0));
        e.events = EPOLLIN | EPOLLET;
        e.data = 8;
        satir("epoll MOD ET", sc4(SYS_epoll_ctl, ep, EPOLL_CTL_MOD, p[0], (long)&e));
        satir("ET ilk", sc4(SYS_epoll_wait, ep, (long)ev, 4, 0));
        satir("ET tekrar", sc4(SYS_epoll_wait, ep, (long)ev, 4, 0));
        sys_write(p[1], "f", 1);
        satir("ET yeni veri", sc4(SYS_epoll_wait, ep, (long)ev, 4, 0));
        e.events = EPOLLIN | EPOLLONESHOT;
        e.data = 9;
        sc4(SYS_epoll_ctl, ep, EPOLL_CTL_MOD, p[0], (long)&e);
        satir("ONESHOT ilk", sc4(SYS_epoll_wait, ep, (long)ev, 4, 0));
        satir("ONESHOT tekrar", sc4(SYS_epoll_wait, ep, (long)ev, 4, 0));
        sc4(SYS_epoll_ctl, ep, EPOLL_CTL_MOD, p[0], (long)&e);
        satir("ONESHOT yeniden kuruldu", sc4(SYS_epoll_wait, ep, (long)ev, 4, 0));
        satir("epoll DEL", sc4(SYS_epoll_ctl, ep, EPOLL_CTL_DEL, p[0], 0));
        satir("epoll DEL ikinci", sc4(SYS_epoll_ctl, ep, EPOLL_CTL_DEL, p[0], 0));
        dfd = sys_open("/proc/self/exe", O_RDONLY, 0);
        if (dfd < 0) dfd = sys_open("/etc/hostname", O_RDONLY, 0);
        satir("epoll dosya EPERM", sc4(SYS_epoll_ctl, ep, EPOLL_CTL_ADD, dfd, (long)&e));
        sys_close((int)dfd);
        satir("epoll kendisi", sc4(SYS_epoll_ctl, ep, EPOLL_CTL_ADD, ep, (long)&e));
        satir("epoll_wait maxevents 0", sc4(SYS_epoll_wait, ep, (long)ev, 0, 0));
        /* soket ve eventfd aynı kümede */
        sc4(SYS_socketpair, AF_UNIX, SOCK_STREAM, 0, (long)sv);
        e.events = EPOLLIN;
        e.data = 20;
        sc4(SYS_epoll_ctl, ep, EPOLL_CTL_ADD, sv[1], (long)&e);
        fd = sc2(SYS_eventfd2, 0, EFD_NONBLOCK);
        e.data = 21;
        sc4(SYS_epoll_ctl, ep, EPOLL_CTL_ADD, fd, (long)&e);
        satir("küme boş", sc4(SYS_epoll_wait, ep, (long)ev, 4, 50));
        {
            unsigned long bir = 1;
            sys_write((int)fd, &bir, 8);
        }
        r = sc4(SYS_epoll_wait, ep, (long)ev, 4, 1000);
        satir("eventfd hazır", r == 1 && ev[0].data == 21);
        sys_write(sv[0], "s", 1);
        r = sc4(SYS_epoll_wait, ep, (long)ev, 4, 1000);
        satir("ikisi hazır", r);
        sys_close(sv[0]);
        sys_close(sv[1]);
        sys_close((int)fd);
        sys_close(p[0]);
        sys_close(p[1]);
        satir("kapananlar düştü", sc4(SYS_epoll_wait, ep, (long)ev, 4, 0));
        /* kapanan tanımlayıcının kaydı kalkar: aynı numara yeni bir nesneye verilince yeniden
           eklenebilir (X sunucusu kapattığı istemcinin numarasını yeni bağlantıya verir) */
        sc4(SYS_socketpair, AF_UNIX, SOCK_STREAM, 0, (long)sv);
        e.events = EPOLLIN | EPOLLET;
        e.data = 30;
        sc4(SYS_epoll_ctl, ep, EPOLL_CTL_ADD, sv[1], (long)&e);
        sys_close(sv[1]);
        satir("kapanmış DEL", sc4(SYS_epoll_ctl, ep, EPOLL_CTL_DEL, sv[1], 0));
        sys_close(sv[0]);
        sc4(SYS_socketpair, AF_UNIX, SOCK_STREAM, 0, (long)p);
        satir("aynı numara", p[1] == sv[1] || p[0] == sv[1]);
        sys_write(p[0], "z", 1);
        e.data = 31;
        satir("yeniden ADD", sc4(SYS_epoll_ctl, ep, EPOLL_CTL_ADD, sv[1], (long)&e));
        r = sc4(SYS_epoll_wait, ep, (long)ev, 4, 1000);
        satir("ET hazır veri", r == 1 && ev[0].data == 31);
        sys_close(p[0]);
        sys_close(p[1]);
        sys_close((int)ep);
    }

    /* ---- eventfd ---- */
    {
        unsigned long v = 0;
        fd = sc2(SYS_eventfd2, 5, EFD_NONBLOCK);
        satir("eventfd poll", poll1((int)fd, POLLIN | POLLOUT, 0));
        satir("eventfd read", sys_read((int)fd, &v, 8));
        satir("eventfd değer", (long)v);
        satir("eventfd boş", sys_read((int)fd, &v, 8));
        satir("eventfd kısa", sys_read((int)fd, &v, 4));
        v = ~0UL;
        satir("eventfd geçersiz", sys_write((int)fd, &v, 8));
        satir("eventfd bayrak", sc2(SYS_eventfd2, 0, 0x10));
        sys_close((int)fd);
        fd = sc2(SYS_eventfd2, 3, EFD_SEMAPHORE | EFD_NONBLOCK);
        sys_read((int)fd, &v, 8);
        satir("semafor", (long)v);
        sys_read((int)fd, &v, 8);
        sys_read((int)fd, &v, 8);
        satir("semafor bitti", sys_read((int)fd, &v, 8));
        sys_close((int)fd);
    }

    /* ---- timerfd ---- */
    {
        struct itimerspec it, eski;
        unsigned long n = 0;
        fd = sc2(SYS_timerfd_create, CLOCK_MONOTONIC, TFD_NONBLOCK);
        satir("timerfd boş", sys_read((int)fd, &n, 8));
        memset(&it, 0, sizeof(it));
        it.it_value.tv_nsec = 30000000;
        satir("timerfd_settime", sc4(SYS_timerfd_settime, fd, 0, (long)&it, 0));
        satir("timerfd poll", poll1((int)fd, POLLIN, 2000));
        satir("timerfd read", sys_read((int)fd, &n, 8));
        satir("dolma", (long)n);
        sc2(SYS_timerfd_gettime, fd, &eski);
        satir("bitince kalan", eski.it_value.tv_sec + eski.it_value.tv_nsec);
        it.it_value.tv_sec = 10;
        it.it_value.tv_nsec = 0;
        it.it_interval.tv_sec = 3;
        sc4(SYS_timerfd_settime, fd, 0, (long)&it, 0);
        sc2(SYS_timerfd_gettime, fd, &eski);
        satir("kurulu aralık", eski.it_interval.tv_sec);
        satir("kurulu kalan", eski.it_value.tv_sec);
        it.it_value.tv_nsec = 1000000000;
        satir("timerfd geçersiz", sc4(SYS_timerfd_settime, fd, 0, (long)&it, 0));
        sys_close((int)fd);
        satir("timerfd saat", sc2(SYS_timerfd_create, 42, 0));
    }

    /* ---- setitimer / alarm ---- */
    {
        struct itimerval it, eski;
        satir("alarm ilk", sc1(SYS_alarm, 5));
        satir("alarm önceki", sc1(SYS_alarm, 0));
        memset(&it, 0, sizeof(it));
        it.it_value.tv_sec = 2;
        satir("setitimer", sc3(SYS_setitimer, ITIMER_REAL, &it, 0));
        sc2(SYS_getitimer, ITIMER_REAL, &eski);
        satir("getitimer saniye", eski.it_value.tv_sec);
        memset(&it, 0, sizeof(it));
        sc3(SYS_setitimer, ITIMER_REAL, &it, &eski);
        satir("eski değer var", eski.it_value.tv_sec + eski.it_value.tv_usec > 0);
        satir("setitimer geçersiz", sc3(SYS_setitimer, 9, &it, 0));
        c = sys_fork();
        if (c == 0) {
            sc1(SYS_alarm, 1);
            sc0(SYS_pause);
            sys_exit(1);
        }
        sys_wait4(c, &st, 0);
        durum("alarm + pause", st);
        c = sys_fork();
        if (c == 0) {
            memset(&it, 0, sizeof(it));
            it.it_value.tv_usec = 50000;
            sc3(SYS_setitimer, ITIMER_REAL, &it, 0);
            for (;;) uyu_poll(1000);
        }
        sys_wait4(c, &st, 0);
        durum("setitimer + poll", st);
        c = sys_fork();
        if (c == 0) {
            sinyal_eylem(SIGALRM, 1);
            sc1(SYS_alarm, 1);
            satir("yok sayılan alarm sonrası poll", uyu_poll(1500));
            sys_exit(3);
        }
        sys_wait4(c, &st, 0);
        durum("yok sayılan alarm", st);
    }

    /* ---- büyük aktarım (dolu tamponda yazan bekler) ---- */
    sc4(SYS_socketpair, AF_UNIX, SOCK_STREAM, 0, (long)sv);
    c = sys_fork();
    if (c == 0) {
        long toplam = 0, n;
        sys_close(sv[0]);
        while ((n = sys_read(sv[1], tampon, sizeof(tampon))) > 0) toplam += n;
        sys_exit(toplam == 300000 ? 0 : 1);
    }
    sys_close(sv[1]);
    for (i = 0; i < 300000; i++) buyuk[i] = (char)i;
    r = 0;
    while (r < 300000) {
        long n = sys_write(sv[0], buyuk + r, 300000 - (size_t)r);
        if (n <= 0) break;
        r += n;
    }
    satir("büyük yazılan", r);
    sys_close(sv[0]);
    sys_wait4(c, &st, 0);
    durum("büyük okuyan", st);

    /* ---- SIGPIPE ---- */
    sc4(SYS_socketpair, AF_UNIX, SOCK_STREAM, 0, (long)sv);
    sys_close(sv[1]);
    c = sys_fork();
    if (c == 0) {
        sys_write(sv[0], "x", 1);
        sys_exit(1);
    }
    sys_wait4(c, &st, 0);
    durum("eşsiz yazma", st);
    sinyal_eylem(SIGPIPE, 1);
    satir("SIG_IGN EPIPE", sys_write(sv[0], "x", 1));
    sinyal_eylem(SIGPIPE, 0);
    sys_close(sv[0]);
    yaz("=== sctest4 bitti ===\n");
    return 0;
}
