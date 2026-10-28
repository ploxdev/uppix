/* Uppix kullanıcı programları için libc'siz, Linux x86_64 uyumlu ortam.
 * Yalnızca Linux sistem çağrıları kullanılır: aynı statik ikili hem gerçek
 * Linux'ta hem Uppix'te değiştirilmeden çalışır.
 *   gcc -static -nostdlib -fno-pie -no-pie -ffreestanding -O2 program.c */
#ifndef UPPIX_LINUX_H
#define UPPIX_LINUX_H

typedef unsigned long size_t;
typedef long ssize_t;

#define SYS_read 0
#define SYS_write 1
#define SYS_open 2
#define SYS_close 3
#define SYS_stat 4
#define SYS_fstat 5
#define SYS_lseek 8
#define SYS_mmap 9
#define SYS_munmap 11
#define SYS_brk 12
#define SYS_ioctl 16
#define SYS_nanosleep 35
#define SYS_exit 60
#define SYS_rt_sigaction 13
#define SYS_rt_sigprocmask 14
#define SYS_pread64 17
#define SYS_pwrite64 18
#define SYS_readv 19
#define SYS_writev 20
#define SYS_access 21
#define SYS_pipe 22
#define SYS_madvise 28
#define SYS_msync 26
#define SYS_dup 32
#define SYS_dup2 33
#define SYS_getpid 39
#define SYS_clone 56
#define SYS_fork 57
#define SYS_vfork 58
#define SYS_execve 59
#define SYS_wait4 61
#define SYS_kill 62
#define SYS_uname 63
#define SYS_fcntl 72
#define SYS_getcwd 79
#define SYS_chdir 80
#define SYS_fchdir 81
#define SYS_rename 82
#define SYS_mkdir 83
#define SYS_rmdir 84
#define SYS_unlink 87
#define SYS_readlink 89
#define SYS_umask 95
#define SYS_getppid 110
#define SYS_getpgrp 111
#define SYS_gettimeofday 96
#define SYS_getrlimit 97
#define SYS_mprotect 10
#define SYS_sigaltstack 131
#define SYS_arch_prctl 158
#define SYS_time 201
#define SYS_futex 202
#define SYS_getdents64 217
#define SYS_waitid 247
#define SYS_clock_gettime 228
#define SYS_clock_getres 229
#define SYS_clock_nanosleep 230
#define SYS_exit_group 231
#define SYS_openat 257
#define SYS_mkdirat 258
#define SYS_newfstatat 262
#define SYS_unlinkat 263
#define SYS_renameat 264
#define SYS_faccessat 269
#define SYS_dup3 292
#define SYS_pipe2 293
#define SYS_poll 7
#define SYS_select 23
#define SYS_pause 34
#define SYS_getitimer 36
#define SYS_alarm 37
#define SYS_setitimer 38
#define SYS_socket 41
#define SYS_connect 42
#define SYS_accept 43
#define SYS_sendto 44
#define SYS_recvfrom 45
#define SYS_sendmsg 46
#define SYS_recvmsg 47
#define SYS_shutdown 48
#define SYS_bind 49
#define SYS_listen 50
#define SYS_getsockname 51
#define SYS_getpeername 52
#define SYS_socketpair 53
#define SYS_setsockopt 54
#define SYS_getsockopt 55
#define SYS_epoll_create 213
#define SYS_epoll_wait 232
#define SYS_epoll_ctl 233
#define SYS_pselect6 270
#define SYS_ppoll 271
#define SYS_timerfd_create 283
#define SYS_eventfd 284
#define SYS_timerfd_settime 286
#define SYS_timerfd_gettime 287
#define SYS_accept4 288
#define SYS_eventfd2 290
#define SYS_epoll_create1 291
#define SYS_prlimit64 302
#define SYS_getrandom 318
#define SYS_link 86
#define SYS_chmod 90
#define SYS_fchmod 91
#define SYS_chown 92
#define SYS_fchown 93
#define SYS_getuid 102
#define SYS_setuid 105
#define SYS_setgid 106
#define SYS_setpgid 109
#define SYS_setsid 112
#define SYS_setresuid 117
#define SYS_getresuid 118
#define SYS_getpgid 121
#define SYS_getsid 124
#define SYS_linkat 265
#define SYS_fchmodat 268
#define FIONBIO 0x5421

#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR 2
#define O_CREAT 0100
#define O_EXCL 0200
#define O_TRUNC 01000
#define O_APPEND 02000
#define O_DIRECTORY 0200000

#define O_CLOEXEC 02000000
#define O_NONBLOCK 04000
#define AT_FDCWD (-100)
#define AT_REMOVEDIR 0x200
#define AT_EMPTY_PATH 0x1000

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define PROT_READ 1
#define PROT_WRITE 2
#define MAP_SHARED 1
#define MS_SYNC 4
#define MAP_PRIVATE 2
#define MAP_ANONYMOUS 0x20
#define MAP_FAILED ((void*)-1)

#define TCGETS 0x5401
#define TIOCGWINSZ 0x5413
#define FBIOGET_VSCREENINFO 0x4600
#define FBIOGET_FSCREENINFO 0x4602
#define KDSETMODE 0x4B3A
#define KDGETMODE 0x4B3B
#define KDGKBMODE 0x4B44
#define KDSKBMODE 0x4B45
#define KD_TEXT 0
#define KD_GRAPHICS 1
#define K_RAW 0
#define K_XLATE 1
#define K_MEDIUMRAW 2
#define VT_GETSTATE 0x5603

/* evdev */
#define EV_SYN 0
#define EV_KEY 1
#define EV_REL 2
#define REL_X 0
#define REL_Y 1
#define REL_WHEEL 8
#define BTN_LEFT 0x110
#define BTN_RIGHT 0x111
#define KEY_ESC 1
#define KEY_Q 16
#define KEY_A 30
#define EVIOC(yon, nr, boy) ((unsigned long)(((unsigned long)(yon) << 30) | ((unsigned long)(boy) << 16) | (0x45UL << 8) | (nr)))
#define EVIOCGVERSION EVIOC(2, 0x01, 4)
#define EVIOCGID EVIOC(2, 0x02, 8)
#define EVIOCGNAME(n) EVIOC(2, 0x06, (n))
#define EVIOCGBIT(ev, n) EVIOC(2, 0x20 + (ev), (n))
#define EVIOCGRAB EVIOC(1, 0x90, 4)

#define S_IFMT 0170000
#define S_IFDIR 0040000
#define S_IFREG 0100000
#define S_IFCHR 0020000
#define S_IFIFO 0010000
#define S_IFSOCK 0140000

#define AF_UNIX 1
#define AF_INET 2
#define SOCK_STREAM 1
#define SOCK_DGRAM 2
#define SOCK_NONBLOCK 04000
#define SOCK_CLOEXEC 02000000
#define SOL_SOCKET 1
#define SO_TYPE 3
#define SO_ERROR 4
#define SO_SNDBUF 7
#define SO_PASSCRED 16
#define SO_PEERCRED 17
#define SCM_RIGHTS 1
#define SCM_CREDENTIALS 2
#define MSG_PEEK 2
#define MSG_CTRUNC 8
#define MSG_TRUNC 0x20
#define MSG_DONTWAIT 0x40
#define MSG_NOSIGNAL 0x4000
#define SHUT_RD 0
#define SHUT_WR 1
#define SHUT_RDWR 2
#define FIONREAD 0x541B

#define POLLIN 1
#define POLLPRI 2
#define POLLOUT 4
#define POLLERR 8
#define POLLHUP 16
#define POLLNVAL 32
#define POLLRDHUP 0x2000
#define EPOLL_CTL_ADD 1
#define EPOLL_CTL_DEL 2
#define EPOLL_CTL_MOD 3
#define EPOLLIN 1
#define EPOLLOUT 4
#define EPOLLONESHOT (1u << 30)
#define EPOLLET (1u << 31)
#define EPOLL_CLOEXEC 02000000
#define EFD_SEMAPHORE 1
#define EFD_NONBLOCK 04000
#define TFD_NONBLOCK 04000
#define CLOCK_MONOTONIC 1
#define ITIMER_REAL 0

struct sockaddr_un {
    unsigned short sun_family;
    char sun_path[108];
};

struct msghdr {
    void* msg_name;
    unsigned int msg_namelen;
    struct iovec* msg_iov;
    size_t msg_iovlen;
    void* msg_control;
    size_t msg_controllen;
    int msg_flags;
};

struct cmsghdr {
    size_t cmsg_len;
    int cmsg_level, cmsg_type;
};

struct ucred {
    int pid;
    unsigned int uid, gid;
};

struct pollfd {
    int fd;
    short events, revents;
};

struct epoll_event {
    unsigned int events;
    unsigned long data;
} __attribute__((packed));


#define WNOHANG 1
#define WEXITED 4
#define P_ALL 0
#define P_PID 1
#define SIGCHLD 17

struct stat {
    unsigned long st_dev, st_ino, st_nlink;
    unsigned int st_mode, st_uid, st_gid, __pad0;
    unsigned long st_rdev;
    long st_size, st_blksize, st_blocks;
    unsigned long st_atime, st_atime_nsec, st_mtime, st_mtime_nsec, st_ctime, st_ctime_nsec;
    long __unused[3];
};

struct timespec {
    long tv_sec, tv_nsec;
};

struct iovec {
    void* iov_base;
    size_t iov_len;
};

struct utsname {
    char sysname[65], nodename[65], release[65], version[65], machine[65], domainname[65];
};

struct rlimit {
    unsigned long rlim_cur, rlim_max;
};

struct timeval {
    long tv_sec, tv_usec;
};

struct itimerval {
    struct timeval it_interval, it_value;
};

struct itimerspec {
    struct timespec it_interval, it_value;
};

struct input_event {
    struct timeval time;
    unsigned short type, code;
    int value;
};

struct input_id {
    unsigned short bustype, vendor, product, version;
};

struct winsize {
    unsigned short ws_row, ws_col, ws_xpixel, ws_ypixel;
};

struct fb_bitfield {
    unsigned int offset, length, msb_right;
};

struct fb_var_screeninfo {
    unsigned int xres, yres, xres_virtual, yres_virtual, xoffset, yoffset, bits_per_pixel, grayscale;
    struct fb_bitfield red, green, blue, transp;
    unsigned int nonstd, activate, height, width, accel_flags, pixclock, left_margin, right_margin;
    unsigned int upper_margin, lower_margin, hsync_len, vsync_len, sync, vmode, rotate, colorspace;
    unsigned int reserved[4];
};

struct fb_fix_screeninfo {
    char id[16];
    unsigned long smem_start;
    unsigned int smem_len, type, type_aux, visual;
    unsigned short xpanstep, ypanstep, ywrapstep;
    unsigned int line_length;
    unsigned long mmio_start;
    unsigned int mmio_len, accel;
    unsigned short capabilities, reserved[2];
};

static inline long sc6(long n, long a, long b, long c, long d, long e, long f) {
    long r;
    register long r10 __asm__("r10") = d;
    register long r8 __asm__("r8") = e;
    register long r9 __asm__("r9") = f;
    __asm__ volatile("syscall"
                     : "=a"(r)
                     : "a"(n), "D"(a), "S"(b), "d"(c), "r"(r10), "r"(r8), "r"(r9)
                     : "rcx", "r11", "memory");
    return r;
}

#define sc0(n) sc6((n), 0, 0, 0, 0, 0, 0)
#define sc1(n, a) sc6((n), (long)(a), 0, 0, 0, 0, 0)
#define sc2(n, a, b) sc6((n), (long)(a), (long)(b), 0, 0, 0, 0)
#define sc3(n, a, b, c) sc6((n), (long)(a), (long)(b), (long)(c), 0, 0, 0)

static inline long sys_read(int fd, void* b, size_t n) { return sc3(SYS_read, fd, b, n); }
static inline long sys_write(int fd, const void* b, size_t n) { return sc3(SYS_write, fd, b, n); }
static inline long sys_open(const char* p, int f, int m) { return sc3(SYS_open, p, f, m); }
static inline long sys_close(int fd) { return sc1(SYS_close, fd); }
static inline long sys_stat(const char* p, struct stat* s) { return sc2(SYS_stat, p, s); }
static inline long sys_fstat(int fd, struct stat* s) { return sc2(SYS_fstat, fd, s); }
static inline long sys_lseek(int fd, long o, int w) { return sc3(SYS_lseek, fd, o, w); }
static inline void* sys_mmap(void* a, size_t n, int p, int f, int fd, long o) {
    return (void*)sc6(SYS_mmap, (long)a, (long)n, p, f, fd, o);
}
static inline long sys_munmap(void* a, size_t n) { return sc2(SYS_munmap, a, n); }
static inline long sys_brk(void* a) { return sc1(SYS_brk, a); }
static inline long sys_ioctl(int fd, unsigned long r, void* a) { return sc3(SYS_ioctl, fd, r, a); }
static inline long sys_ioctl_deger(int fd, unsigned long r, long v) { return sc3(SYS_ioctl, fd, r, v); }
static inline long sys_nanosleep(const struct timespec* t, struct timespec* k) { return sc2(SYS_nanosleep, t, k); }
static inline void sys_exit(int k) {
    for (;;) sc1(SYS_exit, k);
}
static inline long sys_fork(void) { return sc0(SYS_fork); }
static inline long sys_wait4(long pid, int* st, int o) { return sc6(SYS_wait4, pid, (long)st, o, 0, 0, 0); }
static inline long sys_pipe(int* p) { return sc1(SYS_pipe, p); }
static inline long sys_pipe2(int* p, int f) { return sc2(SYS_pipe2, p, f); }
static inline long sys_dup2(int a, int b) { return sc2(SYS_dup2, a, b); }
static inline long sys_execve(const char* p, char* const* a, char* const* e) { return sc3(SYS_execve, p, a, e); }
static inline long sys_kill(long pid, int sig) { return sc2(SYS_kill, pid, sig); }

/* gcc yapı ilklendirmesi için örtük çağırabilir */
void* memset(void* d, int c, size_t n) {
    unsigned char* p = (unsigned char*)d;
    while (n--) *p++ = (unsigned char)c;
    return d;
}

void* memcpy(void* d, const void* s, size_t n) {
    unsigned char* p = (unsigned char*)d;
    const unsigned char* q = (const unsigned char*)s;
    while (n--) *p++ = *q++;
    return d;
}

__attribute__((unused)) static size_t uzunluk(const char* s) {
    size_t n = 0;
    while (s[n]) n++;
    return n;
}

__attribute__((unused)) static int esit(const char* a, const char* b) {
    while (*a && *a == *b) { a++; b++; }
    return *a == *b;
}

__attribute__((unused)) static void yaz(const char* s) { sys_write(1, s, uzunluk(s)); }
__attribute__((unused)) static void hyaz(const char* s) { sys_write(2, s, uzunluk(s)); }

__attribute__((unused)) static void sayi_yaz(long v) {
    char b[24];
    int i = 23;
    int eksi = v < 0;
    unsigned long u = eksi ? (unsigned long)(-v) : (unsigned long)v;
    b[i] = 0;
    do {
        b[--i] = (char)('0' + u % 10);
        u /= 10;
    } while (u);
    if (eksi) b[--i] = '-';
    yaz(b + i);
}

/* "ad: değer\n" */
__attribute__((unused)) static void satir(const char* ad, long v) {
    yaz(ad);
    yaz(": ");
    sayi_yaz(v);
    yaz("\n");
}

int main(int argc, char** argv, char** envp);

__attribute__((used)) static int _giris(long* sp) {
    int argc = (int)sp[0];
    char** argv = (char**)(sp + 1);
    return main(argc, argv, argv + argc + 1);
}

__asm__(".global _start\n"
        "_start:\n"
        "    xor %rbp, %rbp\n"
        "    mov %rsp, %rdi\n"
        "    and $-16, %rsp\n"
        "    call _giris\n"
        "    mov %eax, %edi\n"
        "    mov $60, %eax\n"
        "    syscall\n"
        "    hlt\n");

#endif
