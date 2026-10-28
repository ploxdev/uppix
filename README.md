# Uppix

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

## Çalıştırma

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

Klavye ve ekranla bir pencerede açmak için (Limine menüsü, ardından çerçeve tamponu konsolu):

```bash
uppc derle .
qemu-system-x86_64 -m 256 -serial stdio -drive file=derleme/uppix.img,format=raw
```

Diskleri IDE yerine bir AHCI (SATA) denetleyicisine takmak için `upp.proje`'ye `disk_arayuz = ahci` ekleyin ya da `uppc`'ye `--disk-arayuz ahci` verin (en çok 6 disk; QEMU'da `-device ahci`, diskler `ide-hd,bus=ahci0.N`). Elle:

```bash
qemu-system-x86_64 -m 256 -serial stdio -device ahci,id=ahci0 \
    -drive file=derleme/uppix.img,format=raw,if=none,id=d0 -device ide-hd,drive=d0,bus=ahci0.0,bootindex=0 \
    -drive file=derleme/usb-fat32.img,format=raw,if=none,id=d1 -device ide-hd,drive=d1,bus=ahci0.1
```

UEFI ile açmak için QEMU'ya OVMF verin, örneğin `-bios /usr/share/qemu/OVMF.fd`. Aynı imaj gerçek bir USB belleğe de yazılabilir (`dd if=uppix.img of=/dev/sdX`).

Eski Multiboot yolu da durur: `--onyukleyici multiboot` (varsayılan) ile çekirdek 32-bit `uppix32.elf` olarak üretilir ve QEMU `-kernel` ile açılır; `sh disk_yap.sh` (çekirdek vermeden) bu yol için ayrı `uppix-kok.img` ext4 imajını üretir. Disk verilmezse Uppix RAM'deki bir kök dosya sistemiyle (rootfs) açılır. `poweroff` dosya sistemlerini boşaltıp makineyi kapatır, `reboot` yeniden başlatır.

## Neler var

- **Dosya sistemleri:**
  - **ext4:** okuma ve yazma. Extent ağaçları, eski ext2/3 blok haritaları, htree dizinleri (okuma) ve `metadata_csum` desteklenir: inode, dizin bloğu, bitmap, grup tanımlayıcı ve süperblok sağlamaları CRC32C ile güncellenir. Desteklenmeyen bir özellik varsa disk salt okunur bağlanır.
  - **FAT32:** okuma ve yazma. VFAT uzun dosya adları (UTF-16) okunur ve yazılır, `AD~1.UZN` kısa adları üretilir, tüm FAT kopyaları birlikte güncellenir.
  - **exFAT:** okuma ve yazma. Ayırma bitmap'i, FAT zinciri ve bitişik (NoFatChain) kümeler, girdi kümesi sağlaması, büyük harf tablosuna göre ad karması ve birim etiketi desteklenir.
  - **Bellekte:** `/proc` (`cpuinfo`, `meminfo`, `mounts`, `partitions`, `uptime`, `version`, `dmesg`), `/dev` (`null`, `console`, `tty`, `tty0`, `tty1`, `random`, `fb0`, `input/event0`, `input/event1`, `input/mice`, `sda`…; `st_rdev` Linux'taki numaralardır), `/tmp` ve `/mnt` (tmpfs), `/bin` (yerleşik komutlar).
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

## Kullanıcı kipi (ring 3) ve Linux sistem çağrıları

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
| 16 | `ioctl` | `TIOCGWINSZ` (terminal boyutu), `TCGETS`/`TCSETS`/`TCSETSW`/`TCSETSF` (`termios` saklanır ve geri okunur, kuran süreç bitince varsayılana döner; satır düzeni kanonik kalır), `TIOCSCTTY`, `TIOCNOTTY`, `TIOCGPGRP`/`TIOCSPGRP`, `TIOCGSID`, `FIONBIO`, `FIONREAD` (soket, boru, terminal, dosya); `FBIOGET_VSCREENINFO`, `FBIOGET_FSCREENINFO`, `FBIOPUT_VSCREENINFO` (yalnızca geçerli kip), `FBIOPAN_DISPLAY`, `FBIOBLANK`, `FBIOPUTCMAP`; konsolda `KDGETMODE`/`KDSETMODE`, `KDGKBMODE`/`KDSKBMODE`, `KDGKBTYPE`, `KDGETLED`/`KDSETLED`, `VT_OPENQRY`, `VT_GETSTATE`, `VT_GETMODE`/`VT_SETMODE`, `VT_ACTIVATE`/`VT_WAITACTIVE`, `VT_RELDISP`, `VT_DISALLOCATE` (tek VT); evdev'de `EVIOCG*`, `EVIOCGRAB`, `EVIOCSCLOCKID` |
| 2, 257 | `open`, `openat` | `O_RDONLY/WRONLY/RDWR`, `O_CREAT`, `O_EXCL`, `O_TRUNC`, `O_APPEND`, `O_DIRECTORY`, `O_CLOEXEC`; göreli yol `dirfd`'ye ya da sürecin çalışma dizinine göre; `/dev/null`, `/dev/tty`, `/dev/fb0` |
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
| 59 | `execve` | ELF ve `#!` betikleri (yorumlayıcı + tek argüman, en çok 4 düzey); argv/envp kullanıcı belleğinden; O_CLOEXEC tanımlayıcılar kapanır, yakalanan sinyaller varsayılana döner (`SIG_IGN` kalır), süreç adı ve `/proc/self/exe` güncellenir; hata sırası Linux'taki gibi: `ENOENT`/`EACCES`, `EFAULT`, `E2BIG` (131072 baytı aşan dizgi), `ENOEXEC` |
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

### Komutlar

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

### Düzenleyiciler (`nano`, `vi`)

İkisi de tam ekran çalışır ve aynı çekirdeği ([`duzenleyici.upp`](kaynak/duzenleyici.upp)) paylaşır: satır tamponu, sekmeleri 8 sütuna genişleten görüntü, yatay kaydırma, yalnızca değişen satırları çizen ekran, ANSI tuş dizilerini (oklar, Home/End, PgUp/PgDn, Delete) çözen tuş okuyucu ve geri alma/yineleme. Ekran boyutu `TIOCGWINSZ` ile aynı kaynaktan gelir; `LINES`/`COLUMNS` onu ezer. Çerçeve tamponu konsolunda, VGA ekranda ve seri terminalde çalışır.

- **`nano [+SATIR[,SÜTUN]] [dosya]`:** GNU nano'nun kısayolları: `^O` yaz, `^S` kaydet, `^X` çık (değiştiyse `E`/`H` sorar), `^K` kes (art arda kesimler birikir), `^U` yapıştır, `^W` ara, `^C` konum, `^_` satıra git, `^Z`/`M-U` geri al, `M-E` yinele, `^G` yardım.
- **`vi [+SATIR] [dosya]`** (`vim`): komut, ekleme ve komut satırı kipleri. Hareket: `h j k l`, oklar, `0 ^ $`, `w b e`, `gg G`, `Ctrl-F/B/D/U`. Düzenleme: `x X dd D dw d$ dG yy Y yw p P J r ~ cc cw C s S`, sayı önekleri (`3dd`, `5j`), `u` ve `Ctrl-R`. Ekleme: `i a I A o O`, `Esc`. Arama: `/ ? n N` (sona gelince sarar). Komut satırı: `:w [dosya]`, `:q`, `:q!`, `:wq`, `:x`, `ZZ`, `ZQ`, `:e!`, `:N`, `:$`, `:s/a/b/g`, `:%s/a/b/g` (düz metin). Alt satır vim'deki gibi konumu (`satır,bayt-sütun`) ve oranı gösterir.

`hexdump` util-linux'unkiyle aynı çıktıyı verir: seçeneksiz (16 bitlik sözcükler), `-C` (kanonik, `hd` ile aynı), `-b -c -d -o -x`, `-n` ve `-s` (`0x`, sekizlik, `k`/`m` ekleri), `-v`; yinelenen satırlar `*` ile kısaltılır.

### Yılan (`snake`)

`snake` tam ekran bir yılan oyunudur. Oklar ya da WASD ile yön verilir; `p` duraklatır, `q` ya da Ctrl-C çıkar, çarpınca `r` yeniden başlatır. Her yem 10 puandır ve skor arttıkça yılan hızlanır. Rekor `/root/.yilan_rekor` dosyasında tutulur, ext4 kökte olduğu için açılışlar arasında kalır. `snake -o` otopilottur: yılan yeme doğru giderken taşkın doldurmayla sıkışacağı yollardan kaçınır; `p` ile duraklatılır, `q` ile çıkılır, yönü yılan seçtiği için ok tuşları yok sayılır. Ekran ANSI dizileriyle (`\e[satır;sütunH`, renkler, `\e[?25l`) ve yalnızca değişen hücreler çizilerek güncellenir. Bu yüzden oyun çerçeve tamponu konsolunda, VGA metin ekranında ve seri terminalde aynı şekilde çalışır.

## Grafik arayüz (X11 ve JWM)

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
| JWM | 2.4.3 | Xft, Xrender, PNG, JPEG, XPM, cairo, rsvg, fribidi, Xinerama, Xmu, shape ve NLS kapalı; X çekirdek yazı tipi `fixed` |

**Çalıştırma:** Uppix'te `startx` (kabuğu serbest bırakmak için `startx &`). [`startx`](kullanici/startx.c) Xfbdev'i `:0`'da başlatır (`-dumbSched -noreset`, evdev klavye ve fare), soket bağlantı kabul edince `DISPLAY=:0` ile JWM'yi çalıştırır. JWM bitince (menüde "Oturumu kapat" ya da seri konsoldan `jwm -exit`) X'i durdurur ve konsol metin kipine döner. `startx /usr/bin/xdeneme` JWM yerine tek bir istemci çalıştırır. Sunucunun iletileri `/tmp/Xfbdev.0.log`'a gider. Ekranı ve fareyi görmek için QEMU'yu pencereyle açın:

```bash
uppc derle .
qemu-system-x86_64 -m 256 -serial stdio -display gtk -drive file=derleme/uppix.img,format=raw
```

(`-display sdl` de olur.) Komutları seri terminalden yazarsınız; X açıkken klavye ve fare QEMU penceresindedir (fareyi pencereye tıklayarak yakalayın, Ctrl-Alt-G bırakır). JWM'de masaüstüne sol tıklamak ya da tepsideki "Uppix" düğmesi menüyü açar (test penceresi, JWM'yi yeniden başlat, oturumu kapat); Alt-Tab, Alt-F4 ve Alt-F2 çalışır. Ayarlar [`/etc/jwm/system.jwmrc`](kok/etc/jwm/system.jwmrc)'dedir.

**Tasarım kararları:**
- **`/bin/sh`:** X `xkbcomp`'u, JWM de menü komutlarını `/bin/sh -c` ile çalıştırır. Uppix'te `/bin/sh` çekirdekteki kabuğa giden bir yerleşik komuttur (`#!/yerlesik/sh`), süreç olarak çalıştırılamaz. Bu yüzden `execve` bu satırı görünce, `/usr/bin/ush` varsa onu aynı argv ile çalıştırır. [`ush`](kullanici/ush.c), libc'siz ~330 satırlık bir C kabuğudur. Seçenekler şunlardı: busybox (yeni bir kaynak), xkbcomp'u X'e gömmek (sunucuyu yamalamak) ya da önceden derlenmiş `.xkm` (X düzen her değiştiğinde yine xkbcomp çağırır). En küçük ve Linux'a en sadık seçenek ush'tı. JWM'nin `StartupCommand`'ı da ush ile `echo` çalıştırıp "jwm: masaüstü hazır" yazar (testler bunu bekler).
- **`startx` bir C programıdır:** uppsh'te döngü olmadığından "soket kabul edene dek bekle" betikle yazılamaz.
- **Gömülü yazı tipleri:** libXfont2, `fixed` ve `cursor` yazı tiplerini gzip'li PCF olarak tutar ve zlib ile açar. Yeni bir kaynak indirmemek için zlib, açma işlevleri başarısız dönen bir yer tutucudur ([`ui/yama/zlib`](ui/yama/zlib)); [`gomulu_ac.py`](ui/yama/libxfont2/gomulu_ac.py) dizileri derleme sırasında açar. X'in istediği SHA-1 için de küçük bir uygulama vardır ([`ui/yama/libsha1`](ui/yama/libsha1)).
- **pkg-config:** [`ui/pkg-config`](ui/pkg-config) (Python), statik bağlamada `Requires.private` zincirini bağımlılık sırasıyla açar ve hazırlama kökünü yollara ekler.
- **XKB kuralları:** xkeyboard-config'in `rules` dosyaları xsltproc/perl ile üretilir. Onların yerine elle yazılmış kısa bir `evdev` kuralı ([`ui/xkb/evdev`](ui/xkb/evdev)) kullanılır: pc105, `us` düzeni.

**Sınırlar:** Tek ekran ve tek kip vardır (Limine'nin verdiği çerçeve tamponu, ör. 1280x800x32); `FBIOPUT_VSCREENINFO` yalnızca geçerli kipi kabul eder. VT geçişi (Ctrl-Alt-F*) yok. Yazı tipleri yalnızca `fixed` ve `cursor`'dır. `link` kopyalar, sahiplik tutulmaz, her süreç root'tur. X ve istemciler AF_UNIX ile konuşur (TCP yok); dosya yolu soketi yalnızca bellekteki dosya sistemlerinde kurulabilir (`/tmp`). İşbirlikçi çekirdekte bekleyen çağrılar 1 ms'lik adımlarla yokladığından her X gidiş-dönüşü bir zamanlayıcı adımı sürer: JWM'nin açılışı yaklaşık 5 saniyedir. ush'ta boru (`|`) yoktur.

## Mimari

| Dosya | İçerik |
|---|---|
| `kaynak/uppix.upp` | Açılış: çekirdek günlüğü, disk tarama, kök bağlama, sanal dosya sistemleri, `/etc`, komut tablosu, oturum |
| `kaynak/vfs.upp` | VFS: düğüm önbelleği (`sinif Dugum`), bağlama tablosu, sürücü arayüzü (`sinif Surucu`), yol çözme, `/proc` |
| `kaynak/veriyolu.upp` | Aygıt ağacı ve sürücü çatısı (FreeBSD newbus): aygıtlar, sürücü kaydı (`DRIVER_MODULE`), yoklama/bağlama/ayırma, `nexus`, PCI veri yolu taraması ve BAR'lar, `hostb`/`isab`, MMIO (bus_space) ve DMA (bus_dma) kaynak işlevleri |
| `kaynak/blok.upp` | Blok aygıtları: disk kaydı (sürücülerin okuma/yazma fonksiyon değerleri), MBR/GPT bölümleri, dosya sistemi tanıma |
| `kaynak/ata.upp` | ATA sürücüleri: PCI IDE denetleyicisi (`atapci`) ve kanalları (`ata`); diskleri disk kaydına koyar |
| `kaynak/girdi.upp` | Girdi: `atkbdc`/`atkbd`/`psm` sürücüleri, IntelliMouse algılama ve paket çözme, tarama kodu → `KEY_*`, açılış başına olay kuyrukları (evdev, mousedev), konsol kipleri (KD/K/VT) |
| `kaynak/sistem_cagrisi_soket.upp` | AF_UNIX soketleri: soket nesnesi, adresler (yol, soyut, adsız), akış halkası ve datagram kuyruğu, `SCM_RIGHTS`, seçenekler, `socket`…`recvmsg` |
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
| `kullanici/*.c` | Ring 3 programları (libc'siz statik Linux ELF); `linux.h` ortak başlık; `kullanici/glibc/*.c` glibc ile statik; `kullanici/x11/*.c` libX11 ile (musl, `ui/derle.sh`) |
| `ui/` | Grafik arayüz: `derle.sh` (X.Org Xfbdev, xkbcomp, JWM), `pkg-config`, `yama/` (zlib yer tutucusu, libsha1, gömülü yazı tiplerini açma), `xkb/evdev` |
| `kaynak/yardimci.upp` | Sıralama, biçimleme, joker eşleme, takvim/Unix zamanı |

Her dosya sistemi sürücüsü fonksiyon değerlerinden oluşan bir `sinif Surucu`'dur: `bagla`, `yukle` (dizini önbelleğe al), `oku`, `yaz`, `olustur`, `sil`, `tasi`, `oznitelik`, `alan`. VFS hangi düğümün hangi bağlamaya ait olduğunu bilir ve doğru sürücüyü çağırır. Dizinler ilk erişimde yüklenir. Komutlar dosya sisteminden habersizdir; `cp /root/a.txt /mnt/sdc/` gibi diskler arası kopyalama aynı kodla çalışır.

İkili veri `upp.tampon.*` ile işlenir. Bunlar sınır denetimli bayt tamponlarıdır: küçük sonlu okuma/yazma, UTF-16, CRC32C. Diske blok katmanı disk kaydı üzerinden erişir; ATA sürücüsü kayda çekirdeğin `upp.cekirdek.disk_oku/disk_yaz` PIO mekanizmasını, AHCI sürücüsü kendi DMA okuma/yazmasını (u++'ta, MMIO pencereleri ve `upp.cekirdek.tampon_fiziksel` ile) koyar. Sürücüler PCI yapılandırma alanını `upp.cekirdek.pci_oku` ile okur; yapılandırma yazma ve BAR penceresi açma (`pci_yaz`, `mmio_ac`) `guvensiz` gerektirir ve yalnızca `veriyolu.upp`'deki iki yerde yapılır, pencere içi erişim (`mmio_oku/yaz`) sınır denetimlidir. Ayrıntılar için [U++ API belgesine](https://github.com/ploxdev/upp-lang/blob/main/docs/API_tr.md) bakın.

### Testler

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
| `tests/cekirdek/14_uppix_x11.upp` | Grafik arayüz (`--qemu-izleyici`): `@hazirla` `ui/derle.sh`'i çalıştırır (önbellekten hızlı); `startx`, JWM'nin hazır satırı, `xdeneme` penceresi, izleyiciden `sendkey`/`mouse_move`/`mouse_button`; `screendump`'larda pencere, dikdörtgen, masaüstü ve JWM tepsisi pikselleri; `jwm -exit`'ten sonra konsol `KD_TEXT`/`K_XLATE`'te ve ekranda metin; sonra `e2fsck` |

## Sınırlar ve sıradakiler

- ext4 yazımı günlük (journal) kullanmaz. Dosya en çok 4 extent'te tutulur, yani boş alan çok parçalıysa büyük dosya yazılamaz. htree ile dizinlenmiş büyük dizinlere ekleme yapılmaz. Grup bitmap'i başlatılmamış (`*_UNINIT`) gruplar ayırmada atlanır.
- FAT12/FAT16 ve 512 bayt dışı sektör boyutları desteklenmez. Önbellek yoktur: her işlem diske gider.
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
