# Uppix

Uppix is a small Unix-like operating system written entirely in **u++**
([U++](https://github.com/ploxdev/upp-lang), a Turkish-keyword systems language that compiles to C).
It boots from disk on x86_64 machines with [Limine](onyukleyici/limine) straight into **64-bit long
mode** (BIOS or UEFI) and uses no libc. The **root file system lives on an ext4 disk**, so changes
persist across reboots. It recognizes, reads and writes **FAT32** and **exFAT** disks such as USB
sticks. The shell, the device drivers, the file system drivers and all 81 commands (including the
`nano` and `vi` editors and a snake game) are u++ source code. Underneath there is only u++'s
freestanding runtime: interrupts, the timer, keyboard/serial input, PS/2 mouse and raw keyboard
queues, ATA PIO disk access, PCI configuration space and MMIO windows, a memory allocator and task
switching.

Uppix runs **user programs in ring 3**. Its system calls are **binary-compatible with Linux
x86_64**: a statically linked Linux ELF (without libc, or **built with glibc via `gcc -static`**)
runs on Uppix unmodified and prints the same output. On top of that it runs an unmodified
**X.Org server (Xfbdev)**, the **JWM** window manager, its own terminal emulator, settings and
session apps, and **Doom**.

```text
root@uppix:~# lsblk
AD      BOYUT TÜR   FSTÜR  ETİKET           BAĞLAMA
sda      192M disk
├─sda1    64M part  vfat   UPPIXBOOT        /boot
└─sda2   127M part  ext4   uppix-kok        /
sdb       64M disk
└─sdb1    63M part  vfat   UPPIXUSB         /mnt/sdb1
sdc       32M disk  exfat                   /mnt/sdc
root@uppix:~# cat "/mnt/sdb1/Türkçe uzun dosya adı.txt"
Uzun ve Türkçe bir dosya adı: şğüöçı ŞĞÜÖÇİ
```

The system's user interface (messages, table headers, command help) is in Turkish, as are the
source file and identifier names; this README explains them in English.

**Türkçe:** bu belgenin Türkçesi [en altta](#türkçe).

## Quick start

Uppix is written in [U++](https://github.com/ploxdev/upp-lang). The compiler is kept in this
repository as a single C file ([`derleyici/`](derleyici), "compiler"); build it first:

```bash
sh derleyici/kur.sh          # Linux / WSL           -> derleyici/uppc
derleyici\kur.cmd           # Windows (MinGW-w64)   -> derleyici\uppc.exe
```

Below, `uppc` means `derleyici/uppc` (on Windows `derleyici\uppc.exe`). `derle` means "build" and
`calistir` means "run".

Requirements: `gcc`, `ld` and `qemu-system-x86_64` (on Windows these can live inside WSL). The disk
images need `mkfs.ext4` (e2fsprogs), `sfdisk` (fdisk), `mkfs.fat` and `mtools`; the exFAT sample
disk needs `mkfs.exfat` (`sudo apt install e2fsprogs dosfstools mtools exfatprogs fdisk`).

```bash
uppc calistir .
```

This compiles the kernel, prepares the bootable `uppix.img` with [`disk_yap.sh`](disk_yap.sh)
(driven by `onyukleyici = limine` and `disk_hazirla` in `upp.proje`), and starts QEMU with three
disks and a serial console. QEMU loads Limine from the disk's MBR; Limine reads the kernel from the
FAT32 partition and starts it in 64-bit mode. You type commands into the terminal. The images stay
under `derleme/` ("build"): later builds only replace `/boot/uppix.elf`, so changes you make on the
root file system are kept. Delete `derleme/*.img` to start from scratch.

### Graphical desktop in a window

To see the screen, the desktop and the mouse, build the images and open QEMU with a display:

```bash
uppc derle .
sh scripts/run-gui.sh        # Linux (UPPIX_DISPLAY=sdl for the SDL backend)
scripts\run-gui.cmd          # Windows: runs QEMU inside WSL, the window appears through WSLg
```

The terminal you start it from is the serial console; the QEMU window shows the framebuffer
console. Then, inside Uppix:

```text
root@uppix:~# startx &
startx: X hazır (:0)
jwm: masaüstü hazır
```

Click into the QEMU window to grab the mouse (Ctrl-Alt-G releases it). `doom` starts Doom,
`poweroff` shuts the machine down and `reboot` restarts it.

| Image | Contents |
|---|---|
| `uppix.img` | 192 MiB, MBR. **Partition 1** is a 64 MiB FAT32 ESP labelled `UPPIXBOOT`: `/limine/limine-bios.sys`, `/limine/limine.conf`, `/EFI/BOOT/BOOTX64.EFI`, `/uppix.elf`; mounted as `/boot` in Uppix. **Partition 2** is a 127 MiB ext4 file system labelled `uppix-kok`: the root (`/`), populated from the [`kok/`](kok) tree |
| `usb-fat32.img` | 64 MiB, MBR partition table with FAT32 (`UPPIXUSB`) in partition 1; sample files with long Turkish names |
| `usb-exfat.img` | 32 MiB exFAT (no partition table) |

### Other ways to boot

To attach the disks to an AHCI (SATA) controller instead of IDE, add `disk_arayuz = ahci` to
`upp.proje` or pass `--disk-arayuz ahci` to `uppc` (up to 6 disks; QEMU gets `-device ahci` and the
disks as `ide-hd,bus=ahci0.N`). By hand:

```bash
qemu-system-x86_64 -m 256 -serial stdio -device ahci,id=ahci0 \
    -drive file=derleme/uppix.img,format=raw,if=none,id=d0 -device ide-hd,drive=d0,bus=ahci0.0,bootindex=0 \
    -drive file=derleme/usb-fat32.img,format=raw,if=none,id=d1 -device ide-hd,drive=d1,bus=ahci0.1
```

To boot with UEFI, give QEMU OVMF, for example `-bios /usr/share/qemu/OVMF.fd`. The same image can
be written to a real USB stick (`dd if=uppix.img of=/dev/sdX`).

The old Multiboot path still works: with `--onyukleyici multiboot` (the default bootloader setting)
the kernel is built as a 32-bit `uppix32.elf` and QEMU boots it with `-kernel`; `sh disk_yap.sh`
(without a kernel argument) builds a separate `uppix-kok.img` ext4 image for that path. Without a
disk, Uppix boots with a root file system in RAM (rootfs). `poweroff` flushes the file systems and
powers off; `reboot` restarts.

## Features

- **File systems:**
  - **ext4:** read and write. Extent trees, legacy ext2/3 block maps, htree directories (read) and
    `metadata_csum` are supported: inode, directory block, bitmap, group descriptor and superblock
    checksums are updated with CRC32C. If an unsupported feature is present, the disk is mounted
    read-only.
  - **FAT32:** read and write. VFAT long file names (UTF-16) are read and written, `NAME~1.EXT`
    short names are generated, and all FAT copies are updated together.
  - **exFAT:** read and write. Allocation bitmap, FAT chains and contiguous (NoFatChain) clusters,
    entry set checksums, name hashes based on the up-case table, and the volume label.
  - **In memory:** `/proc` (`cpuinfo`, `meminfo`, `mounts`, `partitions`, `uptime`, `version`,
    `dmesg`), `/dev` (`null`, `console`, `tty`, `tty0`, `tty1`, `random`, `fb0`, `input/event0`,
    `input/event1`, `input/mice`, `ptmx`, `pts/N`, `sda`…; `st_rdev` uses Linux's numbers), `/tmp`
    and `/mnt` (tmpfs), `/bin` (built-in commands).
- **Device tree and driver framework:** modelled on FreeBSD's newbus. The root is `nexus0`, under it
  `pci0`: PCI buses 0..255 are scanned (including multi-function devices), each device's IDs, class
  and BARs (base and size) are stored in the child's ivars, then drivers are probed; as in FreeBSD,
  the driver with the highest priority wins. Drivers: `hostb`, `isab`, `atapci` (PIIX3/PIIX4 IDE),
  `ata` (channels), `ahci` (SATA controller) and `ahcich` (ports); if PCI has no IDE controller, the
  ATA channels are probed at the root as legacy ISA devices. Drivers get helpers to map a BAR's MMIO
  window, enable memory space and bus mastering, and allocate aligned DMA buffers (with their
  physical address). `devinfo` prints the tree and `pciconf -l` the PCI devices in FreeBSD format.
- **Disks:** disk drivers put the disks they find into a common disk registry; the block layer only
  uses the read/write functions from the registry. Up to 4 disks with ATA PIO, one disk per port
  with AHCI (SATA); names in discovery order `sda`, `sdb`…; MBR and GPT partition tables (`sdb1`…).
  At boot the ext4 labelled `uppix-kok` (or else the first one) is mounted as root, the FAT32
  partition labelled `UPPIXBOOT` on `/boot`, and other file systems under `/mnt/<device>`.
- **Shell (`uppsh`):** pipes (`|`), redirections (`>`, `>>`, `<`, `2>`), `;`, `&&`, `||` and
  background jobs (`&`, `jobs`, `kill %1`). Also `$NAME`, `${NAME}`, `$?`, `$$`, `$1…$9`, `$#`,
  `$@`, `~`, globbing (`*`, `?`), single/double quotes, aliases, scripts (`sh file`, `source`,
  executables found through `PATH`) and `/etc/profile`.
- **Line editor:** ↑/↓ history, ←/→, Home/End, Delete, Tab completion of commands and file names,
  Ctrl-A/E/U/K/W/L/C/D.
- **Tasks:** cooperative multitasking (up to 32 tasks, each with a 64 KiB stack). Tasks yield
  during `sleep` and while waiting for input. They are managed with `ps`, `kill` and `jobs`.
- **Boot:** [Limine](onyukleyici/limine) v11.4.1 is vendored in the repository (BIOS stages,
  `BOOTX64.EFI` and the source of the `bios-install` tool). The kernel is an ELF64 linked at
  `0xffffffff80000000`. It gets the memory map, the framebuffer, the HHDM and its own physical
  address through Limine protocol requests. It then builds its own page tables: the kernel at its
  higher-half address with 4 KiB pages and all physical memory in the HHDM with 2 MiB pages (the
  framebuffer write-combining via PAT). The lower half stays empty for user processes. GDT/TSS,
  `SYSCALL` and NX/SMEP/SMAP are set up. The local APIC is disabled so that 8259 PIC interrupts
  arrive directly.
- **Screen:** Limine leaves the display in graphics mode. Uppix draws its own text console on that
  framebuffer: the [Terminus](https://github.com/ploxdev/upp-lang/blob/main/src/runtime/TERMINUS_LISANS.txt)
  8x16 font (including Turkish characters), ANSI colors (foreground and background, reverse video),
  cursor movement, box-drawing lines, scrolling and an underline cursor. Line wrapping is deferred
  as on a VT100: writing the bottom-right cell does not scroll, so full-screen programs can use the
  last line. When booted through Multiboot, the VGA text screen is used.
- **AHCI (SATA):** the `ahci` driver maps the ABAR (BAR5) as MMIO, enables memory space and bus
  mastering, resets the controller, switches it to AHCI mode and adds an `ahcich` for each
  implemented port. A port whose signature is a SATA disk is identified with IDENTIFY (model, LBA48
  sector count). Reads and writes use READ/WRITE DMA EXT and transfer straight into the caller's
  buffer (the PRDT points at the buffer's physical address); writes are followed by FLUSH CACHE EXT.
  The command list, received FIS and command table are in aligned DMA memory. No interrupts are
  used: command completion is polled with a timeout, an error (`PxTFD.ERR`, `PxIS.TFES`) fails the
  read/write and the port is restarted.
- **Input devices and graphics console:** `atkbd0` and `psm0` under the 8042 controller
  (`atkbdc0`). The mouse works with IntelliMouse detection (sample rate sequence 200/100/80, ID 3)
  using 4-byte packets and the wheel; packets are decoded with their sign/overflow bits and
  re-synchronized on bit 3 if they slip. Linux-compatible devices: `/dev/input/event0` (keyboard)
  and `event1` (mouse) are evdev (24-byte `struct input_event`, `EV_SYN`/`EV_KEY`/`EV_REL`,
  `KEY_*` codes from set-1 scan codes including 0xE0 extended keys and Pause; `EVIOCGVERSION`,
  `EVIOCGID`, `EVIOCGNAME`, `EVIOCGPHYS`, `EVIOCGBIT`, `EVIOCGKEY`, `EVIOCGRAB`, `EVIOCSCLOCKID`);
  `/dev/input/mice` is mousedev (PS/2 3 bytes; ImPS/2 4 bytes after the 200/100/80 rate sequence
  is written). Every open has its own event queue; reads block, and return `EAGAIN` with
  `O_NONBLOCK`. While the keyboard device is open the keyboard is in raw mode (keys do not reach the
  shell). On the console, `KDSETMODE` `KD_GRAPHICS` stops the console from drawing on the
  framebuffer; with `KDSKBMODE` `K_RAW`/`K_MEDIUMRAW` scan/key codes are read from the terminal.
  When the process that changed the modes exits, the console returns to `KD_TEXT` and `K_XLATE`.
- **Hardware:** Limine/Multiboot memory map, CMOS clock (file times are Unix seconds), CPUID,
  power-off through ACPI, reboot through the 8042.

## User mode (ring 3) and Linux system calls

```text
root@uppix:~# merhaba bir iki
Merhaba, ring 3! Linux sistem çağrılarıyla konuşuyorum.
argc: 3
...
root@uppix:~# cokme; echo $?
cokme: boş göstericiye yazılıyor...
Bölümleme hatası (SIGSEGV): komut 0x0000000000401161, adres 0x0000000000000000
139
root@uppix:~# fbdemo
fb: 1280x800, 32 bpp, satır 5120 bayt, uppixfb
fbdemo: çizildi (sağ üst köşe)
```

The shell runs an executable ELF file (from `/usr/bin` or by path) in its own address space in
ring 3. The program talks only through system calls and cannot corrupt the kernel. An exception
(null pointer, invalid instruction, division by zero) only terminates that process, like signals on
Linux: the exit status is 128 + signal, and the shell keeps running.

**Multiple processes:** `fork` copies the address space (every owned page into a new frame; the
framebuffer is shared) and starts the child in its own kernel task with the parent's registers at
the moment of the system call; `fork` returns 0 in the child. The child inherits file descriptors
(with pipe reference counts), working directory, umask, signal actions and mask, resource limits
and shared mapping records; the child's output goes to the same pipe or redirection as the
parent's. `execve` loads an ELF or a `#!` script into a new address space; if loading fails the old
image continues. The process table keeps the real parent PID: an exited process stays a zombie
until its parent reaps it with `wait4`, orphans are re-parented to the shell (1), and the shell
reaps its own children. Pipes are 64 KiB kernel objects.

**System calls:** numbers, flags, errno values and struct layouts are the same as Linux x86_64.
Calls are made with the `syscall` instruction: rax holds the number; rdi, rsi, rdx, r10, r8, r9 the
arguments; errors are returned as `-errno`.

| No | Call | On Uppix |
|---|---|---|
| 0, 1 | `read`, `write` | Files (VFS), terminal (canonical lines: echo, backspace, EOF with Ctrl-D, SIGINT with Ctrl-C), input from the shell (pipe, `<`); stdout is captured into pipes and `>`, stderr goes straight to the terminal |
| 17, 18 | `pread64`, `pwrite64` | Read/write at an offset without moving the file position; `ESPIPE` on terminals, `EISDIR` on directories |
| 19, 20 | `readv`, `writev` | `struct iovec` arrays (up to 1024) |
| 8 | `lseek` | `SEEK_SET/CUR/END`; `ESPIPE` on terminals and pipes; on a directory it resets the `getdents64` position |
| 16 | `ioctl` | `TIOCGWINSZ`/`TIOCSWINSZ` (terminal size; stored per PTY), `TIOCGPTN`, `TIOCSPTLCK` (PTY), `TCGETS`/`TCSETS`/`TCSETSW`/`TCSETSF` (`termios` is stored and read back, reset when the setting process exits; line discipline stays canonical), `TIOCSCTTY`, `TIOCNOTTY`, `TIOCGPGRP`/`TIOCSPGRP`, `TIOCGSID`, `FIONBIO`, `FIONREAD` (sockets, pipes, terminals, files); `FBIOGET_VSCREENINFO`, `FBIOGET_FSCREENINFO`, `FBIOPUT_VSCREENINFO` (current mode only), `FBIOPAN_DISPLAY`, `FBIOBLANK`, `FBIOPUTCMAP`; on the console `KDGETMODE`/`KDSETMODE`, `KDGKBMODE`/`KDSKBMODE`, `KDGKBTYPE`, `KDGETLED`/`KDSETLED`, `VT_OPENQRY`, `VT_GETSTATE`, `VT_GETMODE`/`VT_SETMODE`, `VT_ACTIVATE`/`VT_WAITACTIVE`, `VT_RELDISP`, `VT_DISALLOCATE` (single VT); `EVIOCG*`, `EVIOCGRAB`, `EVIOCSCLOCKID` on evdev |
| 2, 257 | `open`, `openat` | `O_RDONLY/WRONLY/RDWR`, `O_CREAT`, `O_EXCL`, `O_TRUNC`, `O_APPEND`, `O_DIRECTORY`, `O_CLOEXEC`; relative paths against `dirfd` or the process's working directory; `/dev/null`, `/dev/tty`, `/dev/fb0`; every open of `/dev/ptmx` creates a new PTY (up to 16), `/dev/pts/N` is the slave side (`EIO` before `unlockpt`) |
| 3 | `close` | |
| 4, 5, 6, 262 | `stat`, `fstat`, `lstat`, `newfstatat` | 144-byte `struct stat`; `AT_EMPTY_PATH` |
| 32, 33, 292, 72 | `dup`, `dup2`, `dup3`, `fcntl` | `F_DUPFD(_CLOEXEC)`, `F_GETFD/SETFD` (`FD_CLOEXEC`), `F_GETFL/SETFL` (always `O_LARGEFILE` on 64-bit) |
| 217 | `getdents64` | Includes `.` and `..`; `d_type` (`DT_DIR`, `DT_REG`, `DT_CHR`), records aligned to 8 bytes |
| 21, 269, 439 | `access`, `faccessat`, `faccessat2` | `R_OK/W_OK/X_OK` (root); `EROFS` on read-only file systems |
| 79, 80, 81 | `getcwd`, `chdir`, `fchdir` | Every process has its own working directory; it does not change the shell's |
| 83, 258 | `mkdir`, `mkdirat` | The process's `umask` is applied |
| 84, 87, 263 | `rmdir`, `unlink`, `unlinkat` | `AT_REMOVEDIR`; `ENOTEMPTY`, `EISDIR`, `EBUSY` on mount points |
| 82, 264, 316 | `rename`, `renameat`, `renameat2` | Overwrites an existing file; `EISDIR`/`ENOTDIR` on file/directory mismatches |
| 89, 267 | `readlink`, `readlinkat` | `/proc/self/exe` gives the executable's path; there are no symlinks (`EINVAL`) |
| 86, 265 | `link`, `linkat` | The new name is a copy of the file's contents and mode (the VFS has no two names for one inode): atomic "create, `EEXIST` if present" (X's lock file) works, later writes are not shared; `EPERM` on directories, `ENOENT`, `EROFS` |
| 90, 91, 268 | `chmod`, `fchmod`, `fchmodat` | Permission bits (`07777`) are written to the file system's attributes; ignored on sockets, pipes and similar descriptors |
| 92, 93, 94, 260 | `chown`, `fchown`, `lchown`, `fchownat` | Ownership is not tracked: succeeds if the file exists |
| 9, 11, 12, 26 | `mmap`, `munmap`, `brk`, `msync` | Anonymous, file-backed, `MAP_FIXED`; writes to file-backed `MAP_SHARED` mappings go to the file on `msync`, `munmap` and process exit (`MAP_PRIVATE` stays private); `/dev/fb0` with `MAP_SHARED` maps the framebuffer directly |
| 10, 28 | `mprotect`, `madvise` | Page protection; `ENOMEM` on unmapped ranges; `MADV_DONTNEED` zeroes the page |
| 60, 231 | `exit`, `exit_group` | |
| 39, 186, 110, 218 | `getpid`, `gettid`, `getppid`, `set_tid_address` | `getppid` returns the real parent (1 for a process started by the shell) |
| 57, 58, 56 | `fork`, `vfork`, `clone` | Copy of the address space, the child in its own task; `vfork` behaves like `fork`; `clone` only with fork-equivalent flags (`SIGCHLD`, `CLONE_CHILD_SETTID`, `CLONE_CHILD_CLEARTID`, `CLONE_PARENT_SETTID`, `CLONE_SETTLS`; that is how glibc's `fork()` calls it), `ENOSYS` for `CLONE_VM`/`CLONE_THREAD`; `EAGAIN` when the task table (32) is full |
| 59 | `execve` | ELF and `#!` scripts (interpreter + one argument, up to 4 levels); argv/envp from user memory; O_CLOEXEC descriptors are closed, caught signals reset to default (`SIG_IGN` stays), process name and `/proc/self/exe` updated; error order as on Linux: `ENOENT`/`EACCES`, `EFAULT`, `E2BIG` (strings over 131072 bytes), `ENOEXEC`. For processes, `/bin/sh` (`#!/yerlesik/sh`, a kernel built-in) is `/usr/bin/ush`; for an interactive call whose stdin is a PTY slave, a uppsh session starts on that PTY and the process waits for it and exits with the shell's status |
| 162, 306, 74, 75 | `sync`, `syncfs`, `fsync`, `fdatasync` | `sync`/`syncfs` flush the file systems (like the `sync` command); writes already go to disk, so `fsync` only validates the descriptor |
| 169 | `reboot` | Magic numbers are checked (`EINVAL`); `RESTART` reboots, `POWER_OFF`/`HALT` power off (file systems are flushed first), `CAD_ON`/`CAD_OFF` are accepted |
| 61, 247 | `wait4`, `waitid` | pid -1 / > 0 / 0 / < -1, `WNOHANG`; status in Linux encoding (`(code & 0xff) << 8` on exit, the signal number on a signal); `waitid` `P_ALL`/`P_PID`/`P_PGID`, `WEXITED`, `WNOWAIT`, `siginfo` (`CLD_EXITED`/`CLD_KILLED`); `rusage` is zeros; `ECHILD` without children; waiting yields, and Ctrl-C interrupts it in the foreground |
| 22, 293 | `pipe`, `pipe2` | 64 KiB pipe: reads wait for data or EOF (all writers closed), writes wait for space, writes up to 4096 bytes are atomic; SIGPIPE and `EPIPE` without readers; `O_CLOEXEC`, `O_NONBLOCK` (`EAGAIN`); `fstat` S_IFIFO, `lseek` `ESPIPE` |
| 41, 53 | `socket`, `socketpair` | `AF_UNIX`; `SOCK_STREAM` (256 KiB receive buffer, writers wait when full) and `SOCK_DGRAM` (message boundaries preserved); `SOCK_CLOEXEC`, `SOCK_NONBLOCK`; other families `EAFNOSUPPORT`, protocols `EPROTONOSUPPORT`; `fstat` S_IFSOCK, `lseek` `ESPIPE`; `read`/`write`/`readv`/`writev`/`dup`/`fork`/`fcntl` also work on sockets |
| 49, 50, 42, 43, 288 | `bind`, `listen`, `connect`, `accept`, `accept4` | Addresses: file path (`bind` creates an `S_IFSOCK` node on an in-memory file system such as `/tmp`; `EADDRINUSE` if the node exists), abstract name (`sun_path[0] == 0`), unnamed; automatic abstract name when `addrlen` is 2. `connect` establishes the connection immediately, `accept` takes it from the backlog (waits if the backlog is full); `ECONNREFUSED`, `ENOENT`, `EISCONN`, `EPROTOTYPE`; default peer for datagrams, removed with `AF_UNSPEC` |
| 51, 52, 48 | `getsockname`, `getpeername`, `shutdown` | Linux address lengths (unnamed 2, path 2 + length + 1, abstract 3 + length); `ENOTCONN`; `SHUT_RD`/`SHUT_WR`/`SHUT_RDWR` affect both ends (EOF, `EPIPE`) |
| 44, 45, 46, 47 | `sendto`, `recvfrom`, `sendmsg`, `recvmsg` | `MSG_PEEK`, `MSG_DONTWAIT`, `MSG_WAITALL`, `MSG_TRUNC`, `MSG_NOSIGNAL` (otherwise SIGPIPE with `EPIPE`), `MSG_CMSG_CLOEXEC`; passing descriptors with `SCM_RIGHTS` (`MSG_CTRUNC` when there is no room; closed when received with `read`), `SCM_CREDENTIALS` with `SO_PASSCRED` |
| 54, 55 | `setsockopt`, `getsockopt` | `SO_REUSEADDR`, `SO_TYPE`, `SO_ERROR`, `SO_SNDBUF`/`SO_RCVBUF` (doubled like Linux), `SO_KEEPALIVE`, `SO_PASSCRED`, `SO_PEERCRED` (pid/uid/gid), `SO_ACCEPTCONN`, `SO_DOMAIN`, `SO_PROTOCOL`; unknown ones `ENOPROTOOPT` |
| 7, 271, 23, 270 | `poll`, `ppoll`, `select`, `pselect6` | Readiness of every descriptor type (file, pipe, socket, listening socket, terminal, shell input, evdev, mice, `/dev/null`, epoll, eventfd, timerfd) comes from one place (`fd_hazir`): `POLLIN`/`POLLOUT`/`POLLHUP`/`POLLERR`/`POLLRDHUP`/`POLLNVAL`; a terminal is readable on a complete line (or Ctrl-D) in canonical mode and on a raw byte in `K_RAW`; timeouts, the remaining time is written back; Ctrl-C gives `EINTR` in the foreground |
| 213, 291, 233, 232, 281 | `epoll_create`, `epoll_create1`, `epoll_ctl`, `epoll_wait`, `epoll_pwait` | Level-triggered; `EPOLLET` (new events only), `EPOLLONESHOT` (re-armed with `EPOLL_CTL_MOD`); `EEXIST`, `ENOENT`, `EPERM` on regular files; the registration of a closed descriptor (or one whose number now refers to another object) is dropped at once, so the same number can be added again; an epoll descriptor can itself be polled |
| 284, 290 | `eventfd`, `eventfd2` | 64-bit counter, `EFD_SEMAPHORE`, `EFD_NONBLOCK`, `EFD_CLOEXEC`; writers wait if it would overflow |
| 283, 286, 287 | `timerfd_create`, `timerfd_settime`, `timerfd_gettime` | `CLOCK_REALTIME`/`MONOTONIC`/`BOOTTIME`, `TFD_TIMER_ABSTIME`, interval timers (a read returns the expiration count); 1 ms resolution |
| 38, 36, 37, 34 | `setitimer`, `getitimer`, `alarm`, `pause` | `ITIMER_REAL` delivers SIGALRM on expiry (the default action ends the process, `SIG_IGN` ignores it); `ITIMER_VIRTUAL`/`PROF` are accepted but never expire; timers are not inherited by `fork` children |
| 111, 121, 124 | `getpgrp`, `getpgid`, `getsid` | A process started by the shell leads its own group; children inherit the group |
| 112, 109 | `setsid`, `setpgid` | `setsid` creates a new group (the session equals the group), `EPERM` for a group leader; `setpgid` for itself or a child (`ESRCH`, `EINVAL`) |
| 102, 104, 107, 108 | `getuid`, `getgid`, `geteuid`, `getegid` | root (0) |
| 105, 106, 113, 114, 117, 119 | `setuid`, `setgid`, `setreuid`, `setregid`, `setresuid`, `setresgid` | Every process is root: setting 0 or leaving unchanged (-1) succeeds, any other ID is `EPERM` |
| 118, 120, 115, 116 | `getresuid`, `getresgid`, `getgroups`, `setgroups` | IDs are 0; no supplementary groups (`setgroups` only with an empty list) |
| 158 | `arch_prctl` | `ARCH_SET_FS/GS`, `ARCH_GET_FS/GS`: per-process FS/GS base (TLS); `EPERM` for kernel addresses |
| 63 | `uname` | `Linux`, `6.1.0-uppix`, `x86_64`; the node name comes from `/etc/hostname` |
| 228, 229 | `clock_gettime`, `clock_getres` | `REALTIME`, `MONOTONIC`, `BOOTTIME`, `TAI`, `*_COARSE`, `*_RAW`, CPU clocks; 1 ms resolution |
| 96, 201 | `gettimeofday`, `time` | |
| 35, 230 | `nanosleep`, `clock_nanosleep` | Yields to other tasks; `TIMER_ABSTIME`; Ctrl-C interrupts it in the foreground |
| 24 | `sched_yield` | |
| 318 | `getrandom` | `GRND_NONBLOCK/RANDOM/INSECURE` |
| 13, 14, 131 | `rt_sigaction`, `rt_sigprocmask`, `sigaltstack` | Actions and mask are stored and read back (SIGKILL/SIGSTOP cannot be masked); handlers are not invoked |
| 62, 200, 234 | `kill`, `tkill`, `tgkill` | pid > 0 a process, 0 its own group, -1 everyone but itself, < -1 a group; the default action ends the process (status 128 + signal); `SIG_IGN` and signals whose default is to ignore have no effect; Ctrl-C goes to the whole foreground group |
| 97, 160, 302 | `getrlimit`, `setrlimit`, `prlimit64` | `RLIMIT_STACK` 1 MiB, `RLIMIT_NOFILE` 32; values are stored per process |
| 95 | `umask` | |
| 202 | `futex` | `FUTEX_WAIT(_BITSET)`, `FUTEX_WAKE` (single-threaded: a wait ends on timeout or Ctrl-C) |
| 273, 99 | `set_robust_list`, `sysinfo` | |

An unsupported call returns `-ENOSYS` and is logged once to `dmesg`.

**Compatibility tests:** [`kullanici/sctest.c`](kullanici/sctest.c) exercises the basic calls,
[`kullanici/sctest2.c`](kullanici/sctest2.c) file system (including link, chmod, chown),
descriptor (FIONBIO), time, TLS and signal calls, [`kullanici/sctest3.c`](kullanici/sctest3.c)
fork/wait4/waitid, pipes, redirection with dup2, execve (scripts, O_CLOEXEC, environment, error
codes), kill, setsid/setpgid and the identity calls, and [`kullanici/sctest4.c`](kullanici/sctest4.c)
AF_UNIX sockets (socketpair, path and abstract names, SCM_RIGHTS, SO_PEERCRED, SO_PASSCRED,
shutdown, EPIPE), poll/select/epoll, eventfd, timerfd and setitimer/alarm, together with their error
paths; their output contains no PIDs, addresses, inodes or times. The QEMU tests run the same
binaries on Uppix and on real Linux and compare the output with `diff`: all 58 + 182 + 98 + 167
lines are identical. Errnos and the order of checks were chosen by comparing against the real
kernel (e.g. `EOPNOTSUPP` for `clock_nanosleep` on a thread clock, `O_LARGEFILE` in `F_GETFL`).

**glibc:** [`kullanici/glibc/gmerhaba.c`](kullanici/glibc/gmerhaba.c) is an ordinary C program
built with glibc using `gcc -static`. glibc's startup (TLS through `arch_prctl`, `brk`, `mmap`,
`mprotect`, `prlimit64`, `getrandom`…) as well as `printf`, `malloc`, `fopen`, `opendir`,
`strerror` and `__thread` variables run unmodified on Uppix; the output matches Linux. The only
unsupported call is `rseq`, and glibc carries on without it.

**Programs** ([`kullanici/`](kullanici), "user"): [`linux.h`](kullanici/linux.h) is the libc-free
environment (`_start`, system call wrappers, Linux structs). [`disk_yap.sh`](disk_yap.sh) compiles
every C file and puts it into `/usr/bin` on the root file system; `kullanici/glibc/*.c` files are
built with `cc -static` when a static glibc (`libc.a`) is available.

| Program | What it does |
|---|---|
| `merhaba` | Prints argv and the environment; exits with 7 with `--cik` |
| `sctest` | Compatibility test for the basic system calls |
| `sctest2` | Compatibility test for file system, descriptor, time, TLS (`arch_prctl` and `%fs`) and signal calls |
| `sctest3` | Compatibility test for process calls: fork, vfork, clone, wait4, waitid, kill, pipe/pipe2, dup2 and execve (runs itself, `merhaba` and a `#!` script) |
| `sctest4` | Compatibility test for socket and readiness calls: AF_UNIX, SCM_RIGHTS, poll/select/epoll, eventfd, timerfd, setitimer/alarm |
| `gmerhaba` | Built with glibc via `gcc -static`: stdio, malloc, TLS, directory reading; exits with 7 |
| `gsurec` | Multi-process program built with glibc: `fork`, `pipe`, `dup2`, `execv`, `waitpid`, `kill` |
| `gsoket` | AF_UNIX client/server built with glibc: abstract names, `fork`, `poll`, `accept4`, `epoll`, `SCM_RIGHTS` |
| `ekran` | Terminal size (`TIOCGWINSZ`) and whether stdin is a terminal (`TCGETS`) |
| `ucat` | Copies stdin or files (like `cat`) |
| `fbdemo` | Opens `/dev/fb0`, queries the screen with ioctls, maps it with `mmap` and draws pixels directly |
| `fbfare` | Mouse cursor in graphics mode (`KD_GRAPHICS`): reads `/dev/input/mice`, draws an arrow, leaves a trail with the left button; `q`/Esc (`/dev/input/event0`) quits back to text mode |
| `olaytest` | evdev test: ioctls, read errors, console modes and the type/code/value sequence of incoming events |
| `cokme` | Writes through a null pointer: only the process dies (SIGSEGV, 139) |
| `dongu` | Infinite loop without system calls: preempted by the time slice, stopped by Ctrl-C or `kill` |
| `ush` | Small `/bin/sh` for processes (`ush -c 'cmd'`, scripts): quotes, `$NAME`, `;` `&&` `||` `&`, redirections, `exec`, `cd`, `exit`, `echo` (see [Graphical interface](#graphical-interface-x11-and-jwm)) |
| `startx` | Starts Xfbdev, runs JWM (or the given command) once the socket is ready, and stops X when it ends |
| `uterm`, `ayarlar`, `oturum`, `xdeneme`, `doom` | X11 programs, see below |

To add your own program, put a `.c` file under `kullanici/` (`#include "linux.h"`,
`int main(int argc, char** argv, char** envp)`). If the image already exists, the next build updates
the programs in `/usr/bin`. The same binary also runs on Linux:

```bash
gcc -static -nostdlib -fno-pie -no-pie -ffreestanding -O2 -o merhaba kullanici/merhaba.c
```

**Architecture:**
- **Memory layout:** the kernel lives entirely in the upper half: the image at
  `0xffffffff80000000`, all physical memory in the HHDM. The lower half belongs to processes and is
  laid out as on Linux:
  - ELF segments at `p_vaddr` (`0x555555554000` + `p_vaddr` for `ET_DYN` without an interpreter);
  - the `brk` heap after the last segment;
  - `mmap` regions growing down from `0x7f0000000000`;
  - a 1 MiB stack up to `0x7ffffffff000`;
  - nothing is ever mapped below `0x10000`.
- **Address spaces:** every process has its own PML4; the kernel half is shared and inaccessible to
  user code. Code pages are read-only, data and stack are NX. SMEP and SMAP are enabled when the CPU
  supports them. User pages come from a physical frame pool separate from the kernel heap.
- **Initial stack:** `argc`, `argv`, `envp` and auxv (`AT_PHDR`, `AT_ENTRY`, `AT_PAGESZ`,
  `AT_RANDOM`, `AT_EXECFN`…) are set up per the SysV ABI, so libc startup code works.
- **Entry and switching:** system calls come in through `SYSCALL`/`SYSRET`. The GDT has kernel and
  user segments and a TSS. A task switch changes CR3, `TSS.rsp0` and the process's FS/GS base
  (`arch_prctl`, TLS).
- **Scheduling:** kernel code is cooperative; ring 3 code is preempted by the timer and yields when
  its 10 ms slice runs out. That is why the shell stays responsive while `dongu &` runs.
- **Mechanism and policy:** mechanism is in C
  ([`upp_kullanici.c`](https://github.com/ploxdev/upp-lang/blob/main/src/runtime/upp_kullanici.c)):
  GDT/TSS, the SYSCALL entry, page tables, permission-checked copies to and from user memory. Policy
  is in u++ ([`surec.upp`](kaynak/surec.upp), [`sistem_cagrisi.upp`](kaynak/sistem_cagrisi.upp),
  [`sistem_cagrisi_dosya.upp`](kaynak/sistem_cagrisi_dosya.upp),
  [`sistem_cagrisi_surec.upp`](kaynak/sistem_cagrisi_surec.upp)): the process table, file
  descriptors, working directory, umask, signal actions, resource limits, ELF loading, mmap
  placement and the meaning of each call.

User mode only exists when booted through Limine. Under Multiboot the kernel uses the lower half
(the first 1 GiB) identity-mapped, so running an ELF is refused with a clear error.

### Commands

| Area | Commands |
|---|---|
| Files | `ls` `cd` `pwd` `cat` `mkdir` `rmdir` `touch` `rm` `cp` `mv` `find` `tree` `stat` `du` `chmod` `basename` `dirname` |
| Disks | `lsblk` `blkid` `mount` (`-t`, `-o ro`) `umount` `df` `sync` |
| Devices | `devinfo` (`-v`) `pciconf` (`-l`, `-lv`, `-lb`) |
| Text | `echo` `wc` `head` `tail` `grep` `sort` `uniq` `rev` `cut` `tr` `seq` `tee` `hexdump` `hd` |
| Editors | `nano` `vi` (`vim`) |
| System | `ps` `jobs` `kill` `sleep` `uptime` `uname` `date` `free` `dmesg` `nproc` `uppfetch` `clear` `reboot` `poweroff` (`halt`, `shutdown`) |
| Shell | `sh` `source` (`.`) `exit` `env` `printenv` `export` `unset` `alias` `unalias` `which` `history` `time` `test` (`[`) `expr` `true` `false` `whoami` `id` `hostname` `help` |
| Games | `snake` |

```text
root@uppix:~# mount /dev/sdb1 /mnt/usb -o ro      # mount by hand (create the mount point with mkdir first)
root@uppix:~# cp ~/not.txt /mnt/sdc/ && sync      # copy to the exFAT disk
root@uppix:~# umount /mnt/sdb1
```

### Editors (`nano`, `vi`)

Both run full screen and share one core ([`duzenleyici.upp`](kaynak/duzenleyici.upp)): a line
buffer, a view that expands tabs to 8 columns, horizontal scrolling, a screen that redraws only the
changed lines, a key reader that decodes ANSI key sequences (arrows, Home/End, PgUp/PgDn, Delete),
and undo/redo. The screen size comes from the same source as `TIOCGWINSZ`; `LINES`/`COLUMNS`
override it. They work on the framebuffer console, the VGA screen and a serial terminal.

- **`nano [+LINE[,COLUMN]] [file]`:** GNU nano's shortcuts: `^O` write out, `^S` save, `^X` exit
  (asks `E`/`H` — yes/no — if modified), `^K` cut (consecutive cuts accumulate), `^U` paste, `^W`
  search, `^C` position, `^_` go to line, `^Z`/`M-U` undo, `M-E` redo, `^G` help.
- **`vi [+LINE] [file]`** (`vim`): normal, insert and command-line modes. Motion: `h j k l`, arrows,
  `0 ^ $`, `w b e`, `gg G`, `Ctrl-F/B/D/U`. Editing: `x X dd D dw d$ dG yy Y yw p P J r ~ cc cw C s
  S`, count prefixes (`3dd`, `5j`), `u` and `Ctrl-R`. Insert: `i a I A o O`, `Esc`. Search:
  `/ ? n N` (wraps around). Command line: `:w [file]`, `:q`, `:q!`, `:wq`, `:x`, `ZZ`, `ZQ`, `:e!`,
  `:N`, `:$`, `:s/a/b/g`, `:%s/a/b/g` (plain text). The bottom line shows the position
  (`line,byte-column`) and percentage as in vim.

`hexdump` produces the same output as util-linux's: no options (16-bit words), `-C` (canonical,
same as `hd`), `-b -c -d -o -x`, `-n` and `-s` (`0x`, octal, `k`/`m` suffixes), `-v`; repeated lines
are collapsed into `*`.

### Snake (`snake`)

`snake` is a full-screen snake game. Steer with the arrows or WASD; `p` pauses, `q` or Ctrl-C
quits, `r` restarts after a crash. Each food is worth 10 points and the snake speeds up as the score
grows. The high score is kept in `/root/.yilan_rekor`, which survives reboots because the root is
ext4. `snake -o` is an autopilot: the snake heads for the food while avoiding paths where a flood
fill shows it would get trapped; `p` pauses, `q` quits, and the arrow keys are ignored because the
snake picks its own direction. The screen is updated with ANSI sequences (`\e[row;colH`, colors,
`\e[?25l`) and only the changed cells are drawn, so the game works the same on the framebuffer
console, the VGA text screen and a serial terminal.

## Graphical interface (X11 and JWM)

```text
root@uppix:~# startx &
startx: X hazır (:0)
jwm: masaüstü hazır
```

Uppix runs X.Org's kdrive server **Xfbdev** and the **JWM** window manager as unmodified Linux
programs: both are ordinary ELFs statically linked against musl. Xfbdev draws to `/dev/fb0`
(`mmap`), reads the keyboard and mouse from `/dev/input/event0` and `event1` (evdev) and switches
the console to `KD_GRAPHICS`. Clients connect through `/tmp/.X11-unix/X0` and the abstract AF_UNIX
socket of the same name. X compiles the keyboard layout with `xkbcomp` at startup.

**Building** (Linux or WSL; `gcc`, `make`, `python3`, `curl`, `xz`, `bzip2`):

```bash
sh ui/derle.sh
```

[`ui/derle.sh`](ui/derle.sh) downloads the source archives from x.org, github.com/joewing/jwm and
musl.libc.org, verifies them against the sha256 sums pinned in the script and keeps them under
`derleme/ui/` (ignored by git). The build runs in `~/.cache/uppix-ui`, because the Windows disk is
slow from WSL (override with `UPPIX_UI_IS`). Everything is installed with `--prefix=/usr` and
`DESTDIR` into a staging root, and the result is copied to `derleme/ui/kur` (8.9 MiB). Each package
is stamped when done: an interrupted build resumes where it stopped, and if the script and its
inputs have not changed it finishes immediately. The first build can take a few tens of minutes. If
`derleme/ui/kur` exists, `disk_yap.sh` copies its contents into the image's root (updating an
existing image with `debugfs`).

| Package | Version | Options |
|---|---|---|
| musl | 1.2.5 | Through the `musl-gcc` wrapper; `-static -no-pie` |
| xorgproto, libXau, xcb-proto, libxcb, xtrans | 2018.4, 1.0.9, 1.14.1, 1.14, 1.4.0 | Docs disabled |
| libX11, libXext | 1.6.12, 1.3.4 | Loadable i18n and XF86BigFont disabled |
| pixman, libfontenc, libXfont2 | 0.38.4, 1.1.4, 2.0.4 | libXfont2 without FreeType, BDF/SNF and fontconfig: built-in fonts only |
| libxkbfile, xkbcomp, xkeyboard-config | 1.1.0, 1.4.4, 2.23.1 | XKB data in `/usr/share/X11/xkb` |
| xorg-server | 1.19.7 | The last release with Xfbdev (removed in 1.20). Only kdrive/Xfbdev; Xorg, Xwayland, Xnest, Xvfb, Xephyr, GLX, DRI, MIT-SHM, TCP and the input thread disabled; font path `built-ins` |
| JWM | 2.4.3 | Xft, Xrender, PNG, JPEG, XPM, cairo, rsvg, fribidi, Xinerama, Xmu and NLS disabled; shape enabled (rounded corners); X core font `fixed`, text converted from UTF-8 when drawn ([`ui/yama/jwm/latin1.py`](ui/yama/jwm/latin1.py)) |

**Running:** `startx` in Uppix (`startx &` to keep the shell free). [`startx`](kullanici/startx.c)
starts Xfbdev on `:0` (`-dumbSched -noreset`, evdev keyboard and mouse) and, once the socket
accepts connections, runs JWM with `DISPLAY=:0`. When JWM exits ("Oturumu kapat" — log out — in the
menu, or `jwm -exit` from the serial console) it stops X and the console returns to text mode.
`startx /usr/bin/xdeneme` runs a single client instead of JWM. The server's messages go to
`/tmp/Xfbdev.0.log`. Open QEMU with a display ([`scripts/run-gui.sh`](scripts/run-gui.sh),
[`scripts/run-gui.cmd`](scripts/run-gui.cmd), or by hand):

```bash
uppc derle .
qemu-system-x86_64 -m 512 -serial stdio -display gtk -drive file=derleme/uppix.img,format=raw
```

(`-display sdl` also works.) Commands can be typed on the serial terminal; while X is running the
keyboard and mouse belong to the QEMU window (click into it to grab the mouse, Ctrl-Alt-G releases
it).

**Desktop:** the background is a gradient, window title bars have gradients and rounded corners.
The tray at the bottom shows, from left to right, "Uppix" (the menu), "Terminal", a pager for the
two desktops, the task list, a clock (UTC) and a "Session" button. Left- or right-clicking the
desktop, the "Uppix" button or Alt-F1 opens the menu: Terminal, Settings, Doom, a test window,
desktops, reload JWM, log out, restart, shut down. Windows are moved by their title bar and resized
from their edges; right-clicking a title bar opens the window menu. Shortcuts: Ctrl-Alt-T terminal,
Ctrl-Alt-D Doom, Ctrl-Alt-Del session dialog, Alt-Tab / Alt-Shift-Tab switch windows, Alt-F2 window
menu, Alt-F4 close, Alt-F9 minimize, Alt-F10 maximize, Alt-1/Alt-2 and Ctrl-Alt-←/→ switch
desktops. Menus, shortcuts and mouse bindings live in
[`/etc/jwm/system.jwmrc`](kok/etc/jwm/system.jwmrc) (JWM 2.4 has no default mouse bindings); the
look lives in [`/etc/jwm/tema.jwmrc`](kok/etc/jwm/tema.jwmrc), which it includes.

| Program | What it does |
|---|---|
| [`uterm`](kullanici/x11/uterm.c) | Terminal emulator. Opens a PTY from `/dev/ptmx`, makes the slave side stdin in the child and runs an interactive `/bin/sh`; the kernel starts a **uppsh session** attached to that PTY, so the 81 built-in commands, pipes, history, Tab completion, `nano`/`vi`/`snake` and ring 3 programs all work in the terminal. VT100/ANSI: cursor movement, erase, insert/delete line, scroll regions, 16 colors (256 colors approximated), bold/reverse/underline, the alternate screen, cursor position reports; box-drawing lines use the font's DEC line glyphs. `TIOCSWINSZ` when the window is resized. Arrows, Home/End, PgUp/PgDn, Delete, F1-F12; in the Turkish layout ş, ğ, ı, İ are sent as UTF-8 |
| [`ayarlar`](kullanici/x11/ayarlar.c) | Settings: theme (Gece "Night", Orman "Forest", Gün batımı "Sunset", Grafit "Graphite": desktop gradient, title, tray and menu colors), keyboard layout (us / tr), clock format and system information (kernel, memory, screen, uptime). "Uygula" (Apply) writes `/etc/jwm/tema.jwmrc`, `/etc/jwm/ayarlar` and `/etc/X11/klavye`, loads the layout immediately through XKB (`XkbGetKeyboardByName`; the next `startx` also starts with `-xkb-layout`) and runs `jwm -restart` (in 2.4 `-reload` only reloads the menu). `ayarlar --uygula THEME CLOCK us\|tr` applies settings without a window |
| [`oturum`](kullanici/x11/oturum.c) | Session dialog: log out (`jwm -exit`; startx stops X and the console returns to text mode), restart and shut down (`reboot(2)`: the kernel flushes the file systems). `oturum cikis\|yeniden\|kapat` asks only about that action; Enter confirms, Esc cancels |
| [`xdeneme`](kullanici/x11/xdeneme.c) | Test window: a green window, a red rectangle, one key and one mouse event |
| [`doom`](kullanici/doom/uppix.c) | Doom (see [Doom](#doom)) |

The X programs share [`arayuz.h`](kullanici/x11/arayuz.h): TrueColor colors, gradient fills,
buttons, UTF-8 → ISO 8859-1 text and window titles (`WM_NAME` in Latin-1, `_NET_WM_NAME` in UTF-8).

**Design decisions:**
- **`/bin/sh`:** X runs `xkbcomp` and JWM runs menu commands through `/bin/sh -c`. On Uppix
  `/bin/sh` is a built-in command that goes to the in-kernel shell (`#!/yerlesik/sh`) and cannot run
  as a process. So when `execve` sees that line and `/usr/bin/ush` exists, it runs ush with the same
  argv. [`ush`](kullanici/ush.c) is a libc-free C shell of about 330 lines. The alternatives were
  busybox (a new source), embedding xkbcomp in X (patching the server) or a precompiled `.xkm` (X
  still calls xkbcomp whenever the layout changes). ush was the smallest and most Linux-faithful
  option. JWM's `StartupCommand` also uses ush to `echo` "jwm: masaüstü hazır" (desktop ready), which
  the tests wait for.
- **`startx` is a C program:** uppsh has no loops, so "wait until the socket accepts" cannot be
  written as a script.
- **Built-in fonts:** libXfont2 keeps the `fixed` and `cursor` fonts as gzipped PCF and inflates
  them with zlib. To avoid downloading another source, zlib is a stub whose inflate functions fail
  ([`ui/yama/zlib`](ui/yama/zlib)); [`gomulu_ac.py`](ui/yama/libxfont2/gomulu_ac.py) inflates the
  arrays at build time. There is also a small SHA-1 implementation for what X needs
  ([`ui/yama/libsha1`](ui/yama/libsha1)).
- **pkg-config:** [`ui/pkg-config`](ui/pkg-config) (Python) expands the `Requires.private` chain in
  dependency order for static linking and adds the staging root to the paths.
- **XKB rules:** xkeyboard-config's `rules` files are generated with xsltproc/perl. Instead, a short
  hand-written `evdev` rule ([`ui/xkb/evdev`](ui/xkb/evdev)) is used: pc105, layout from
  `-xkb-layout` (`us`, `tr`; all of xkeyboard-config's `symbols` files are in the image).
- **uppsh in the terminal:** on Uppix the shell lives in the kernel; for a terminal to be useful it
  needs the built-in commands far more than ELFs. So instead of attaching ush to the terminal, the
  kernel starts a new uppsh session bound to the PTY ([`kaynak/pty.upp`](kaynak/pty.upp)). The
  session is a separate kernel task; the shell's working directory, `$?`, `exit` and positional
  parameters are per session, while environment variables, aliases, history and jobs are shared.
  All terminal I/O of the shell and the commands (`konsol_*`) goes to the screen/serial port or the
  PTY depending on the task's session; keys come from the console's or the PTY's queue. The
  terminal of processes in the session is that PTY (`TIOCGWINSZ` gives the PTY size, no KD/VT
  ioctls); Ctrl-C interrupts the foreground command.
- **JWM text:** without Xft, JWM draws text with the X core font without conversion. A small patch
  ([`ui/yama/jwm/latin1.py`](ui/yama/jwm/latin1.py)) converts UTF-8 to ISO 8859-1 before drawing and
  measuring, so menus and titles can be written in UTF-8.

**Limits:** there is one screen and one mode (the framebuffer Limine provides, e.g. 1280x800x32);
`FBIOPUT_VSCREENINFO` only accepts the current mode. No VT switching (Ctrl-Alt-F*). The only fonts
are `fixed` (6x13) and `cursor`; since `fixed` is ISO 8859-1, ü, ö, ç look right but ş, ğ, ı, İ are
drawn as s, g, i, I in menus, titles and the terminal (UTF-8 still reaches the shell and files). The
wallpaper is a solid color or gradient only (no library for XPM/PNG). uterm has no scrollback and
no selection/clipboard; a loop that makes no system calls cannot be interrupted with Ctrl-C from the
terminal (stop it with `kill`). Programs that read the PTY slave directly get raw bytes (the line
discipline lives in the shell). `link` copies, ownership is not tracked, every process is root. X
and its clients talk over AF_UNIX (no TCP); file path sockets can only be created on in-memory file
systems (`/tmp`). Because blocking calls in the cooperative kernel poll in 1 ms steps, every X round
trip takes one timer step: JWM takes about 5 seconds to start. ush has no pipes (`|`).

### Doom

id Software's Doom runs through [doomgeneric](https://github.com/ozkl/doomgeneric) (derived from
Chocolate Doom, reducing the screen and keys to four functions; GPLv2). The source is in
[`kullanici/doom/doomgeneric`](kullanici/doom/doomgeneric) (ozkl/doomgeneric `e257a09`, only the
files that are compiled; unmodified). The Uppix backend is
[`kullanici/doom/uppix.c`](kullanici/doom/uppix.c); `ui/derle.sh` links it statically with musl and
libX11 into `/usr/bin/doom`. The game data is the shareware **DOOM1.WAD** v1.9 (episode 1, "Knee-Deep
in the Dead", 9 maps; from Debian's `doom-wad-shareware` package, MD5
`f0cefca49926d00903cf57551d901abe`). id Software allows the shareware version to be distributed for
free without modification: the license is in [`doom1.wad.lisans`](kullanici/doom/doom1.wad.lisans).
The file is placed in `/usr/share/games/doom/` in the image, where Doom looks for it. The full
version's `doom.wad` or `doom2.wad` also work when put into the same directory or passed with
`-iwad`.

- **On the desktop:** "Doom" in the menu or Ctrl-Alt-D; it opens a 640x400 window.
- **On the console:** when X is not running, `doom` starts full screen (`/dev/fb0`, scaled 2x at
  1280x800). The keyboard is read from `/dev/input/event0` and does not reach the shell while the
  game runs. On exit the console returns to text mode.
- `-fb` / `-x11` force a mode; Doom's own options pass through (`-warp 1 3`, `-skill 4`,
  `-timedemo demo1` …). Settings (`default.cfg`) and saves are written to `$HOME` (or `/root`).

Keys: arrows walk/turn, Ctrl fires, space opens doors, Shift runs, Alt with the arrows strafes, `,`
`.` step sideways, 1-7 weapons, Tab map, Esc menu, F2/F3 save/load. Keys are read by their position
on the keyboard (keycode in X, evdev code on the console), so they work the same in the `tr` layout.
There is no sound (Uppix has no audio device).

## Source layout

Source files and identifiers use Turkish names; the table gives the English meaning.

| File | Contents |
|---|---|
| `kaynak/ana.upp` | Entry point ("main") |
| `kaynak/uppix.upp` | Boot: kernel log, disk scan, mounting the root, virtual file systems, `/etc`, the command table, the session |
| `kaynak/vfs.upp` | VFS: node cache (`sinif Dugum`, "class Node"), mount table, driver interface (`sinif Surucu`, "class Driver"), path resolution, `/proc` |
| `kaynak/veriyolu.upp` | Device tree and driver framework ("bus", FreeBSD newbus): devices, driver registration (`DRIVER_MODULE`), probe/attach/detach, `nexus`, PCI bus scan and BARs, `hostb`/`isab`, MMIO (bus_space) and DMA (bus_dma) resource functions |
| `kaynak/blok.upp` | Block devices ("block"): the disk registry (drivers' read/write function values), MBR/GPT partitions, file system detection |
| `kaynak/ata.upp` | ATA drivers: the PCI IDE controller (`atapci`) and its channels (`ata`); they put disks into the disk registry |
| `kaynak/ahci.upp` | AHCI (SATA) drivers: controller (`ahci`) and ports (`ahcich`); IDENTIFY, READ/WRITE DMA EXT, FLUSH CACHE EXT, polled command completion |
| `kaynak/girdi.upp` | Input ("input"): `atkbdc`/`atkbd`/`psm` drivers, IntelliMouse detection and packet decoding, scan code → `KEY_*`, per-open event queues (evdev, mousedev), console modes (KD/K/VT) |
| `kaynak/ext4.upp`, `fat.upp`, `exfat.upp` | ext4, FAT32 and exFAT drivers |
| `kaynak/kabuk.upp` | `uppsh` ("shell"): line splitting, tokens and expansions, pipes and redirections, and-or lists, background jobs, scripts |
| `kaynak/satir.upp` | Line editor ("line"), history, Tab completion, Ctrl-C interruption |
| `kaynak/cikti.upp` | Per-task output capture ("output", for pipes and redirections) |
| `kaynak/pty.upp` | Pseudo-terminals (`/dev/ptmx`, `/dev/pts/N`) and shell sessions: per-session working directory, `$?`, `exit`; the console I/O layer (`konsol_yaz`, `konsol_tus_bekle`…), a uppsh session bound to a PTY |
| `kaynak/komut_*.upp` | Commands ("command"); each is `fonk(liste[metin] a, metin girdi) -> sayi` (`komut_dosya` files, `komut_disk` disks, `komut_metin` text, `komut_sistem` system, `komut_nano`, `komut_vi`, `komut_hexdump`, `komut_aygit` devices: `devinfo`, `pciconf`, `komut_oyun` games: `snake`) |
| `kaynak/duzenleyici.upp` | The editor core shared by `nano` and `vi` ("editor") |
| `kaynak/surec.upp` | User processes ("process"): process table, file descriptors, ELF64 loader, SysV initial stack, launching from the shell |
| `kaynak/sistem_cagrisi.upp` | The Linux x86_64 system call ("sistem çağrısı") dispatcher; I/O and memory (read, write, open, stat, lseek, ioctl, mmap, brk…) |
| `kaynak/sistem_cagrisi_dosya.upp` | File system and descriptor calls (pread/pwrite, readv/writev, dup, fcntl, getdents64, access, getcwd/chdir, mkdir, unlink, rename, readlink) |
| `kaynak/sistem_cagrisi_surec.upp` | Process, time, TLS and signal calls (fork/clone, execve, wait4/waitid, kill, arch_prctl, uname, clock_*, getrandom, mprotect, rt_sig*, prlimit64, futex…) |
| `kaynak/sistem_cagrisi_soket.upp` | AF_UNIX sockets ("socket"): socket objects, addresses (path, abstract, unnamed), stream ring and datagram queue, `SCM_RIGHTS`, options, `socket`…`recvmsg` |
| `kaynak/sistem_cagrisi_hazir.upp` | The readiness model ("ready", `fd_hazir`) and `poll`/`select`/`epoll` on top of it; `eventfd`, `timerfd`, `setitimer`/`alarm`/`pause`; the terminal line discipline |
| `kaynak/sistem_cagrisi_girdi.upp` | Reads/writes/ioctls on `/dev/input/*`, console ioctls, `K_RAW`/`K_MEDIUMRAW` reads |
| `kaynak/yardimci.upp` | Helpers ("helper"): sorting, formatting, glob matching, calendar/Unix time |
| `kullanici/*.c` | Ring 3 programs (libc-free static Linux ELFs); `linux.h` is the shared header; `kullanici/glibc/*.c` static with glibc; `kullanici/x11/*.c` with libX11 (musl, `ui/derle.sh`); `kullanici/doom` doomgeneric, the Uppix backend and DOOM1.WAD |
| `kok/` | Files of the root file system ("root"): `/etc`, JWM configuration… |
| `ui/` | Graphical interface: `derle.sh` (X.Org Xfbdev, xkbcomp, JWM, Doom), `pkg-config`, `yama/` (patches: zlib stub, libsha1, inflating the built-in fonts), `xkb/evdev` |
| `onyukleyici/` | Bootloader (Limine) |
| `derleyici/` | The pinned U++ compiler ("compiler") |
| `scripts/` | QEMU launchers with a display (`run-gui.sh`, `run-gui.cmd`) |
| `disk_yap.sh` | Builds the disk images ("make disk") |
| `upp.proje` | U++ project file: entry point, kernel target, bootloader, disks |

Every file system driver is a `sinif Surucu` made of function values: `bagla` (mount), `yukle`
(load a directory into the cache), `oku` (read), `yaz` (write), `olustur` (create), `sil` (delete),
`tasi` (move), `oznitelik` (attributes), `alan` (space). The VFS knows which node belongs to which
mount and calls the right driver. Directories are loaded on first access. Commands know nothing
about file systems; a cross-disk copy such as `cp /root/a.txt /mnt/sdc/` uses the same code.

Binary data is handled with `upp.tampon.*` ("buffer"): bounds-checked byte buffers with
little-endian reads/writes, UTF-16 and CRC32C. The block layer reaches disks through the disk
registry; the ATA driver registers the kernel's `upp.cekirdek.disk_oku/disk_yaz` PIO mechanism, the
AHCI driver its own DMA read/write (in u++, using MMIO windows and `upp.cekirdek.tampon_fiziksel`).
Drivers read PCI configuration space with `upp.cekirdek.pci_oku`; configuration writes and mapping
BAR windows (`pci_yaz`, `mmio_ac`) require `guvensiz` ("unsafe") and are done in only two places in
`veriyolu.upp`; accesses inside a window (`mmio_oku/yaz`) are bounds-checked. See the
[U++ API documentation](https://github.com/ploxdev/upp-lang/blob/main/docs/API_en.md) for details.

## Tests

The tests boot Uppix in QEMU, send commands over the serial port and check the output and (when
needed) the disk images with Linux tools:

```bash
python3 tests/run_tests.py --uppc derleyici/uppc            # all
python3 tests/run_tests.py --uppc derleyici/uppc --dosya tests/cekirdek/06_uppix_kullanici.upp
```

| Test | What it does |
|---|---|
| `01_uppix.upp` | Boot without disks (RAM root); 25 shell commands |
| `02_uppix_disk.upp` | Builds the images from scratch, writes to all three file systems, then checks them with Linux tools: `e2fsck -fn`, `debugfs`, `fsck.fat`, `mtype`, `fsck.exfat` |
| `03_uppix_kalici.upp` | Second boot with the same images: is everything written still there ("persistent") |
| `04_uppix_limine.upp` | Builds `uppix.img` and boots it in 64-bit mode through Limine; writes to `/boot` and the root, then checks both partitions with `e2fsck`, `fsck.fat` and `mtype` |
| `05_uppix_snake.upp` | Plays `snake -o -n 200` on autopilot without delays; high score file, refusal under pipes/redirection and option errors |
| `06_uppix_kullanici.upp` | Ring 3 ("user"): argv/exit status, `sctest`, `sctest2`, glibc-built `gmerhaba`, terminal ioctl, pipes, SIGSEGV, a loop preempted by the time slice and `kill`; three programs' output is compared with real Linux using `diff`, then the ext4 root is checked with `e2fsck` |
| `07_uppix_araclar.upp` | `hexdump`/`hd` formats; `nano` (typing, cut/paste, search, the save prompt) and `vi` (insert, `yy p`, `dd`, `:%s`, refusal of `:q`, `:wq`) sessions driven by keys over the serial port ("tools") |
| `08_uppix_disk_kaynak.upp` | An ext4 file allocated with `fallocate` on Linux (unwritten extents) reads as zeros and frees its blocks when deleted; repeated FAT32 allocation and remounting; then `e2fsck` and `fsck.fat` |
| `09_uppix_veriyolu.upp` | Device tree on QEMU i440fx ("bus"): `pciconf -l`/`-lv`/`-lb` show the host bridge (8086:1237) and the PIIX3 IDE (`atapci0`), `devinfo` shows `nexus0 → pci0 → atapci0 → ata0/ata1`; disks enter the registry through the `ata` driver and ext4/FAT32 work as before, then `e2fsck` and `debugfs` |
| `10_uppix_ahci.upp` | Three disks on ICH9 AHCI ports (`--disk-arayuz ahci`): Limine boots from the SATA disk, the ext4 root is mounted from SATA; `pciconf`, `devinfo` (`ahci0`, `ahcich0..5`) and `lsblk`; writes to ext4 (including a large file split into several DMA transfers), `/boot`, FAT32 and exFAT, a ring 3 program is loaded from SATA; then `e2fsck -fn`, `debugfs`, `fsck.fat -n`, `mtype`, `fsck.exfat -n` |
| `11_uppix_surecler.upp` | Multiple processes ("processes"): `sctest3` (fork, wait4, waitid, kill, pipes, dup2 + execve, `#!`, O_CLOEXEC, error codes) and glibc-built `gsurec`; both outputs compared with real Linux using `diff`; children's output goes into pipes and `>`; then `e2fsck` |
| `12_uppix_fare.upp` | Input and graphics console ("mouse", `--qemu-izleyici`): `atkbdc0`/`atkbd0`/`psm0` in `devinfo`; `olaytest` checks evdev ioctls and the sequence of `mouse_move`/`mouse_button`/`sendkey` events from the QEMU monitor; `fbfare` cursor and trail (`screendump` in the monitor, pixels checked in `@denetle`); after a process exits in graphics mode the console returns to `KD_TEXT`/`K_XLATE` |
| `13_uppix_soket.upp` | AF_UNIX sockets and readiness calls ("socket"): `sctest4` (including a client/server over fork) and glibc-built `gsoket`; both outputs compared with real Linux using `diff`; then `e2fsck` |
| `14_uppix_x11.upp` | Graphical interface (`--qemu-izleyici`, `@sure: 400`): `@hazirla` runs `ui/derle.sh` (fast from the cache); `startx`, JWM's ready line, the `xdeneme` window and `sendkey`/`mouse_move`/`mouse_button` from the monitor; output of commands typed into `uterm` (`echo`, `ls /dev/pts`, `pwd`) coming from the uppsh session on the PTY (`--dokum`); the root menu; theme, clock and `tr` layout through `ayarlar --uygula`; the `oturum cikis` confirmation, a second `startx`, text mode; finally `reboot(2)` through `oturum kapat`. `screendump`s check pixels of the window, desktop gradient, tray, terminal, menu and new theme; then `e2fsck` |
| `15_uppix_doom.upp` | Doom (`--qemu-izleyici`, `@sure: 600`): `doom -fb` on the console scaled 2x to 1280x800; Esc, Up, Enter, y from the monitor quit through the menu and return to text mode; after `startx`, `doom -timedemo demo1` plays the demo in an X window and prints the time. `screendump`s check a Doom frame and text mode; then `e2fsck` |

All test files are in `tests/cekirdek/` ("kernel").

## Limits

- ext4 writes do not use the journal. A file is kept in at most 4 extents, so a large file cannot
  be written if free space is badly fragmented. Entries are not added to large htree-indexed
  directories. Groups with uninitialized bitmaps (`*_UNINIT`) are skipped when allocating.
- FAT12/FAT16 and sector sizes other than 512 bytes are not supported. There is no block cache:
  drivers read a file as a whole and only the contents of the last file read stay in memory
  ([`vfs.upp`](kaynak/vfs.upp), `fs_oku_tampon`; `read(2)` copies its window from there). Every
  other operation goes to disk.
- Disk drivers are ATA PIO and AHCI. AHCI uses a single command slot (no NCQ), polls instead of
  using interrupts, and supports neither hot plugging nor ATAPI (SATA CD) devices. No NVMe or
  virtio-blk.
- PCI-PCI bridges do not appear as separate buses (`pcib`) in the device tree: all buses are
  scanned flat under `pci0`. No interrupts (INTx/MSI) are attached to drivers and there are no
  windows for I/O BARs. On the Multiboot path a BAR's MMIO window can only be mapped within the
  first 1 GiB (with Limine at any address the HHDM covers, which includes QEMU's BARs below 4 GiB),
  so AHCI only works with Limine.
- Kernel code is cooperative; only ring 3 code is preempted. Built-in commands run inside the
  kernel.
- User processes are single-threaded: `clone` only works fork-style, there is no
  `CLONE_VM`/`CLONE_THREAD` (pthreads, `posix_spawn`, glibc `system()`/`popen()`). Signal handlers
  are not invoked and there is no stopping (SIGSTOP/SIGTSTP): a caught signal also ends the process.
  No dynamic linking (`PT_INTERP`), symlinks or `rseq`; static programs (libc-free or glibc) work.
- `fork` copies pages immediately (no copy-on-write). `MAP_SHARED` file mappings become private
  copies in the child: parent and child do not see each other's writes, and each writes its own
  back to the file on `msync`/`munmap`/exit. Descriptors duplicated by `dup` or `fork` do not share
  the file position; stdin coming from the shell (pipe, `<`) is copied to the child. Every child
  uses a kernel task: at most 32 tasks at once (including the shell and background jobs), after
  which `fork` returns `EAGAIN`.
- A signal from another process ends a fork child immediately; a process running on the shell's
  task (one started from the shell) ends at its next system call or while waiting: a loop with no
  system calls only stops with Ctrl-C or the shell's `kill`. `rusage` and process times are not
  tracked. The argument/environment total in `execve` cannot exceed 256 KiB. The stack is 1 MiB and
  does not grow on page faults.
- Sockets are `AF_UNIX` only (no `AF_INET`/`AF_INET6`, no `SOCK_SEQPACKET`). File path sockets can
  only be created on in-memory file systems (tmpfs such as `/tmp`), `EOPNOTSUPP` on disk. No
  `MSG_OOB`; `SO_SNDBUF`/`SO_RCVBUF` are stored but the stream buffer is a fixed 256 KiB and the
  datagram queue holds 64 messages. A socket descriptor passed over itself (a cycle) is not
  collected. `epoll` registrations are tied to the (process, fd) pair: if a child uses the same
  instance after `fork`, it watches the parent's descriptors. `EPOLLEXCLUSIVE` is ignored. Blocking
  calls poll in 1 ms steps; CPU time is not tracked, so `ITIMER_VIRTUAL`/`ITIMER_PROF` never expire.
  The terminal's `termios` (`TCSETS`) is stored but the line discipline stays canonical (raw mode is
  available through `KDSKBMODE`).
- Input: a single virtual console (no VT switching); no keyboard LEDs or autorepeat settings, events
  cannot be written to evdev (no `uinput`). No USB mouse/keyboard (PS/2 only).
- The framebuffer must be below 4 GiB and RGB; otherwise output only goes to the serial port. No
  SMP: only the boot processor runs.
- `uppc` does not run inside Uppix yet, because it needs a compiler for its C output.

## License

Apache License 2.0, see [`LICENSE`](LICENSE). Vendored third-party code keeps its own license:
Limine (BSD-2-Clause, [`onyukleyici/limine`](onyukleyici/limine)), doomgeneric (GPLv2,
[`kullanici/doom/doomgeneric/LICENSE`](kullanici/doom/doomgeneric/LICENSE)) and the shareware
DOOM1.WAD ([`kullanici/doom/doom1.wad.lisans`](kullanici/doom/doom1.wad.lisans)).

---

## Türkçe

Uppix, tamamen **u++** ile yazılmış, Unix benzeri küçük bir işletim sistemidir. x86_64 makinede [Limine](onyukleyici/limine) ile diskten **64-bit long mode**'da açılır (BIOS ya da UEFI) ve libc kullanmaz. **Kök dosya sistemi ext4 bir diskte durur** ve yapılan değişiklikler kalıcıdır. USB bellek gibi **FAT32** ve **exFAT** diskleri tanır, okur ve yazar. Kabuk, aygıt sürücüleri, dosya sistemi sürücüleri ve 81 komutun (`nano` ve `vi` düzenleyicileri ve bir yılan oyunu dahil) hepsi u++ kaynağıdır. Altta yalnızca u++'ın serbest (freestanding) çalışma zamanı vardır: kesmeler, zamanlayıcı, klavye/seri girdi, PS/2 fare ve ham klavye kuyrukları, ATA PIO disk erişimi, PCI yapılandırma alanı ve MMIO pencereleri, bellek ayırıcı ve görev geçişi.

Uppix **kullanıcı programlarını ring 3'te** çalıştırır. Sistem çağrıları **Linux x86_64 ile birebir uyumludur**: Linux için statik derlenmiş bir ELF (libc'siz ya da **glibc ile `gcc -static`**), Uppix'te değiştirilmeden çalışır ve aynı çıktıyı verir.

```text
root@uppix:~# lsblk
AD      BOYUT TÜR   FSTÜR  ETİKET           BAĞLAMA
sda      192M disk
├─sda1    64M part  vfat   UPPIXBOOT        /boot
└─sda2   127M part  ext4   uppix-kok        /
sdb       64M disk
└─sdb1    63M part  vfat   UPPIXUSB         /mnt/sdb1
sdc       32M disk  exfat                   /mnt/sdc
root@uppix:~# cat "/mnt/sdb1/Türkçe uzun dosya adı.txt"
Uzun ve Türkçe bir dosya adı: şğüöçı ŞĞÜÖÇİ
```

### Çalıştırma

Uppix, [U++](https://github.com/ploxdev/upp-lang) diliyle yazılmıştır. Derleyici bu depoda tek bir C dosyası olarak durur ([`derleyici/`](derleyici)); önce onu kurun:

```bash
sh derleyici/kur.sh          # Linux / WSL  → derleyici/uppc
derleyici\kur.cmd           # Windows (MinGW-w64) → derleyici\uppc.exe
```

Aşağıdaki `uppc`, `derleyici/uppc` (Windows'ta `derleyici\uppc.exe`) demektir.

Gerekenler: `gcc`, `ld` ve `qemu-system-x86_64` (Windows'ta WSL içinde de olur). Disk imajı için `mkfs.ext4` (e2fsprogs), `sfdisk` (fdisk), `mkfs.fat` ve `mtools` gerekir; exFAT örnek diski için `mkfs.exfat` (`sudo apt install e2fsprogs dosfstools mtools exfatprogs fdisk`).

```bash
uppc calistir .
```

Bu komut çekirdeği derler, [`disk_yap.sh`](disk_yap.sh) ile önyüklenebilir `uppix.img` imajını hazırlar (`upp.proje` içindeki `onyukleyici = limine` ve `disk_hazirla`) ve QEMU'yu üç diskle ve seri konsolla başlatır. QEMU, Limine'i diskin MBR'sinden yükler; Limine de çekirdeği FAT32 bölümünden okuyup 64-bit kipte başlatır. Komutları terminale yazarsınız. İmajlar `derleme/` altında kalır: sonraki derlemelerde yalnızca `/boot/uppix.elf` güncellenir, kökte yaptığınız değişiklikler yerinde kalır. Sıfırdan başlamak için `derleme/*.img` dosyalarını silin.

| İmaj | İçerik |
|---|---|
| `uppix.img` | 192 MiB, MBR. **1. bölüm** 64 MiB FAT32 `UPPIXBOOT` (ESP): `/limine/limine-bios.sys`, `/limine/limine.conf`, `/EFI/BOOT/BOOTX64.EFI`, `/uppix.elf`; Uppix'te `/boot` olarak bağlanır. **2. bölüm** 127 MiB ext4 `uppix-kok`: kök (`/`), içeriği [`kok/`](kok) ağacından gelir |
| `usb-fat32.img` | 64 MiB, MBR bölüm tablosu ve 1. bölümde FAT32 (`UPPIXUSB`), örnek dosyalar uzun Türkçe adlarla |
| `usb-exfat.img` | 32 MiB exFAT (bölümsüz) |

Klavye ve ekranla bir pencerede açmak için (Limine menüsü, ardından çerçeve tamponu konsolu; X ve Doom için):

```bash
uppc derle .
sh scripts/run-gui.sh        # Linux
scripts\run-gui.cmd          # Windows: QEMU WSL içinde çalışır, pencere WSLg ile gelir
```

Diskleri IDE yerine bir AHCI (SATA) denetleyicisine takmak için `upp.proje`'ye `disk_arayuz = ahci` ekleyin ya da `uppc`'ye `--disk-arayuz ahci` verin (en çok 6 disk; QEMU'da `-device ahci`, diskler `ide-hd,bus=ahci0.N`). Elle:

```bash
qemu-system-x86_64 -m 256 -serial stdio -device ahci,id=ahci0 \
    -drive file=derleme/uppix.img,format=raw,if=none,id=d0 -device ide-hd,drive=d0,bus=ahci0.0,bootindex=0 \
    -drive file=derleme/usb-fat32.img,format=raw,if=none,id=d1 -device ide-hd,drive=d1,bus=ahci0.1
```

UEFI ile açmak için QEMU'ya OVMF verin, örneğin `-bios /usr/share/qemu/OVMF.fd`. Aynı imaj gerçek bir USB belleğe de yazılabilir (`dd if=uppix.img of=/dev/sdX`).

Eski Multiboot yolu da durur: `--onyukleyici multiboot` (varsayılan) ile çekirdek 32-bit `uppix32.elf` olarak üretilir ve QEMU `-kernel` ile açılır; `sh disk_yap.sh` (çekirdek vermeden) bu yol için ayrı `uppix-kok.img` ext4 imajını üretir. Disk verilmezse Uppix RAM'deki bir kök dosya sistemiyle (rootfs) açılır. `poweroff` dosya sistemlerini boşaltıp makineyi kapatır, `reboot` yeniden başlatır.

### Neler var

- **Dosya sistemleri:**
  - **ext4:** okuma ve yazma. Extent ağaçları, eski ext2/3 blok haritaları, htree dizinleri (okuma) ve `metadata_csum` desteklenir: inode, dizin bloğu, bitmap, grup tanımlayıcı ve süperblok sağlamaları CRC32C ile güncellenir. Desteklenmeyen bir özellik varsa disk salt okunur bağlanır.
  - **FAT32:** okuma ve yazma. VFAT uzun dosya adları (UTF-16) okunur ve yazılır, `AD~1.UZN` kısa adları üretilir, tüm FAT kopyaları birlikte güncellenir.
  - **exFAT:** okuma ve yazma. Ayırma bitmap'i, FAT zinciri ve bitişik (NoFatChain) kümeler, girdi kümesi sağlaması, büyük harf tablosuna göre ad karması ve birim etiketi desteklenir.
  - **Bellekte:** `/proc` (`cpuinfo`, `meminfo`, `mounts`, `partitions`, `uptime`, `version`, `dmesg`), `/dev` (`null`, `console`, `tty`, `tty0`, `tty1`, `random`, `fb0`, `input/event0`, `input/event1`, `input/mice`, `ptmx`, `pts/N`, `sda`…; `st_rdev` Linux'taki numaralardır), `/tmp` ve `/mnt` (tmpfs), `/bin` (yerleşik komutlar).
- **Aygıt ağacı ve sürücü çatısı:** FreeBSD newbus tasarımı. Kökte `nexus0`, altında `pci0`: PCI veri yolu 0..255 taranır (çok işlevli aygıtlar dahil), her aygıtın kimliği, sınıfı ve BAR'ları (taban ve boyut) çocuğun değişkenlerine yazılır, sonra sürücüler yoklanır; FreeBSD gibi en yüksek öncelikli sürücü kazanır. Sürücüler `hostb`, `isab`, `atapci` (PIIX3/PIIX4 IDE), `ata` (kanallar), `ahci` (SATA denetleyicisi) ve `ahcich` (kapılar); PCI'da IDE denetleyicisi yoksa ATA kanalları kökte eski ISA aygıtları olarak aranır. Sürücüler için BAR'ın MMIO penceresini açma, bellek alanı ve bus master açma ve hizalı DMA tamponları (fiziksel adresiyle) hazırdır. `devinfo` ağacı, `pciconf -l` PCI aygıtlarını FreeBSD biçiminde gösterir.
- **Diskler:** disk sürücüleri bulduğu diskleri ortak bir disk kaydına koyar; blok katmanı yalnızca kayıttaki okuma/yazma fonksiyonlarını kullanır. ATA PIO ile en çok 4 disk, AHCI (SATA) ile her kapıda bir disk; adlar bulunma sırasıyla `sda`, `sdb`…; MBR ve GPT bölüm tabloları (`sdb1`…). Açılışta `uppix-kok` etiketli (yoksa ilk) ext4 kök olarak bağlanır, `UPPIXBOOT` etiketli FAT32 bölümü `/boot`'a, diğer dosya sistemleri `/mnt/<aygıt>` altına bağlanır.
- **Kabuk (`uppsh`):** boru (`|`), yönlendirme (`>`, `>>`, `<`, `2>`), `;`, `&&`, `||` ve arka plan işleri (`&`, `jobs`, `kill %1`). Ayrıca `$AD`, `${AD}`, `$?`, `$$`, `$1…$9`, `$#`, `$@`, `~`, joker (`*`, `?`), tek/çift tırnak, takma adlar, betikler (`sh dosya`, `source`, `PATH` üzerinden çalıştırılabilir dosyalar) ve `/etc/profile`.
- **Satır düzenleyici:** ↑/↓ geçmiş, ←/→, Home/End, Delete, Tab ile komut ve dosya adı tamamlama, Ctrl-A/E/U/K/W/L/C/D.
- **Görevler:** işbirlikçi çoklu görev (en çok 32 görev, her biri 64 KiB yığınla). Görevler `sleep` sırasında ve girdi beklenirken sıra verir. `ps`, `kill` ve `jobs` ile yönetilirler.
- **Önyükleme:** [Limine](onyukleyici/limine) v11.4.1 depoya gömülüdür (BIOS aşamaları, `BOOTX64.EFI` ve `bios-install` aracının kaynağı). Çekirdek, `0xffffffff80000000`'da duran bir ELF64'tür. Limine protokolünün istekleriyle bellek haritasını, çerçeve tamponunu, HHDM'yi ve çekirdeğin fiziksel adresini alır. Sonra kendi sayfa tablolarını kurar: çekirdek kendi yüksek adresinde 4 KiB sayfalarla, tüm fiziksel bellek de HHDM'de 2 MiB sayfalarla eşlenir (çerçeve tamponu PAT ile write-combining). Alt yarı boş kalır, kullanıcı süreçlerinindir. GDT/TSS, `SYSCALL` ve NX/SMEP/SMAP kurulur. 8259 PIC kesmeleri doğrudan gelsin diye yerel APIC kapatılır.
- **Ekran:** Limine ekranı grafik kipte bırakır. Uppix bu çerçeve tamponuna kendi metin konsolunu çizer: [Terminus](https://github.com/ploxdev/upp-lang/blob/main/src/runtime/TERMINUS_LISANS.txt) 8x16 yazı tipi (Türkçe karakterler dahil), ANSI renkleri (ön ve arka plan, ters video), imleç hareketleri, kutu çizgileri, kaydırma ve alt çizgi imleç. Satır sarma VT100'deki gibi ertelenir: sağ alt hücreye yazmak ekranı kaydırmaz, bu yüzden tam ekran programlar son satırı da kullanabilir. Multiboot ile açılınca VGA metin ekranı kullanılır.
- **AHCI (SATA):** `ahci` sürücüsü ABAR'ı (BAR5) MMIO olarak açar, bellek alanını ve bus master'ı etkinleştirir, denetleyiciyi sıfırlayıp AHCI kipine alır ve uygulanan her kapı için bir `ahcich` ekler. İmzası SATA disk olan kapıda disk IDENTIFY ile tanınır (model, LBA48 sektör sayısı). Okuma/yazma READ/WRITE DMA EXT ile yapılır, veri doğrudan çağıranın tamponuna aktarılır (PRDT tamponun fiziksel adresini gösterir); yazmadan sonra FLUSH CACHE EXT gelir. Komut listesi, alınan FIS ve komut tablosu hizalı DMA belleğindedir. Kesme kullanılmaz: komut bitişi zaman aşımıyla yoklanır, hata (`PxTFD.ERR`, `PxIS.TFES`) okumayı/yazmayı başarısız kılar ve kapı yeniden başlatılır.
- **Girdi aygıtları ve grafik konsol:** 8042 denetleyicisi (`atkbdc0`) altında `atkbd0` ve `psm0`. Fare IntelliMouse algılamasıyla (200/100/80 örnekleme hızı dizisi, kimlik 3) 4 baytlık paketle ve tekerlekle çalışır; paketler işaret/taşma bitleriyle çözülür, senkron kaçarsa 3. bitle yeniden hizalanır. Linux uyumlu aygıtlar: `/dev/input/event0` (klavye) ve `event1` (fare) evdev'dir (24 baytlık `struct input_event`, `EV_SYN`/`EV_KEY`/`EV_REL`, küme 1 tarama kodlarından `KEY_*` kodları, 0xE0 genişletilmiş tuşlar ve Pause dahil; `EVIOCGVERSION`, `EVIOCGID`, `EVIOCGNAME`, `EVIOCGPHYS`, `EVIOCGBIT`, `EVIOCGKEY`, `EVIOCGRAB`, `EVIOCSCLOCKID`); `/dev/input/mice` mousedev'dir (PS/2 3 bayt; 200/100/80 hız dizisi yazılınca ImPS/2 4 bayt). Her açılışın kendi olay kuyruğu vardır; okumalar engelleyicidir, `O_NONBLOCK`'ta `EAGAIN` döner. Klavye aygıtı açıkken klavye ham kiptedir (tuşlar kabuğa gitmez). Konsolda `KDSETMODE` `KD_GRAPHICS` konsolun çerçeve tamponuna çizmesini durdurur; `KDSKBMODE` `K_RAW`/`K_MEDIUMRAW`'da terminalden tarama/tuş kodları okunur. Kipleri değiştiren süreç bitince konsol `KD_TEXT` ve `K_XLATE`'e döner.
- **Donanım:** Limine/Multiboot bellek haritası, CMOS saati (dosya zamanları Unix saniyesidir), CPUID, ACPI ile kapatma, 8042 ile yeniden başlatma.

### Kullanıcı kipi (ring 3) ve Linux sistem çağrıları

```text
root@uppix:~# merhaba bir iki
Merhaba, ring 3! Linux sistem çağrılarıyla konuşuyorum.
argc: 3
...
root@uppix:~# cokme; echo $?
cokme: boş göstericiye yazılıyor...
Bölümleme hatası (SIGSEGV): komut 0x0000000000401161, adres 0x0000000000000000
139
root@uppix:~# fbdemo
fb: 1280x800, 32 bpp, satır 5120 bayt, uppixfb
fbdemo: çizildi (sağ üst köşe)
```

Kabuk, çalıştırılabilir bir ELF dosyasını (`/usr/bin`'de ya da yol vererek) ayrı bir adres alanında, ring 3'te çalıştırır. Program yalnızca sistem çağrılarıyla konuşur; çekirdeği bozamaz. Bir istisna (boş gösterici, geçersiz komut, sıfıra bölme) yalnızca o süreci Linux'taki sinyaller gibi sonlandırır: çıkış kodu 128 + sinyaldir, kabuk çalışmaya devam eder.

**Çoklu süreç:** `fork` adres alanını kopyalar (sahipli her sayfa yeni bir çerçeveye; çerçeve tamponu paylaşılır) ve çocuğu ebeveynin sistem çağrısı anındaki yazmaçlarıyla kendi çekirdek görevinde başlatır; çocukta `fork` 0 döner. Çocuğa tanımlayıcılar (boru sayaçlarıyla), çalışma dizini, umask, sinyal eylemleri ve maskesi, kaynak sınırları ve paylaşımlı eşleme kayıtları geçer; çocuğun çıktısı ebeveyninkiyle aynı boruya ya da yönlendirmeye gider. `execve` ELF'i ya da `#!` betiğini yeni bir adres alanına yükler; yükleme başarısızsa eski görüntü sürer. Süreç tablosu gerçek ebeveyn PID'ini tutar: biten süreç ebeveyni `wait4` ile toplayana dek zombi kalır, ebeveyni biten çocuklar kabuğa (1) geçer, kabuk kendi çocuklarını kendisi toplar. Borular 64 KiB'lık çekirdek nesneleridir.

**Sistem çağrıları:** Numaralar, bayraklar, errno değerleri ve yapı düzenleri Linux x86_64'le aynıdır. Çağrı `syscall` komutuyla yapılır: rax numara; rdi, rsi, rdx, r10, r8, r9 argüman; hata dönüşü `-errno`.

| No | Çağrı | Uppix'te |
|---|---|---|
| 0, 1 | `read`, `write` | Dosya (VFS), terminal (kanonik satır: yankı, geri silme, Ctrl-D ile EOF, Ctrl-C ile SIGINT), kabuktan gelen girdi (boru, `<`); stdout borulara ve `>`'a yakalanır, stderr doğrudan terminale gider |
| 17, 18 | `pread64`, `pwrite64` | Ofsetten okuma/yazma, dosya konumu değişmez; terminalde `ESPIPE`, dizinde `EISDIR` |
| 19, 20 | `readv`, `writev` | `struct iovec` dizisi (en çok 1024) |
| 8 | `lseek` | `SEEK_SET/CUR/END`; terminalde ve boruda `ESPIPE`; dizinde `getdents64` konumunu sıfırlar |
| 16 | `ioctl` | `TIOCGWINSZ`/`TIOCSWINSZ` (terminal boyutu; PTY'de saklanır), `TIOCGPTN`, `TIOCSPTLCK` (PTY), `TCGETS`/`TCSETS`/`TCSETSW`/`TCSETSF` (`termios` saklanır ve geri okunur, kuran süreç bitince varsayılana döner; satır düzeni kanonik kalır), `TIOCSCTTY`, `TIOCNOTTY`, `TIOCGPGRP`/`TIOCSPGRP`, `TIOCGSID`, `FIONBIO`, `FIONREAD` (soket, boru, terminal, dosya); `FBIOGET_VSCREENINFO`, `FBIOGET_FSCREENINFO`, `FBIOPUT_VSCREENINFO` (yalnızca geçerli kip), `FBIOPAN_DISPLAY`, `FBIOBLANK`, `FBIOPUTCMAP`; konsolda `KDGETMODE`/`KDSETMODE`, `KDGKBMODE`/`KDSKBMODE`, `KDGKBTYPE`, `KDGETLED`/`KDSETLED`, `VT_OPENQRY`, `VT_GETSTATE`, `VT_GETMODE`/`VT_SETMODE`, `VT_ACTIVATE`/`VT_WAITACTIVE`, `VT_RELDISP`, `VT_DISALLOCATE` (tek VT); evdev'de `EVIOCG*`, `EVIOCGRAB`, `EVIOCSCLOCKID` |
| 2, 257 | `open`, `openat` | `O_RDONLY/WRONLY/RDWR`, `O_CREAT`, `O_EXCL`, `O_TRUNC`, `O_APPEND`, `O_DIRECTORY`, `O_CLOEXEC`; göreli yol `dirfd`'ye ya da sürecin çalışma dizinine göre; `/dev/null`, `/dev/tty`, `/dev/fb0`; `/dev/ptmx` her açılışta yeni PTY (en çok 16), `/dev/pts/N` yavru uç (`unlockpt`'tan önce `EIO`) |
| 3 | `close` | |
| 4, 5, 6, 262 | `stat`, `fstat`, `lstat`, `newfstatat` | 144 baytlık `struct stat`; `AT_EMPTY_PATH` |
| 32, 33, 292, 72 | `dup`, `dup2`, `dup3`, `fcntl` | `F_DUPFD(_CLOEXEC)`, `F_GETFD/SETFD` (`FD_CLOEXEC`), `F_GETFL/SETFL` (64 bitte her zaman `O_LARGEFILE`) |
| 217 | `getdents64` | `.` ve `..` dahil; `d_type` (`DT_DIR`, `DT_REG`, `DT_CHR`), 8 bayta hizalı kayıtlar |
| 21, 269, 439 | `access`, `faccessat`, `faccessat2` | `R_OK/W_OK/X_OK` (root); salt okunur dosya sisteminde `EROFS` |
| 79, 80, 81 | `getcwd`, `chdir`, `fchdir` | Her sürecin kendi çalışma dizini vardır; kabuğunkini değiştirmez |
| 83, 258 | `mkdir`, `mkdirat` | İzinlerden sürecin `umask`'ı düşülür |
| 84, 87, 263 | `rmdir`, `unlink`, `unlinkat` | `AT_REMOVEDIR`; `ENOTEMPTY`, `EISDIR`, bağlama noktasında `EBUSY` |
| 82, 264, 316 | `rename`, `renameat`, `renameat2` | Var olan dosyanın üzerine yazar; dosya/dizin uyuşmazlığında `EISDIR`/`ENOTDIR` |
| 89, 267 | `readlink`, `readlinkat` | `/proc/self/exe` çalıştırılabilir dosyanın yolunu verir; sembolik bağ yoktur (`EINVAL`) |
| 86, 265 | `link`, `linkat` | Yeni ad dosyanın içeriği ve kipiyle bir kopyadır (VFS'te aynı inode'a iki ad yoktur): "varsa `EEXIST`" ile atomik oluşturma (X'in kilit dosyası) çalışır, sonraki yazmalar paylaşılmaz; dizinde `EPERM`, `ENOENT`, `EROFS` |
| 90, 91, 268 | `chmod`, `fchmod`, `fchmodat` | İzin bitleri (`07777`) dosya sisteminin özniteliğine yazılır; soket, boru gibi tanımlayıcılarda yok sayılır |
| 92, 93, 94, 260 | `chown`, `fchown`, `lchown`, `fchownat` | Sahiplik tutulmaz: dosya varsa başarılı |
| 9, 11, 12, 26 | `mmap`, `munmap`, `brk`, `msync` | Anonim, dosyadan, `MAP_FIXED`; dosya destekli `MAP_SHARED` yazmaları `msync`, `munmap` ve süreç sonunda dosyaya yazılır (`MAP_PRIVATE` sürece özel); `/dev/fb0` `MAP_SHARED` ile çerçeve tamponunu doğrudan eşler |
| 10, 28 | `mprotect`, `madvise` | Sayfa koruması; eşlenmemiş aralıkta `ENOMEM`; `MADV_DONTNEED` sayfayı sıfırlar |
| 60, 231 | `exit`, `exit_group` | |
| 39, 186, 110, 218 | `getpid`, `gettid`, `getppid`, `set_tid_address` | `getppid` gerçek ebeveyni verir (kabuğun çalıştırdığı süreçte 1) |
| 57, 58, 56 | `fork`, `vfork`, `clone` | Adres alanının kopyası, çocuk kendi görevinde; `vfork` `fork` gibidir; `clone` yalnızca fork'a denk bayraklarla (`SIGCHLD`, `CLONE_CHILD_SETTID`, `CLONE_CHILD_CLEARTID`, `CLONE_PARENT_SETTID`, `CLONE_SETTLS`; glibc `fork()` böyle çağırır), `CLONE_VM`/`CLONE_THREAD` için `ENOSYS`; görev tablosu doluysa (32) `EAGAIN` |
| 59 | `execve` | ELF ve `#!` betikleri (yorumlayıcı + tek argüman, en çok 4 düzey); argv/envp kullanıcı belleğinden; O_CLOEXEC tanımlayıcılar kapanır, yakalanan sinyaller varsayılana döner (`SIG_IGN` kalır), süreç adı ve `/proc/self/exe` güncellenir; hata sırası Linux'taki gibi: `ENOENT`/`EACCES`, `EFAULT`, `E2BIG` (131072 baytı aşan dizgi), `ENOEXEC`. `/bin/sh` (`#!/yerlesik/sh`) süreçler için `/usr/bin/ush`'tır; etkileşimli çağrıda stdin bir PTY yavru ucuysa o PTY'de bir uppsh oturumu başlar, süreç oturumu bekleyip kabuğun çıkış koduyla biter |
| 162, 306, 74, 75 | `sync`, `syncfs`, `fsync`, `fdatasync` | `sync`/`syncfs` dosya sistemlerini boşaltır (`sync` komutu gibi); yazmalar zaten diske gittiğinden `fsync` yalnızca tanımlayıcıyı denetler |
| 169 | `reboot` | Sihirli sayılar denetlenir (`EINVAL`); `RESTART` yeniden başlatır, `POWER_OFF`/`HALT` kapatır (önce dosya sistemleri boşaltılır), `CAD_ON`/`CAD_OFF` kabul edilir |
| 61, 247 | `wait4`, `waitid` | pid -1 / > 0 / 0 / < -1, `WNOHANG`; durum Linux kodlamasıyla (çıkışta `(kod & 0xff) << 8`, sinyalde sinyal numarası); `waitid` `P_ALL`/`P_PID`/`P_PGID`, `WEXITED`, `WNOWAIT`, `siginfo` (`CLD_EXITED`/`CLD_KILLED`); `rusage` sıfırlardır; çocuk yoksa `ECHILD`; bekleme sıra vererek yapılır, ön planda Ctrl-C ile kesilir |
| 22, 293 | `pipe`, `pipe2` | 64 KiB'lık boru: okuma veri ya da EOF'a (bütün yazıcılar kapanınca) dek, yazma yer açılana dek bekler, 4096 bayta kadarki yazma bölünmez; okuyucu yoksa SIGPIPE ve `EPIPE`; `O_CLOEXEC`, `O_NONBLOCK` (`EAGAIN`); `fstat` S_IFIFO, `lseek` `ESPIPE` |
| 41, 53 | `socket`, `socketpair` | `AF_UNIX`; `SOCK_STREAM` (256 KiB'lık alma tamponu, dolunca yazan bekler) ve `SOCK_DGRAM` (ileti sınırları korunur); `SOCK_CLOEXEC`, `SOCK_NONBLOCK`; diğer aileler `EAFNOSUPPORT`, protokol `EPROTONOSUPPORT`; `fstat` S_IFSOCK, `lseek` `ESPIPE`; `read`/`write`/`readv`/`writev`/`dup`/`fork`/`fcntl` soketlerde de çalışır |
| 49, 50, 42, 43, 288 | `bind`, `listen`, `connect`, `accept`, `accept4` | Adresler: dosya yolu (`bind` bellekteki dosya sisteminde, ör. `/tmp`, bir `S_IFSOCK` düğümü oluşturur; düğüm varsa `EADDRINUSE`), soyut ad (`sun_path[0] == 0`), adsız; `addrlen` 2 ise otomatik soyut ad. `connect` bağlantıyı hemen kurar, `accept` kuyruktan alır (kuyruk doluysa bekler); `ECONNREFUSED`, `ENOENT`, `EISCONN`, `EPROTOTYPE`; datagramda varsayılan eş, `AF_UNSPEC` ile kaldırılır |
| 51, 52, 48 | `getsockname`, `getpeername`, `shutdown` | Linux'taki adres uzunlukları (adsız 2, yol 2 + uzunluk + 1, soyut 3 + uzunluk); `ENOTCONN`; `SHUT_RD`/`SHUT_WR`/`SHUT_RDWR` iki ucu da etkiler (EOF, `EPIPE`) |
| 44, 45, 46, 47 | `sendto`, `recvfrom`, `sendmsg`, `recvmsg` | `MSG_PEEK`, `MSG_DONTWAIT`, `MSG_WAITALL`, `MSG_TRUNC`, `MSG_NOSIGNAL` (yoksa `EPIPE` ile SIGPIPE), `MSG_CMSG_CLOEXEC`; `SCM_RIGHTS` ile tanımlayıcı geçirme (yer yetmezse `MSG_CTRUNC`, `read` ile alınırsa kapatılır), `SO_PASSCRED` ile `SCM_CREDENTIALS` |
| 54, 55 | `setsockopt`, `getsockopt` | `SO_REUSEADDR`, `SO_TYPE`, `SO_ERROR`, `SO_SNDBUF`/`SO_RCVBUF` (Linux gibi iki katı), `SO_KEEPALIVE`, `SO_PASSCRED`, `SO_PEERCRED` (pid/uid/gid), `SO_ACCEPTCONN`, `SO_DOMAIN`, `SO_PROTOCOL`; bilinmeyen `ENOPROTOOPT` |
| 7, 271, 23, 270 | `poll`, `ppoll`, `select`, `pselect6` | Bütün tanımlayıcı türlerinin (dosya, boru, soket, dinleyen soket, terminal, kabuk girdisi, evdev, mice, `/dev/null`, epoll, eventfd, timerfd) hazırlığı tek yerden (`fd_hazir`): `POLLIN`/`POLLOUT`/`POLLHUP`/`POLLERR`/`POLLRDHUP`/`POLLNVAL`; terminal kanonik kipte tam satırda (ya da Ctrl-D'de), `K_RAW`'da ham baytta okunabilir; zaman aşımı, kalan süre geri yazılır; ön planda Ctrl-C `EINTR` |
| 213, 291, 233, 232, 281 | `epoll_create`, `epoll_create1`, `epoll_ctl`, `epoll_wait`, `epoll_pwait` | Seviye tetiklemeli; `EPOLLET` (yalnızca yeni olay), `EPOLLONESHOT` (`EPOLL_CTL_MOD` ile yeniden kurulur); `EEXIST`, `ENOENT`, sıradan dosyada `EPERM`; kapanan (ya da numarası başka nesneye geçen) tanımlayıcının kaydı hemen düşer, aynı numara yeniden eklenebilir; epoll tanımlayıcısı da poll'lanabilir |
| 284, 290 | `eventfd`, `eventfd2` | 64 bit sayaç, `EFD_SEMAPHORE`, `EFD_NONBLOCK`, `EFD_CLOEXEC`; taşacaksa yazan bekler |
| 283, 286, 287 | `timerfd_create`, `timerfd_settime`, `timerfd_gettime` | `CLOCK_REALTIME`/`MONOTONIC`/`BOOTTIME`, `TFD_TIMER_ABSTIME`, aralıklı zamanlayıcı (okuma dolma sayısını verir); 1 ms çözünürlük |
| 38, 36, 37, 34 | `setitimer`, `getitimer`, `alarm`, `pause` | `ITIMER_REAL` dolunca SIGALRM (varsayılan eylem süreci bitirir, `SIG_IGN` yok sayar); `ITIMER_VIRTUAL`/`PROF` kabul edilir ama dolmaz; zamanlayıcılar `fork`'ta çocuğa geçmez |
| 111, 121, 124 | `getpgrp`, `getpgid`, `getsid` | Kabuğun çalıştırdığı süreç kendi grubunun başıdır; çocuklar grubu devralır |
| 112, 109 | `setsid`, `setpgid` | `setsid` yeni grup kurar (oturum grupla birdir), grup başında `EPERM`; `setpgid` kendisi ya da çocuğu için (`ESRCH`, `EINVAL`) |
| 102, 104, 107, 108 | `getuid`, `getgid`, `geteuid`, `getegid` | root (0) |
| 105, 106, 113, 114, 117, 119 | `setuid`, `setgid`, `setreuid`, `setregid`, `setresuid`, `setresgid` | Her süreç root'tur: 0'a ya da değişmeden (-1) kurmak başarılı, başka kimlik `EPERM` |
| 118, 120, 115, 116 | `getresuid`, `getresgid`, `getgroups`, `setgroups` | Kimlikler 0; ek grup yok (`setgroups` yalnızca boş listeyle) |
| 158 | `arch_prctl` | `ARCH_SET_FS/GS`, `ARCH_GET_FS/GS`: süreç başına FS/GS tabanı (TLS); çekirdek adresinde `EPERM` |
| 63 | `uname` | `Linux`, `6.1.0-uppix`, `x86_64`; düğüm adı `/etc/hostname`'den |
| 228, 229 | `clock_gettime`, `clock_getres` | `REALTIME`, `MONOTONIC`, `BOOTTIME`, `TAI`, `*_COARSE`, `*_RAW`, CPU saatleri; 1 ms çözünürlük |
| 96, 201 | `gettimeofday`, `time` | |
| 35, 230 | `nanosleep`, `clock_nanosleep` | Diğer görevlere sıra verir; `TIMER_ABSTIME`; ön planda Ctrl-C ile kesilir |
| 24 | `sched_yield` | |
| 318 | `getrandom` | `GRND_NONBLOCK/RANDOM/INSECURE` |
| 13, 14, 131 | `rt_sigaction`, `rt_sigprocmask`, `sigaltstack` | Eylemler ve maske saklanır ve geri okunur (SIGKILL/SIGSTOP maskelenemez); işleyici çağrılmaz |
| 62, 200, 234 | `kill`, `tkill`, `tgkill` | pid > 0 süreç, 0 kendi grubu, -1 kendisi dışında herkes, < -1 grup; varsayılan eylem süreci bitirir (çıkış 128 + sinyal), `SIG_IGN` ve varsayılanı yok saymak olan sinyaller etkisizdir; Ctrl-C ön plandaki grubun hepsine gider |
| 97, 160, 302 | `getrlimit`, `setrlimit`, `prlimit64` | `RLIMIT_STACK` 1 MiB, `RLIMIT_NOFILE` 32; değerler süreç başına saklanır |
| 95 | `umask` | |
| 202 | `futex` | `FUTEX_WAIT(_BITSET)`, `FUTEX_WAKE` (tek iş parçacığı: bekleme zaman aşımı ya da Ctrl-C ile biter) |
| 273, 99 | `set_robust_list`, `sysinfo` | |

Desteklenmeyen bir çağrı `-ENOSYS` döndürür ve `dmesg`'e bir kez yazılır.

**Uyumluluk testleri:** [`kullanici/sctest.c`](kullanici/sctest.c) temel çağrıları, [`kullanici/sctest2.c`](kullanici/sctest2.c) dosya sistemi (link, chmod, chown dahil), tanımlayıcı (FIONBIO), zaman, TLS ve sinyal çağrılarını, [`kullanici/sctest3.c`](kullanici/sctest3.c) fork/wait4/waitid, pipe, dup2 ile yönlendirme, execve (betik, O_CLOEXEC, ortam, hata kodları), kill, setsid/setpgid ve kimlik çağrılarını, [`kullanici/sctest4.c`](kullanici/sctest4.c) AF_UNIX soketlerini (socketpair, yol ve soyut ad, SCM_RIGHTS, SO_PEERCRED, SO_PASSCRED, shutdown, EPIPE), poll/select/epoll'u, eventfd, timerfd ve setitimer/alarm'ı hata yollarıyla birlikte sınar; çıktılarında PID, adres, inode ya da zaman yoktur. QEMU testleri aynı ikilileri Uppix'te ve gerçek Linux'ta çalıştırıp çıktıları `diff` ile karşılaştırır: 58 + 182 + 98 + 167 satırın hepsi aynıdır. Errno'lar ve denetim sırası gerçek çekirdekle karşılaştırılarak seçilmiştir (ör. `clock_nanosleep` iş parçacığı saatinde `EOPNOTSUPP`, `F_GETFL`'de `O_LARGEFILE`).

**glibc:** [`kullanici/glibc/gmerhaba.c`](kullanici/glibc/gmerhaba.c) sıradan bir C programıdır ve glibc ile `gcc -static` derlenir. glibc'nin açılışı (`arch_prctl` ile TLS, `brk`, `mmap`, `mprotect`, `prlimit64`, `getrandom`…) ve `printf`, `malloc`, `fopen`, `opendir`, `strerror`, `__thread` değişkenleri Uppix'te değiştirilmeden çalışır; çıktı Linux'takiyle aynıdır. Desteklenmeyen tek çağrı `rseq`'tir; glibc onsuz devam eder.

**Programlar** ([`kullanici/`](kullanici)): [`linux.h`](kullanici/linux.h) libc'siz ortamdır (`_start`, sistem çağrısı sarmalayıcıları, Linux yapıları). [`disk_yap.sh`](disk_yap.sh) her C dosyasını derleyip kökte `/usr/bin`'e koyar; `kullanici/glibc/*.c` dosyaları statik glibc (`libc.a`) varsa `cc -static` ile derlenir.

| Program | Ne yapar |
|---|---|
| `merhaba` | argv ve ortamı yazar; `--cik` ile 7 döner |
| `sctest` | Temel sistem çağrılarının uyumluluk testi |
| `sctest2` | Dosya sistemi, tanımlayıcı, zaman, TLS (`arch_prctl` ve `%fs`) ve sinyal çağrılarının uyumluluk testi |
| `sctest3` | Süreç çağrılarının uyumluluk testi: fork, vfork, clone, wait4, waitid, kill, pipe/pipe2, dup2 ve execve (kendisini, `merhaba`'yı ve bir `#!` betiğini çalıştırır) |
| `gmerhaba` | glibc ile `gcc -static` derlenmiş program: stdio, malloc, TLS, dizin okuma; 7 döner |
| `gsurec` | glibc ile `gcc -static` derlenmiş çoklu süreç programı: `fork`, `pipe`, `dup2`, `execv`, `waitpid`, `kill` |
| `sctest4` | Soket ve hazırlık çağrılarının uyumluluk testi: AF_UNIX, SCM_RIGHTS, poll/select/epoll, eventfd, timerfd, setitimer/alarm |
| `gsoket` | glibc ile `gcc -static` derlenmiş AF_UNIX istemci-sunucu: soyut ad, `fork`, `poll`, `accept4`, `epoll`, `SCM_RIGHTS` |
| `ekran` | Terminal boyutu (`TIOCGWINSZ`) ve stdin terminal mi (`TCGETS`) |
| `ucat` | stdin'i ya da dosyaları kopyalar (`cat` gibi) |
| `fbdemo` | `/dev/fb0`'ı açar, ioctl ile ekranı öğrenir, `mmap` ile eşleyip doğrudan piksel çizer |
| `fbfare` | Grafik kipte (`KD_GRAPHICS`) fare imleci: `/dev/input/mice`'tan okur, ok imleci çizer, sol tuşla iz bırakır; `q`/Esc (`/dev/input/event0`) ile çıkıp metin kipine döner |
| `olaytest` | evdev sınaması: ioctl'lar, okuma hataları, konsol kipleri ve gelen olayların tür/kod/değer dizisi |
| `cokme` | Boş göstericiye yazar: yalnızca süreç sonlanır (SIGSEGV, 139) |
| `dongu` | Sistem çağrısı yapmayan sonsuz döngü: zaman dilimiyle kesilir, Ctrl-C ya da `kill` ile durur |
| `ush` | Süreçler için küçük `/bin/sh` (`ush -c 'komut'`, betik): tırnaklar, `$AD`, `;` `&&` `||` `&`, yönlendirmeler, `exec`, `cd`, `exit`, `echo` (bkz. [Grafik arayüz](#grafik-arayüz-x11-ve-jwm)) |
| `startx` | Xfbdev'i başlatır, soket hazır olunca JWM'yi (ya da verilen komutu) çalıştırır, o bitince X'i durdurur |
| `xdeneme` | libX11 test istemcisi ([`kullanici/x11/xdeneme.c`](kullanici/x11/xdeneme.c), `ui/derle.sh` derler): pencere, dikdörtgen, bir tuş ve bir fare olayı |

Kendi programınızı eklemek için `kullanici/` altına bir `.c` dosyası koyun (`#include "linux.h"`, `int main(int argc, char** argv, char** envp)`). İmaj zaten varsa sonraki derleme `/usr/bin`'deki programları günceller. Aynı ikili Linux'ta da çalışır:

```bash
gcc -static -nostdlib -fno-pie -no-pie -ffreestanding -O2 -o merhaba kullanici/merhaba.c
```

**Mimari:**
- **Bellek düzeni:** Çekirdek tamamen üst yarıdadır: görüntü `0xffffffff80000000`'da, tüm fiziksel bellek HHDM'de. Alt yarı süreçlerindir ve düzen Linux'takiyle aynıdır:
  - ELF bölümleri `p_vaddr`'da (yorumlayıcısız `ET_DYN` için `0x555555554000` + `p_vaddr`);
  - `brk` yığını son bölümün ardından;
  - `mmap` bölgeleri `0x7f0000000000`'dan aşağı doğru;
  - 1 MiB yığın `0x7ffffffff000`'a kadar;
  - `0x10000` altı hiç eşlenmez.
- **Adres alanları:** Her sürecin kendi PML4'ü vardır; çekirdek yarısı paylaşılır ve kullanıcıya kapalıdır. Kod sayfaları salt okunurdur, veri ve yığın NX'tir. İşlemci destekliyorsa SMEP ve SMAP açılır. Kullanıcı sayfaları çekirdek yığınından ayrı bir fiziksel çerçeve havuzundan gelir.
- **İlk yığın:** SysV ABI'ye göre `argc`, `argv`, `envp` ve auxv (`AT_PHDR`, `AT_ENTRY`, `AT_PAGESZ`, `AT_RANDOM`, `AT_EXECFN`…) kurulur, yani libc başlangıç kodları da çalışabilir.
- **Giriş ve geçiş:** Sistem çağrıları `SYSCALL`/`SYSRET` ile gelir. GDT'de çekirdek ve kullanıcı segmentleri ve bir TSS vardır. Görev geçişi CR3'ü, `TSS.rsp0`'ı ve sürecin FS/GS tabanını (`arch_prctl`, TLS) değiştirir.
- **Zamanlama:** Çekirdek kodu işbirlikçidir; ring 3 kodu ise zamanlayıcıyla kesilir ve 10 ms'lik dilimi dolunca sırasını verir. Bu yüzden `dongu &` çalışırken kabuk yanıt vermeye devam eder.
- **Mekanizma ve politika:** Mekanizma C'dedir ([`upp_kullanici.c`](https://github.com/ploxdev/upp-lang/blob/main/src/runtime/upp_kullanici.c)): GDT/TSS, SYSCALL girişi, sayfa tabloları, kullanıcı belleğine izin denetimli kopyalama. Politika u++'tadır ([`surec.upp`](kaynak/surec.upp), [`sistem_cagrisi.upp`](kaynak/sistem_cagrisi.upp), [`sistem_cagrisi_dosya.upp`](kaynak/sistem_cagrisi_dosya.upp), [`sistem_cagrisi_surec.upp`](kaynak/sistem_cagrisi_surec.upp)): süreç tablosu, dosya tanımlayıcıları, çalışma dizini, umask, sinyal eylemleri, kaynak sınırları, ELF yükleme, mmap yerleşimi ve çağrıların anlamı.

Kullanıcı kipi yalnızca Limine ile açılışta vardır. Multiboot'ta çekirdek alt yarıyı (ilk 1 GiB) birebir kullandığından ELF çalıştırmak açık bir hatayla reddedilir.

#### Komutlar

| Alan | Komutlar |
|---|---|
| Dosya | `ls` `cd` `pwd` `cat` `mkdir` `rmdir` `touch` `rm` `cp` `mv` `find` `tree` `stat` `du` `chmod` `basename` `dirname` |
| Disk | `lsblk` `blkid` `mount` (`-t`, `-o ro`) `umount` `df` `sync` |
| Aygıt | `devinfo` (`-v`) `pciconf` (`-l`, `-lv`, `-lb`) |
| Metin | `echo` `wc` `head` `tail` `grep` `sort` `uniq` `rev` `cut` `tr` `seq` `tee` `hexdump` `hd` |
| Düzenleyici | `nano` `vi` (`vim`) |
| Sistem | `ps` `jobs` `kill` `sleep` `uptime` `uname` `date` `free` `dmesg` `nproc` `uppfetch` `clear` `reboot` `poweroff` (`halt`, `shutdown`) |
| Kabuk | `sh` `source` (`.`) `exit` `env` `printenv` `export` `unset` `alias` `unalias` `which` `history` `time` `test` (`[`) `expr` `true` `false` `whoami` `id` `hostname` `help` |
| Oyun | `snake` |

```text
root@uppix:~# mount /dev/sdb1 /mnt/usb -o ro      # elle bağlama (nokta önce mkdir ile)
root@uppix:~# cp ~/not.txt /mnt/sdc/ && sync      # exFAT diske kopyala
root@uppix:~# umount /mnt/sdb1
```

#### Düzenleyiciler (`nano`, `vi`)

İkisi de tam ekran çalışır ve aynı çekirdeği ([`duzenleyici.upp`](kaynak/duzenleyici.upp)) paylaşır: satır tamponu, sekmeleri 8 sütuna genişleten görüntü, yatay kaydırma, yalnızca değişen satırları çizen ekran, ANSI tuş dizilerini (oklar, Home/End, PgUp/PgDn, Delete) çözen tuş okuyucu ve geri alma/yineleme. Ekran boyutu `TIOCGWINSZ` ile aynı kaynaktan gelir; `LINES`/`COLUMNS` onu ezer. Çerçeve tamponu konsolunda, VGA ekranda ve seri terminalde çalışır.

- **`nano [+SATIR[,SÜTUN]] [dosya]`:** GNU nano'nun kısayolları: `^O` yaz, `^S` kaydet, `^X` çık (değiştiyse `E`/`H` sorar), `^K` kes (art arda kesimler birikir), `^U` yapıştır, `^W` ara, `^C` konum, `^_` satıra git, `^Z`/`M-U` geri al, `M-E` yinele, `^G` yardım.
- **`vi [+SATIR] [dosya]`** (`vim`): komut, ekleme ve komut satırı kipleri. Hareket: `h j k l`, oklar, `0 ^ $`, `w b e`, `gg G`, `Ctrl-F/B/D/U`. Düzenleme: `x X dd D dw d$ dG yy Y yw p P J r ~ cc cw C s S`, sayı önekleri (`3dd`, `5j`), `u` ve `Ctrl-R`. Ekleme: `i a I A o O`, `Esc`. Arama: `/ ? n N` (sona gelince sarar). Komut satırı: `:w [dosya]`, `:q`, `:q!`, `:wq`, `:x`, `ZZ`, `ZQ`, `:e!`, `:N`, `:$`, `:s/a/b/g`, `:%s/a/b/g` (düz metin). Alt satır vim'deki gibi konumu (`satır,bayt-sütun`) ve oranı gösterir.

`hexdump` util-linux'unkiyle aynı çıktıyı verir: seçeneksiz (16 bitlik sözcükler), `-C` (kanonik, `hd` ile aynı), `-b -c -d -o -x`, `-n` ve `-s` (`0x`, sekizlik, `k`/`m` ekleri), `-v`; yinelenen satırlar `*` ile kısaltılır.

#### Yılan (`snake`)

`snake` tam ekran bir yılan oyunudur. Oklar ya da WASD ile yön verilir; `p` duraklatır, `q` ya da Ctrl-C çıkar, çarpınca `r` yeniden başlatır. Her yem 10 puandır ve skor arttıkça yılan hızlanır. Rekor `/root/.yilan_rekor` dosyasında tutulur, ext4 kökte olduğu için açılışlar arasında kalır. `snake -o` otopilottur: yılan yeme doğru giderken taşkın doldurmayla sıkışacağı yollardan kaçınır; `p` ile duraklatılır, `q` ile çıkılır, yönü yılan seçtiği için ok tuşları yok sayılır. Ekran ANSI dizileriyle (`\e[satır;sütunH`, renkler, `\e[?25l`) ve yalnızca değişen hücreler çizilerek güncellenir. Bu yüzden oyun çerçeve tamponu konsolunda, VGA metin ekranında ve seri terminalde aynı şekilde çalışır.

### Grafik arayüz (X11 ve JWM)

```text
root@uppix:~# startx &
startx: X hazır (:0)
jwm: masaüstü hazır
```

Uppix, X.Org'un kdrive sunucusu **Xfbdev**'i ve **JWM** pencere yöneticisini değiştirilmemiş Linux programları olarak çalıştırır: ikisi de musl ile statik derlenmiş sıradan ELF'lerdir. Xfbdev `/dev/fb0`'a çizer (`mmap`), klavyeyi ve fareyi `/dev/input/event0` ve `event1`'den (evdev) okur, konsolu `KD_GRAPHICS`'e alır. İstemciler `/tmp/.X11-unix/X0` ve aynı adlı soyut AF_UNIX soketinden bağlanır. Klavye düzenini X açılışta `xkbcomp` ile derler.

**Derleme** (Linux ya da WSL; `gcc`, `make`, `python3`, `curl`, `xz`, `bzip2`):

```bash
sh ui/derle.sh
```

[`ui/derle.sh`](ui/derle.sh) kaynak arşivlerini x.org'dan, github.com/joewing/jwm'den ve musl'ü musl.libc.org'dan indirir, betikte sabit duran sha256 özetleriyle doğrular ve `derleme/ui/` altında saklar (git'te yok sayılır). Derleme `~/.cache/uppix-ui`'de yapılır, çünkü WSL'de Windows diski yavaştır (`UPPIX_UI_IS` ile değiştirilebilir). Her şey `--prefix=/usr` ve `DESTDIR` ile bir hazırlama köküne kurulur, sonuç `derleme/ui/kur`'a kopyalanır (8.9 MiB). Her paket bitince damgalanır: yarıda kalan derleme kaldığı yerden sürer, betik ve girdileri değişmediyse hemen biter. İlk derleme birkaç on dakika sürebilir. `derleme/ui/kur` varsa `disk_yap.sh` içeriğini imajın köküne koyar (imaj zaten varsa `debugfs` ile günceller).

| Paket | Sürüm | Seçenekler |
|---|---|---|
| musl | 1.2.5 | `musl-gcc` sarmalayıcısıyla; `-static -no-pie` |
| xorgproto, libXau, xcb-proto, libxcb, xtrans | 2018.4, 1.0.9, 1.14.1, 1.14, 1.4.0 | belgeler kapalı |
| libX11, libXext | 1.6.12, 1.3.4 | yüklenebilir i18n ve XF86BigFont kapalı |
| pixman, libfontenc, libXfont2 | 0.38.4, 1.1.4, 2.0.4 | libXfont2'de FreeType, BDF/SNF ve fontconfig kapalı: yalnızca gömülü yazı tipleri |
| libxkbfile, xkbcomp, xkeyboard-config | 1.1.0, 1.4.4, 2.23.1 | XKB verisi `/usr/share/X11/xkb` |
| xorg-server | 1.19.7 | Xfbdev'in bulunduğu son sürüm (1.20'de kaldırıldı). Yalnızca kdrive/Xfbdev; Xorg, Xwayland, Xnest, Xvfb, Xephyr, GLX, DRI, MIT-SHM, TCP ve giriş iş parçacığı kapalı; yazı tipi yolu `built-ins` |
| JWM | 2.4.3 | Xft, Xrender, PNG, JPEG, XPM, cairo, rsvg, fribidi, Xinerama, Xmu ve NLS kapalı; shape açık (yuvarlak köşeler); X çekirdek yazı tipi `fixed`, metin çizerken UTF-8'den çevrilir ([`ui/yama/jwm/latin1.py`](ui/yama/jwm/latin1.py)) |

**Çalıştırma:** Uppix'te `startx` (kabuğu serbest bırakmak için `startx &`). [`startx`](kullanici/startx.c) Xfbdev'i `:0`'da başlatır (`-dumbSched -noreset`, evdev klavye ve fare), soket bağlantı kabul edince `DISPLAY=:0` ile JWM'yi çalıştırır. JWM bitince (menüde "Oturumu kapat" ya da seri konsoldan `jwm -exit`) X'i durdurur ve konsol metin kipine döner. `startx /usr/bin/xdeneme` JWM yerine tek bir istemci çalıştırır. Sunucunun iletileri `/tmp/Xfbdev.0.log`'a gider. Ekranı ve fareyi görmek için QEMU'yu pencereyle açın:

```bash
uppc derle .
qemu-system-x86_64 -m 256 -serial stdio -display gtk -drive file=derleme/uppix.img,format=raw
```

(`-display sdl` de olur.) Komutları seri terminalden yazarsınız; X açıkken klavye ve fare QEMU penceresindedir (fareyi pencereye tıklayarak yakalayın, Ctrl-Alt-G bırakır).

**Masaüstü:** Masaüstü bir degradedir, pencere başlıkları degradeli ve köşeleri yuvarlaktır. Alttaki şeritte soldan sağa "Uppix" (menü), "Terminal", iki masanın küçük görünümü, pencere listesi, saat (UTC) ve "Oturum" düğmesi vardır. Masaüstüne sol ya da sağ tıklamak, "Uppix" düğmesi ya da Alt-F1 menüyü açar: Terminal, Ayarlar, test penceresi, masalar, JWM'yi yenile, Oturumu kapat, Yeniden başlat, Kapat. Pencereler başlıktan taşınır, kenardan boyutlandırılır; başlıkta sağ tık pencere menüsünü açar. Kısayollar: Ctrl-Alt-T uçbirim, Ctrl-Alt-Del oturum penceresi, Alt-Tab / Alt-Shift-Tab pencere değiştirir, Alt-F2 pencere menüsü, Alt-F4 kapat, Alt-F9 simge durumu, Alt-F10 büyüt, Alt-1/Alt-2 ve Ctrl-Alt-←/→ masa değiştirir. Menüler, kısayollar ve fare bağlamaları [`/etc/jwm/system.jwmrc`](kok/etc/jwm/system.jwmrc)'dedir (JWM 2.4'te fare bağlamalarının varsayılanı yoktur); görünüm onun içerdiği [`/etc/jwm/tema.jwmrc`](kok/etc/jwm/tema.jwmrc)'dedir.

| Program | Ne yapar |
|---|---|
| [`uterm`](kullanici/x11/uterm.c) | Uçbirim. `/dev/ptmx`'ten PTY açar, çocukta yavru ucu stdin yapıp etkileşimli `/bin/sh` çalıştırır; çekirdek o PTY'ye bağlı bir **uppsh oturumu** başlatır, yani 81 yerleşik komut, boru, geçmiş, Tab tamamlama, `nano`/`vi`/`snake` ve ring 3 programları uçbirimde çalışır. VT100/ANSI: imleç hareketi, silme, satır ekleme/silme, kaydırma bölgesi, 16 renk (256 renkten yaklaştırma), kalın/ters/altı çizili, alternatif ekran, imleç konumu sorgusu; kutu çizgileri yazı tipinin DEC çizgi glif'leriyle çizilir. Pencere boyutu değişince `TIOCSWINSZ`. Oklar, Home/End, PgUp/PgDn, Delete, F1-F12; Türkçe düzende ş, ğ, ı, İ UTF-8 gider |
| [`ayarlar`](kullanici/x11/ayarlar.c) | Tema (Gece, Orman, Gün batımı, Grafit: masaüstü degradesi, başlık, şerit ve menü renkleri), klavye düzeni (us / tr), saat biçimi ve sistem bilgisi (çekirdek, bellek, ekran, açık kalma süresi). "Uygula" `/etc/jwm/tema.jwmrc`, `/etc/jwm/ayarlar` ve `/etc/X11/klavye`'yi yazar, düzeni XKB ile hemen yükler (`XkbGetKeyboardByName`; sonraki `startx` de `-xkb-layout` ile açar) ve `jwm -restart` çalıştırır (2.4'te `-reload` yalnızca menüyü yükler). `ayarlar --uygula TEMA SAAT us\|tr` penceresiz uygular |
| [`oturum`](kullanici/x11/oturum.c) | Onay penceresi: Oturumu kapat (`jwm -exit`; startx X'i durdurur, konsol metin kipine döner), Yeniden başlat ve Kapat (`reboot(2)`: çekirdek dosya sistemlerini boşaltır). `oturum cikis\|yeniden\|kapat` yalnızca o işi sorar; Enter onaylar, Esc vazgeçer |
| [`xdeneme`](kullanici/x11/xdeneme.c) | Test penceresi: yeşil pencere, kırmızı dikdörtgen, bir tuş ve bir fare olayı |
| [`doom`](kullanici/doom/uppix.c) | Doom (bkz. [Doom](#doom-1)) |

Üçü de ortak [`arayuz.h`](kullanici/x11/arayuz.h)'yi kullanır: TrueColor renkler, degrade dolgu, düğmeler, UTF-8 → ISO 8859-1 metin ve pencere adı (`WM_NAME` Latin-1, `_NET_WM_NAME` UTF-8).

**Tasarım kararları:**
- **`/bin/sh`:** X `xkbcomp`'u, JWM de menü komutlarını `/bin/sh -c` ile çalıştırır. Uppix'te `/bin/sh` çekirdekteki kabuğa giden bir yerleşik komuttur (`#!/yerlesik/sh`), süreç olarak çalıştırılamaz. Bu yüzden `execve` bu satırı görünce, `/usr/bin/ush` varsa onu aynı argv ile çalıştırır. [`ush`](kullanici/ush.c), libc'siz ~330 satırlık bir C kabuğudur. Seçenekler şunlardı: busybox (yeni bir kaynak), xkbcomp'u X'e gömmek (sunucuyu yamalamak) ya da önceden derlenmiş `.xkm` (X düzen her değiştiğinde yine xkbcomp çağırır). En küçük ve Linux'a en sadık seçenek ush'tı. JWM'nin `StartupCommand`'ı da ush ile `echo` çalıştırıp "jwm: masaüstü hazır" yazar (testler bunu bekler).
- **`startx` bir C programıdır:** uppsh'te döngü olmadığından "soket kabul edene dek bekle" betikle yazılamaz.
- **Gömülü yazı tipleri:** libXfont2, `fixed` ve `cursor` yazı tiplerini gzip'li PCF olarak tutar ve zlib ile açar. Yeni bir kaynak indirmemek için zlib, açma işlevleri başarısız dönen bir yer tutucudur ([`ui/yama/zlib`](ui/yama/zlib)); [`gomulu_ac.py`](ui/yama/libxfont2/gomulu_ac.py) dizileri derleme sırasında açar. X'in istediği SHA-1 için de küçük bir uygulama vardır ([`ui/yama/libsha1`](ui/yama/libsha1)).
- **pkg-config:** [`ui/pkg-config`](ui/pkg-config) (Python), statik bağlamada `Requires.private` zincirini bağımlılık sırasıyla açar ve hazırlama kökünü yollara ekler.
- **XKB kuralları:** xkeyboard-config'in `rules` dosyaları xsltproc/perl ile üretilir. Onların yerine elle yazılmış kısa bir `evdev` kuralı ([`ui/xkb/evdev`](ui/xkb/evdev)) kullanılır: pc105, düzen `-xkb-layout` ile (`us`, `tr`; xkeyboard-config'in bütün `symbols` dosyaları imajdadır).
- **Uçbirimde uppsh:** Uppix'te kabuk çekirdektedir; bir uçbirimin işe yaraması için ELF'lerden çok yerleşik komutlar gerekir. Bu yüzden ush'u uçbirime bağlamak yerine çekirdek, PTY'ye bağlı yeni bir uppsh oturumu başlatır ([`kaynak/pty.upp`](kaynak/pty.upp)). Oturum ayrı bir çekirdek görevidir; kabuğun çalışma dizini, `$?`, `exit` ve konum parametreleri oturum başınadır, ortam değişkenleri, takma adlar, geçmiş ve işler ortaktır. Kabuğun ve komutların bütün terminal G/Ç'si (`konsol_*`) görevin oturumuna göre ekrana/seri porta ya da PTY'ye gider; tuşlar konsolun ya da PTY'nin kuyruğundan gelir. Oturumdaki süreçlerin terminali o PTY'dir (`TIOCGWINSZ` PTY boyutu, KD/VT ioctl'ları yok); Ctrl-C ön plandaki komutu keser.
- **JWM metni:** Xft olmadan JWM metni X çekirdek yazı tipiyle dönüştürmeden çizer. Küçük bir yama ([`ui/yama/jwm/latin1.py`](ui/yama/jwm/latin1.py)) çizimden ve genişlik ölçmeden önce UTF-8'i ISO 8859-1'e çevirir; böylece menüler ve başlıklar UTF-8 yazılabilir.

**Sınırlar:** Tek ekran ve tek kip vardır (Limine'nin verdiği çerçeve tamponu, ör. 1280x800x32); `FBIOPUT_VSCREENINFO` yalnızca geçerli kipi kabul eder. VT geçişi (Ctrl-Alt-F*) yok. Yazı tipleri yalnızca `fixed` (6x13) ve `cursor`'dır; `fixed` ISO 8859-1 olduğundan ü, ö, ç doğru görünür ama ş, ğ, ı, İ menülerde, başlıklarda ve uçbirimde s, g, i, I olarak çizilir (kabuğa ve dosyalara UTF-8 gider). Duvar kâğıdı yalnızca düz renk ya da degradedir (XPM/PNG için kitaplık yok). uterm'de geri kaydırma ve seçim/pano yok; sistem çağrısı yapmayan bir döngü uçbirimden Ctrl-C ile kesilmez (`kill` ile durur). PTY yavru ucunu doğrudan okuyan programlar ham bayt alır (satır düzeni kabuktadır). `link` kopyalar, sahiplik tutulmaz, her süreç root'tur. X ve istemciler AF_UNIX ile konuşur (TCP yok); dosya yolu soketi yalnızca bellekteki dosya sistemlerinde kurulabilir (`/tmp`). İşbirlikçi çekirdekte bekleyen çağrılar 1 ms'lik adımlarla yokladığından her X gidiş-dönüşü bir zamanlayıcı adımı sürer: JWM'nin açılışı yaklaşık 5 saniyedir. ush'ta boru (`|`) yoktur.

#### Doom

id Software'in Doom'u, [doomgeneric](https://github.com/ozkl/doomgeneric) (Chocolate Doom'dan türetilmiş, ekranı ve tuşları dört işleve indirgeyen sürüm; GPLv2) ile çalışır. Kaynak [`kullanici/doom/doomgeneric`](kullanici/doom/doomgeneric)'tedir (ozkl/doomgeneric `e257a09`, yalnızca derlenen dosyalar; değiştirilmedi). Uppix arka ucu [`kullanici/doom/uppix.c`](kullanici/doom/uppix.c)'dir; `ui/derle.sh` musl ve libX11 ile statik derleyip `/usr/bin/doom` yapar. Bölüm dosyası paylaşımlı (shareware) **DOOM1.WAD** v1.9'dur (1. bölüm "Knee-Deep in the Dead", 9 harita; Debian'ın `doom-wad-shareware` paketinden, MD5 `f0cefca49926d00903cf57551d901abe`). id Software paylaşımlı sürümün değiştirilmeden ücretsiz dağıtılmasına izin verir: lisans [`doom1.wad.lisans`](kullanici/doom/doom1.wad.lisans)'tadır. Dosya imajda `/usr/share/games/doom/`'dadır; Doom orayı kendisi arar. Tam sürümün `doom.wad`'ı ya da `doom2.wad` aynı dizine konursa ya da `-iwad` ile verilirse onlar da açılır.

- **Masaüstünde:** menüde "Doom" ya da Ctrl-Alt-D; 640x400'lük bir pencere açılır.
- **Konsolda:** X açık değilken `doom` tam ekran açılır (`/dev/fb0`, 1280x800'de 2 kat büyütülmüş). Klavye `/dev/input/event0`'dan okunur ve oyun açıkken kabuğa gitmez. Çıkınca konsol metin kipine döner.
- `-fb` / `-x11` kipi zorlar; Doom'un kendi seçenekleri de geçer (`-warp 1 3`, `-skill 4`, `-timedemo demo1` …). Ayarlar (`default.cfg`) ve kayıtlar `$HOME`'a (yoksa `/root`) yazılır.

Tuşlar: oklar yürür/döner, Ctrl ateş eder, boşluk kapı açar, Shift koşturur, Alt basılıyken oklar yana kaydırır, `,` `.` yana adım, 1-7 silah, Tab harita, Esc menü, F2/F3 kaydet/yükle. Tuşlar klavyedeki yerlerine göre okunur (X'te keycode, konsolda evdev kodu), bu yüzden `tr` düzeninde de aynı çalışır. Ses yoktur (Uppix'te ses aygıtı yok).

### Mimari

| Dosya | İçerik |
|---|---|
| `kaynak/uppix.upp` | Açılış: çekirdek günlüğü, disk tarama, kök bağlama, sanal dosya sistemleri, `/etc`, komut tablosu, oturum |
| `kaynak/vfs.upp` | VFS: düğüm önbelleği (`sinif Dugum`), bağlama tablosu, sürücü arayüzü (`sinif Surucu`), yol çözme, `/proc` |
| `kaynak/veriyolu.upp` | Aygıt ağacı ve sürücü çatısı (FreeBSD newbus): aygıtlar, sürücü kaydı (`DRIVER_MODULE`), yoklama/bağlama/ayırma, `nexus`, PCI veri yolu taraması ve BAR'lar, `hostb`/`isab`, MMIO (bus_space) ve DMA (bus_dma) kaynak işlevleri |
| `kaynak/blok.upp` | Blok aygıtları: disk kaydı (sürücülerin okuma/yazma fonksiyon değerleri), MBR/GPT bölümleri, dosya sistemi tanıma |
| `kaynak/ata.upp` | ATA sürücüleri: PCI IDE denetleyicisi (`atapci`) ve kanalları (`ata`); diskleri disk kaydına koyar |
| `kaynak/girdi.upp` | Girdi: `atkbdc`/`atkbd`/`psm` sürücüleri, IntelliMouse algılama ve paket çözme, tarama kodu → `KEY_*`, açılış başına olay kuyrukları (evdev, mousedev), konsol kipleri (KD/K/VT) |
| `kaynak/sistem_cagrisi_soket.upp` | AF_UNIX soketleri: soket nesnesi, adresler (yol, soyut, adsız), akış halkası ve datagram kuyruğu, `SCM_RIGHTS`, seçenekler, `socket`…`recvmsg` |
| `kaynak/pty.upp` | Sözde terminaller (`/dev/ptmx`, `/dev/pts/N`) ve kabuk oturumları: oturum başına çalışma dizini, `$?`, `exit`; konsol G/Ç katmanı (`konsol_yaz`, `konsol_tus_bekle`…), PTY'ye bağlı uppsh oturumu |
| `kaynak/sistem_cagrisi_hazir.upp` | Hazırlık modeli (`fd_hazir`) ve üstündeki `poll`/`select`/`epoll`; `eventfd`, `timerfd`, `setitimer`/`alarm`/`pause`; terminalin satır düzeni |
| `kaynak/sistem_cagrisi_girdi.upp` | `/dev/input/*` okuma/yazma/ioctl, konsol ioctl'ları, `K_RAW`/`K_MEDIUMRAW` okuma |
| `kaynak/ahci.upp` | AHCI (SATA) sürücüleri: denetleyici (`ahci`) ve kapılar (`ahcich`); IDENTIFY, READ/WRITE DMA EXT, FLUSH CACHE EXT, yoklamalı komut bitişi |
| `kaynak/ext4.upp` | ext4 sürücüsü |
| `kaynak/fat.upp` | FAT32 sürücüsü |
| `kaynak/exfat.upp` | exFAT sürücüsü |
| `kaynak/kabuk.upp` | `uppsh`: satır bölme, jetonlar ve açılımlar, boru ve yönlendirme, ve-veya listeleri, arka plan işleri, betikler |
| `kaynak/satir.upp` | Satır düzenleyici, geçmiş, Tab tamamlama, Ctrl-C ile kesme |
| `kaynak/cikti.upp` | Görev başına çıktı yakalama (boru ve yönlendirme için) |
| `kaynak/komut_*.upp` | Komutlar; her biri `fonk(liste[metin] a, metin girdi) -> sayi` (`komut_oyun.upp`: `snake`, `komut_nano.upp`, `komut_vi.upp`, `komut_hexdump.upp`, `komut_aygit.upp`: `devinfo`, `pciconf`) |
| `kaynak/surec.upp` | Kullanıcı süreçleri: süreç tablosu, dosya tanımlayıcıları, ELF64 yükleyici, SysV ilk yığını, kabuktan çalıştırma |
| `kaynak/sistem_cagrisi.upp` | Linux x86_64 sistem çağrısı dağıtıcısı; G/Ç ve bellek (read, write, open, stat, lseek, ioctl, mmap, brk…) |
| `kaynak/sistem_cagrisi_dosya.upp` | Dosya sistemi ve tanımlayıcı çağrıları (pread/pwrite, readv/writev, dup, fcntl, getdents64, access, getcwd/chdir, mkdir, unlink, rename, readlink) |
| `kaynak/sistem_cagrisi_surec.upp` | Süreç, zaman, TLS ve sinyal çağrıları (fork/clone, execve, wait4/waitid, kill, arch_prctl, uname, clock_*, getrandom, mprotect, rt_sig*, prlimit64, futex…) |
| `kaynak/duzenleyici.upp` | `nano` ve `vi`'nin ortak düzenleyici çekirdeği |
| `kullanici/*.c` | Ring 3 programları (libc'siz statik Linux ELF); `linux.h` ortak başlık; `kullanici/glibc/*.c` glibc ile statik; `kullanici/x11/*.c` libX11 ile (musl, `ui/derle.sh`); `kullanici/doom` doomgeneric, Uppix arka ucu ve DOOM1.WAD |
| `ui/` | Grafik arayüz: `derle.sh` (X.Org Xfbdev, xkbcomp, JWM), `pkg-config`, `yama/` (zlib yer tutucusu, libsha1, gömülü yazı tiplerini açma), `xkb/evdev` |
| `kaynak/yardimci.upp` | Sıralama, biçimleme, joker eşleme, takvim/Unix zamanı |

Her dosya sistemi sürücüsü fonksiyon değerlerinden oluşan bir `sinif Surucu`'dur: `bagla`, `yukle` (dizini önbelleğe al), `oku`, `yaz`, `olustur`, `sil`, `tasi`, `oznitelik`, `alan`. VFS hangi düğümün hangi bağlamaya ait olduğunu bilir ve doğru sürücüyü çağırır. Dizinler ilk erişimde yüklenir. Komutlar dosya sisteminden habersizdir; `cp /root/a.txt /mnt/sdc/` gibi diskler arası kopyalama aynı kodla çalışır.

İkili veri `upp.tampon.*` ile işlenir. Bunlar sınır denetimli bayt tamponlarıdır: küçük sonlu okuma/yazma, UTF-16, CRC32C. Diske blok katmanı disk kaydı üzerinden erişir; ATA sürücüsü kayda çekirdeğin `upp.cekirdek.disk_oku/disk_yaz` PIO mekanizmasını, AHCI sürücüsü kendi DMA okuma/yazmasını (u++'ta, MMIO pencereleri ve `upp.cekirdek.tampon_fiziksel` ile) koyar. Sürücüler PCI yapılandırma alanını `upp.cekirdek.pci_oku` ile okur; yapılandırma yazma ve BAR penceresi açma (`pci_yaz`, `mmio_ac`) `guvensiz` gerektirir ve yalnızca `veriyolu.upp`'deki iki yerde yapılır, pencere içi erişim (`mmio_oku/yaz`) sınır denetimlidir. Ayrıntılar için [U++ API belgesine](https://github.com/ploxdev/upp-lang/blob/main/docs/API_tr.md) bakın.

#### Testler

Testler QEMU'da açılır; seri porttan komut gönderir, çıktıyı ve (gerekirse) disk imajlarını Linux araçlarıyla denetler:

```bash
python3 tests/run_tests.py --uppc derleyici/uppc            # hepsi
python3 tests/run_tests.py --uppc derleyici/uppc --dosya tests/cekirdek/06_uppix_kullanici.upp
```

| Test | Ne yapar |
|---|---|
| `tests/cekirdek/01_uppix.upp` | Disksiz açılış (RAM kök); 25 kabuk komutu |
| `tests/cekirdek/02_uppix_disk.upp` | İmajları sıfırdan üretir, üç dosya sistemine yazar, sonra Linux araçlarıyla denetler: `e2fsck -fn`, `debugfs`, `fsck.fat`, `mtype`, `fsck.exfat` |
| `tests/cekirdek/03_uppix_kalici.upp` | Aynı imajlarla ikinci açılış: yazılanlar yerinde mi |
| `tests/cekirdek/06_uppix_kullanici.upp` | Ring 3: argv/çıkış kodu, `sctest`, `sctest2`, glibc ile derlenmiş `gmerhaba`, terminal ioctl'u, boru, SIGSEGV, zaman dilimiyle kesilen döngü ve `kill`; üç programın çıktısı gerçek Linux'takiyle `diff` ile karşılaştırılır, ext4 kök sonra `e2fsck` ile denetlenir |
| `tests/cekirdek/13_uppix_soket.upp` | AF_UNIX soketleri ve hazırlık çağrıları: `sctest4` (fork ile istemci-sunucu dahil) ve glibc ile derlenmiş `gsoket`; ikisinin çıktısı gerçek Linux'takiyle `diff` ile karşılaştırılır; sonra `e2fsck` |
| `tests/cekirdek/12_uppix_fare.upp` | Girdi ve grafik konsol (`--qemu-izleyici`): `devinfo`'da `atkbdc0`/`atkbd0`/`psm0`; `olaytest` evdev ioctl'ları ve QEMU izleyicisinden gelen `mouse_move`/`mouse_button`/`sendkey` olaylarının dizisi; `fbfare` imleç ve iz (izleyicide `screendump`, pikseller `@denetle`'de denetlenir); kipte biten süreçten sonra konsol `KD_TEXT`/`K_XLATE`'e döner |
| `tests/cekirdek/11_uppix_surecler.upp` | Çoklu süreç: `sctest3` (fork, wait4, waitid, kill, boru, dup2 + execve, `#!`, O_CLOEXEC, hata kodları) ve glibc ile derlenmiş `gsurec`; ikisinin çıktısı gerçek Linux'takiyle `diff` ile karşılaştırılır; çocukların çıktısı boruya ve `>`'a gider; sonra `e2fsck` |
| `tests/cekirdek/08_uppix_disk_kaynak.upp` | Linux'ta `fallocate` ile ayrılmış (yazılmamış kapsamlı) ext4 dosyası sıfır okunur ve silinince blokları bırakılır; FAT32 art arda ayrılıp yeniden bağlanır; sonra `e2fsck` ve `fsck.fat` |
| `tests/cekirdek/07_uppix_araclar.upp` | `hexdump`/`hd` biçimleri; tuşları seri porttan gelen `nano` (yazma, kes/yapıştır, arama, kaydetmeyi sorma) ve `vi` (ekleme, `yy p`, `dd`, `:%s`, `:q`'nun reddi, `:wq`) oturumları |
| `tests/cekirdek/05_uppix_snake.upp` | `snake -o -n 200` otopilotu beklemeden oynatır; rekor dosyası, boru/yönlendirmede reddetme ve seçenek hataları |
| `tests/cekirdek/09_uppix_veriyolu.upp` | QEMU i440fx'te aygıt ağacı: `pciconf -l`/`-lv`/`-lb` ana köprüyü (8086:1237) ve PIIX3 IDE'yi (`atapci0`) gösterir, `devinfo` `nexus0 → pci0 → atapci0 → ata0/ata1`; diskler `ata` sürücüsünden disk kaydına girer, ext4'e ve FAT32'ye eskisi gibi erişilir, sonra `e2fsck` ve `debugfs` |
| `tests/cekirdek/10_uppix_ahci.upp` | Üç disk ICH9 AHCI kapılarında (`--disk-arayuz ahci`): Limine SATA diskinden açılır, kök ext4 SATA'dan bağlanır; `pciconf`, `devinfo` (`ahci0`, `ahcich0..5`) ve `lsblk`; ext4'e (birden çok DMA parçalı büyük dosya dahil), `/boot`'a, FAT32'ye ve exFAT'e yazılır, ring 3 programı SATA'dan yüklenir; sonra `e2fsck -fn`, `debugfs`, `fsck.fat -n`, `mtype`, `fsck.exfat -n` |
| `tests/cekirdek/04_uppix_limine.upp` | `uppix.img`'i üretir ve Limine ile 64-bit açar; `/boot` ve kökü yazar, sonra iki bölümü `e2fsck`, `fsck.fat` ve `mtype` ile denetler |
| `tests/cekirdek/14_uppix_x11.upp` | Grafik arayüz (`--qemu-izleyici`, `@sure: 400`): `@hazirla` `ui/derle.sh`'i çalıştırır (önbellekten hızlı); `startx`, JWM'nin hazır satırı, `xdeneme` penceresi ve izleyiciden `sendkey`/`mouse_move`/`mouse_button`; `uterm`'e yazılan komutların (`echo`, `ls /dev/pts`, `pwd`) PTY'deki uppsh oturumundan gelen çıktısı (`--dokum`); kök menü; `ayarlar --uygula` ile tema, saat ve `tr` düzeni; `oturum cikis` onayı, ikinci `startx`, metin kipi; son olarak `oturum kapat` ile `reboot(2)`. `screendump`'larda pencere, masaüstü degradesi, şerit, uçbirim, menü ve yeni tema pikselleri; sonra `e2fsck` |
| `tests/cekirdek/15_uppix_doom.upp` | Doom (`--qemu-izleyici`, `@sure: 600`): konsolda `doom -fb` 1280x800'e 2 kat; izleyiciden Esc, Yukarı, Enter, y ile menüden çıkış ve metin kipine dönüş; `startx`'ten sonra `doom -timedemo demo1` X penceresinde demoyu oynatıp süreyi yazar. `screendump`'larda Doom karesi ve metin kipi; sonra `e2fsck` |

### Sınırlar ve sıradakiler

- ext4 yazımı günlük (journal) kullanmaz. Dosya en çok 4 extent'te tutulur, yani boş alan çok parçalıysa büyük dosya yazılamaz. htree ile dizinlenmiş büyük dizinlere ekleme yapılmaz. Grup bitmap'i başlatılmamış (`*_UNINIT`) gruplar ayırmada atlanır.
- FAT12/FAT16 ve 512 bayt dışı sektör boyutları desteklenmez. Blok önbelleği yoktur: sürücüler dosyayı bütün olarak okur, yalnızca son okunan dosyanın içeriği bellekte tutulur ([`vfs.upp`](kaynak/vfs.upp), `fs_oku_tampon`; `read(2)` oradan pencereyi kopyalar). Diğer her işlem diske gider.
- Disk sürücüleri ATA PIO ve AHCI'dir. AHCI tek komut yuvası kullanır (NCQ yok), kesme yerine yoklar, sıcak takmayı ve ATAPI (SATA CD) aygıtlarını desteklemez. NVMe ve virtio-blk sırada.
- Aygıt ağacında PCI-PCI köprüleri ayrı veri yolu (`pcib`) olarak yer almaz: bütün veri yolları `pci0`'ın altında düz taranır. Sürücülere kesme (INTx/MSI) bağlanmaz, G/Ç BAR'ları için pencere yoktur. Multiboot yolunda BAR'ın MMIO penceresi yalnızca ilk 1 GiB'de açılabilir (Limine'de HHDM'in kapsadığı her adreste; QEMU'nun 4 GiB altındaki BAR'ları açılır); bu yüzden AHCI yalnızca Limine ile çalışır.
- Çekirdek içindeki kod işbirlikçidir; yalnızca ring 3 kodu kesilir. Yerleşik komutlar çekirdek içinde çalışır.
- Kullanıcı süreçleri tek iş parçacıklıdır: `clone` yalnızca fork'a denk biçimde çalışır, `CLONE_VM`/`CLONE_THREAD` (pthread, `posix_spawn`, glibc `system()`/`popen()`) yoktur. Sinyal işleyicileri çağrılmaz, durdurma (SIGSTOP/SIGTSTP) da yoktur: yakalanan sinyal de süreci bitirir. Dinamik bağlama (`PT_INTERP`), sembolik bağlar ve `rseq` yok; statik programlar (libc'siz ya da glibc) çalışır.
- `fork` sayfaları hemen kopyalar (yazmada kopyalama, COW yok). `MAP_SHARED` dosya eşlemeleri çocukta özel bir kopyaya dönüşür: ebeveyn ve çocuk birbirinin yazdığını görmez, ikisi de kendi yazdığını `msync`/`munmap`/çıkışta dosyaya geri yazar. `dup` ya da `fork` ile çoğaltılan tanımlayıcılar dosya konumunu paylaşmaz; kabuktan gelen stdin (boru, `<`) çocuğa kopyalanır. Her çocuk bir çekirdek görevi kullanır: aynı anda en çok 32 görev (kabuk ve arka plan işleri dahil), dolunca `fork` `EAGAIN` döner.
- Başka bir süreçten gelen sinyal, fork'un çocuğunu hemen bitirir; kabuğun görevinde koşan süreci (kabuktan başlatılan süreç) ise sıradaki sistem çağrısında ya da beklerken bitirir: sistem çağrısı yapmayan bir döngü yalnızca Ctrl-C ya da kabuğun `kill`'iyle durur. `rusage` ve süreç süreleri tutulmaz. `execve`'de argüman/ortam toplamı 256 KiB'ı aşamaz. Yığın 1 MiB'dır ve sayfa hatasıyla büyümez.
- Soketler yalnızca `AF_UNIX`'tir (`AF_INET`/`AF_INET6`, `SOCK_SEQPACKET` yok). Dosya yolu soketleri yalnızca bellekteki dosya sistemlerinde (`/tmp` gibi tmpfs) kurulabilir, diskte `EOPNOTSUPP`. `MSG_OOB` yok; `SO_SNDBUF`/`SO_RCVBUF` saklanır ama akış tamponu sabit 256 KiB'tır, datagram kuyruğu 64 iletidir. Kendi üzerinden geçirilen soket tanımlayıcısı (döngü) toplanmaz. `epoll` kayıtları (süreç, fd) çiftine bağlıdır: `fork`'tan sonra çocuk aynı örneği kullanırsa ebeveynin tanımlayıcılarını izler. `EPOLLEXCLUSIVE` yok sayılır. Bekleyen çağrılar 1 ms'lik adımlarla yoklar; işlemci süresi tutulmadığından `ITIMER_VIRTUAL`/`ITIMER_PROF` dolmaz. Terminalin `termios`'u (`TCSETS`) saklanır ama satır düzeni kanonik kalır (ham kip `KDSKBMODE` ile).
- Girdi: tek sanal konsol vardır (VT geçişi yok); klavye LED'leri ve otomatik tekrar ayarı yok, evdev'e olay yazılamaz (`uinput` yok). USB fare/klavye yok (yalnızca PS/2).
- Çerçeve tamponu 4 GiB'nin altında ve RGB olmalı; değilse ekran çıktısı yalnızca seri porta gider. SMP yok: yalnızca açılış işlemcisi çalışır.
- `uppc` henüz Uppix içinde çalışmıyor, çünkü C çıktısını derleyecek bir derleyici gerekiyor.
