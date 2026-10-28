#!/bin/sh
# Uppix grafik arayüzü: musl ile statik Xfbdev (xorg-server kdrive), xkbcomp, JWM ve
# kullanici/x11 altındaki libX11 programları (uterm, ayarlar, oturum, xdeneme). Linux'ta ya
# da WSL'de çalışır:
#
#   sh ui/derle.sh
#
# Kaynak arşivleri x.org, musl.libc.org ve github.com/joewing/jwm'den indirilir, aşağıdaki
# sha256 özetleriyle doğrulanır ve derleme/ui/indir'de saklanır. Derleme WSL'in kendi dosya
# sisteminde ($UPPIX_UI_IS, varsayılan ~/.cache/uppix-ui) yapılır: paketler --prefix=/usr
# ile derlenip DESTDIR ile bir hazırlama köküne kurulur. Uppix'e girecek dosyalar
# derleme/ui/kur altına kopyalanır (disk_yap.sh oradan alır). Yeniden çalıştırılınca biten
# adımlar atlanır; sonuç güncelse hemen çıkar.
#
# Yerine-koyucular (ui/yama): pkg-config (Python), libsha1 (xorg-server'ın SHA1'i), zlib
# (yalnızca gömülü yazı tipleri kullanıldığından sıkıştırma hiç gerekmez) ve
# pthread-stubs.pc. XKB kural dosyası ui/xkb/evdev'dir.
set -e
BURA=$(cd "$(dirname "$0")/.." && pwd)
UI="$BURA/derleme/ui"
INDIR="$UI/indir"
KUR="$UI/kur"
IS=${UPPIX_UI_IS:-$HOME/.cache/uppix-ui}
KOK="$IS/kok"
J=$(nproc 2>/dev/null || echo 4)
SURUM=1
mkdir -p "$INDIR" "$IS/kaynak" "$IS/damga" "$IS/gunluk" "$KOK"

# ad  dosya  url  sha256
PAKETLER="
musl musl-1.2.5.tar.gz https://musl.libc.org/releases/musl-1.2.5.tar.gz a9a118bbe84d8764da0ea0d28b3ab3fae8477fc7e4085d90102b8596fc7c75e4
xorgproto xorgproto-2018.4.tar.bz2 https://www.x.org/releases/individual/proto/xorgproto-2018.4.tar.bz2 fee885e0512899ea5280c593fdb2735beb1693ad170c22ebcc844470eec415a0
libXau libXau-1.0.9.tar.bz2 https://www.x.org/releases/individual/lib/libXau-1.0.9.tar.bz2 ccf8cbf0dbf676faa2ea0a6d64bcc3b6746064722b606c8c52917ed00dcb73ec
xcb-proto xcb-proto-1.14.1.tar.xz https://www.x.org/releases/individual/proto/xcb-proto-1.14.1.tar.xz f04add9a972ac334ea11d9d7eb4fc7f8883835da3e4859c9afa971efdf57fcc3
libxcb libxcb-1.14.tar.xz https://www.x.org/releases/individual/lib/libxcb-1.14.tar.xz a55ed6db98d43469801262d81dc2572ed124edc3db31059d4e9916eb9f844c34
xtrans xtrans-1.4.0.tar.bz2 https://www.x.org/releases/individual/lib/xtrans-1.4.0.tar.bz2 377c4491593c417946efcd2c7600d1e62639f7a8bbca391887e2c4679807d773
libX11 libX11-1.6.12.tar.bz2 https://www.x.org/releases/individual/lib/libX11-1.6.12.tar.bz2 f108227469419ac04d196df0f3b80ce1f7f65059bb54c0de811f4d8e03fd6ec7
libXext libXext-1.3.4.tar.bz2 https://www.x.org/releases/individual/lib/libXext-1.3.4.tar.bz2 59ad6fcce98deaecc14d39a672cf218ca37aba617c9a0f691cac3bcd28edf82b
pixman pixman-0.38.4.tar.bz2 https://www.x.org/releases/individual/lib/pixman-0.38.4.tar.bz2 84abb7fa2541af24d9c3b34bf75d6ac60cc94ac4410061bbb295b66a29221550
libfontenc libfontenc-1.1.4.tar.bz2 https://www.x.org/releases/individual/lib/libfontenc-1.1.4.tar.bz2 2cfcce810ddd48f2e5dc658d28c1808e86dcf303eaff16728b9aa3dbc0092079
libXfont2 libXfont2-2.0.4.tar.bz2 https://www.x.org/releases/individual/lib/libXfont2-2.0.4.tar.bz2 6d151b3368e5035efede4b6264c0fdc6662c1c99dbc2de425e3480cababc69e6
libxkbfile libxkbfile-1.1.0.tar.bz2 https://www.x.org/releases/individual/lib/libxkbfile-1.1.0.tar.bz2 758dbdaa20add2db4902df0b1b7c936564b7376c02a0acd1f2a331bd334b38c7
xkbcomp xkbcomp-1.4.4.tar.bz2 https://www.x.org/releases/individual/app/xkbcomp-1.4.4.tar.bz2 59cce603a607a17722a0a1cf99010f4894e7812beb5d695abbc08474d59af27e
xkeyboard-config xkeyboard-config-2.23.1.tar.bz2 https://www.x.org/releases/individual/data/xkeyboard-config/xkeyboard-config-2.23.1.tar.bz2 2a4bbc05fea22151b7a7c8ac2655d549aa9b0486bedc7f5a68c72716343b02f3
xorg-server xorg-server-1.19.7.tar.bz2 https://www.x.org/releases/individual/xserver/xorg-server-1.19.7.tar.bz2 7112f7128a4f5b06ceb8bba1bdc5e5c9e0fae682a42d35218bc12ba693f4c80c
jwm jwm-2.4.3.tar.xz https://github.com/joewing/jwm/releases/download/v2.4.3/jwm-2.4.3.tar.xz ee3b4ee0c452ef31fcb9303ab50aaf496cf5bdf7b5f1fdc9a1251b7175ca67ab
"

bilgi() { echo "[ui] $*"; }

paket_alan() { echo "$PAKETLER" | awk -v a="$1" -v n="$2" '$1 == a { print $n }'; }

# Arşivi indirir (yoksa) ve özetini doğrular.
indir() {
    dosya=$(paket_alan "$1" 2)
    url=$(paket_alan "$1" 3)
    ozet=$(paket_alan "$1" 4)
    if [ ! -f "$INDIR/$dosya" ]; then
        bilgi "indiriliyor: $url"
        curl -fsSL -o "$INDIR/$dosya.yarim" "$url"
        mv "$INDIR/$dosya.yarim" "$INDIR/$dosya"
    fi
    gercek=$(sha256sum "$INDIR/$dosya" | cut -d' ' -f1)
    if [ "$gercek" != "$ozet" ]; then
        echo "[ui] HATA: $dosya özeti uyuşmuyor ($gercek)" >&2
        exit 1
    fi
}

# Arşivi temiz bir kaynak dizinine açar; dizin adını yazar.
ac() {
    dosya=$(paket_alan "$1" 2)
    dizin=$(echo "$dosya" | sed 's/\.tar\..*//')
    rm -rf "$IS/kaynak/$dizin"
    tar -xf "$INDIR/$dosya" -C "$IS/kaynak"
    echo "$IS/kaynak/$dizin"
}

bitti_mi() { [ -f "$IS/damga/$1" ]; }
bitir() { touch "$IS/damga/$1"; }

# Adımı günlüğe yazarak çalıştırır; hata olursa günlüğün sonu gösterilir.
adim() {
    ad=$1
    shift
    if ! "$@" > "$IS/gunluk/$ad.txt" 2>&1; then
        echo "[ui] HATA: $ad başarısız; son satırlar ($IS/gunluk/$ad.txt):" >&2
        tail -n 40 "$IS/gunluk/$ad.txt" >&2
        exit 1
    fi
}

for ad in $(echo "$PAKETLER" | awk 'NF { print $1 }'); do indir "$ad"; done

# ---- sonuç güncel mi ----
ozet_hepsi=$( (echo "$PAKETLER" "$SURUM"; cat "$0" "$BURA"/ui/pkg-config "$BURA"/ui/yama/*/* "$BURA"/ui/xkb/* "$BURA"/kullanici/x11/*.c "$BURA"/kullanici/x11/*.h 2>/dev/null) | sha256sum | cut -d' ' -f1)
if [ -f "$KUR/.ozet" ] && [ "$(cat "$KUR/.ozet")" = "$ozet_hepsi" ]; then
    bilgi "güncel: $KUR"
    exit 0
fi

# ---- musl ----
MUSL="$IS/musl"
musl_derle() {
    d=$(ac musl) || return 1
    cd "$d" || return 1
    CC=gcc ./configure --prefix="$MUSL" --disable-shared || return 1
    make -j"$J" || return 1
    make install || return 1
    # musl çekirdek başlıklarını içermez: Linux'un libc'den bağımsız başlıkları bağlanır
    for b in linux asm-generic mtd; do [ -d /usr/include/$b ] && ln -sfn /usr/include/$b "$MUSL/include/$b"; done
    asm=/usr/include/asm
    [ -d /usr/include/x86_64-linux-gnu/asm ] && asm=/usr/include/x86_64-linux-gnu/asm
    ln -sfn "$asm" "$MUSL/include/asm"
}
if ! bitti_mi musl; then
    bilgi "musl"
    adim musl musl_derle
    bitir musl
fi

# ---- ortam ----
mkdir -p "$IS/arac"
cp "$BURA/ui/pkg-config" "$IS/arac/pkg-config"
chmod +x "$IS/arac/pkg-config"
export PATH="$IS/arac:$MUSL/bin:$PATH"
export CC="musl-gcc"
# GCC 14+ eski X kodundaki bazı uyarıları varsayılan olarak hata sayar; C23 yerine gnu11
export CFLAGS="-O2 -fno-pie -fno-stack-protector -std=gnu11 -Wno-error=incompatible-pointer-types -Wno-error=int-conversion -Wno-error=implicit-function-declaration -Wno-error=array-bounds -Wno-error=address -Wno-error=nonnull -Wno-error=int-to-pointer-cast -Wno-error=pointer-to-int-cast"
export CPPFLAGS="-I$KOK/usr/include"
export LDFLAGS="-static -no-pie -L$KOK/usr/lib"
export PKG_CONFIG="$IS/arac/pkg-config"
export PKG_CONFIG_PATH=
export PKG_CONFIG_LIBDIR="$KOK/usr/lib/pkgconfig:$KOK/usr/share/pkgconfig"
export PKG_CONFIG_SYSROOT_DIR="$KOK"
export PYTHON=python3

KURULUM_EK=

# musl uyarlamaları (kaynak açıldıktan sonra, configure'dan önce).
kaynak_duzelt() {
    case "$1" in
        libXfont2)
            # gömülü yazı tipleri gzip'li saklanır; zlib yerine-koyucu olduğundan derlemede
            # çözülür ve açılırken sıkıştırma katmanı atlanır
            python3 "$BURA/ui/yama/libxfont2/gomulu_ac.py" src/builtins/fonts.c || return 1
            sed -i 's/if ((cooked = BufFilePushZIP (raw)))/if (0 \&\& (cooked = BufFilePushZIP (raw)))/' src/builtins/file.c || return 1
            grep -q "if (0 && (cooked = BufFilePushZIP" src/builtins/file.c || return 1
            ;;
        jwm)
            # çekirdek yazı tipi ISO 8859-1: metin çizimden önce UTF-8'den çevrilir
            python3 "$BURA/ui/yama/jwm/latin1.py" src/font.c || return 1
            ;;
        xorg-server)
            # kdrive glibc'ye özgü __uid_t/__gid_t türlerini kullanır
            sed -i 's/__uid_t/uid_t/g; s/__gid_t/gid_t/g' hw/kdrive/linux/linux.c || return 1
            ;;
    esac
    return 0
}

# configure'dan sonraki uyarlamalar.
configure_sonrasi() {
    case "$1" in
        jwm)
            # JWM kütüphaneleri LDFLAGS'e koyar (-lX11 ... -lXext); statik bağlamada libXext'ten
            # sonra yine libX11, onun da ardından xcb gerekir
            sed -i 's/^LDFLAGS = \(.*\)$/LDFLAGS = \1 -lX11 -lxcb -lXau/' src/Makefile || return 1
            ;;
    esac
    return 0
}

# Autotools paketi: --prefix=/usr, statik, DESTDIR ile hazırlama köküne.
autotools() {
    ad=$1
    shift
    d=$(ac "$ad") || return 1
    cd "$d" || return 1
    kaynak_duzelt "$ad" || return 1
    ./configure --prefix=/usr --sysconfdir=/etc --localstatedir=/var --disable-shared --enable-static "$@" || return 1
    configure_sonrasi "$ad" || return 1
    make -j"$J" || return 1
    make install DESTDIR="$KOK" $KURULUM_EK || return 1
    rm -f "$KOK"/usr/lib/*.la
}

# Damga, paketin seçeneklerinin ve yamalarının (ui/yama/<paket>) özetini tutar: biri
# değişince paket yeniden derlenir. Boş damga (eski biçim) geçerli sayılır.
paket() {
    ad=$1
    shift
    imza=$( (printf '%s\n' "$@"; cat "$BURA"/ui/yama/"$ad"/* 2>/dev/null) | sha256sum | cut -c1-16)
    if bitti_mi "$ad" && { [ ! -s "$IS/damga/$ad" ] || [ "$(cat "$IS/damga/$ad")" = "$imza" ]; }; then return 0; fi
    bilgi "$ad"
    adim "$ad" autotools "$ad" "$@"
    echo "$imza" > "$IS/damga/$ad"
}

# yerine-koyucular: libsha1, zlib, pthread-stubs
yamalar() {
    mkdir -p "$KOK/usr/include" "$KOK/usr/lib/pkgconfig" "$IS/yama"
    for y in libsha1 zlib; do
        cp "$BURA/ui/yama/$y/"*.h "$KOK/usr/include/" || return 1
        musl-gcc $CFLAGS -c "$BURA/ui/yama/$y/$y.c" -o "$IS/yama/$y.o" || return 1
    done
    ar rcs "$KOK/usr/lib/libsha1.a" "$IS/yama/libsha1.o" || return 1
    ar rcs "$KOK/usr/lib/libz.a" "$IS/yama/zlib.o" || return 1
    printf 'prefix=/usr\nlibdir=${prefix}/lib\nincludedir=${prefix}/include\n\nName: libsha1\nDescription: SHA-1 (Uppix)\nVersion: 0.3\nLibs: -L${libdir} -lsha1\nCflags: -I${includedir}\n' > "$KOK/usr/lib/pkgconfig/libsha1.pc"
    printf 'prefix=/usr\nlibdir=${prefix}/lib\nincludedir=${prefix}/include\n\nName: zlib\nDescription: zlib yerine-koyucusu (Uppix)\nVersion: 1.2.13\nLibs: -L${libdir} -lz\nCflags: -I${includedir}\n' > "$KOK/usr/lib/pkgconfig/zlib.pc"
    printf 'Name: pthread stubs\nDescription: musl pthread işlevlerini içerir\nVersion: 0.4\nCflags:\nLibs:\n' > "$KOK/usr/lib/pkgconfig/pthread-stubs.pc"
}
if ! bitti_mi yama; then
    bilgi "yerine-koyucular"
    adim yama yamalar
    bitir yama
fi

paket xorgproto
paket libXau
# eski automake py-compile'ı Python 3.12+'da yok olan imp modülünü ister: bayt derlemesi atlanır
KURULUM_EK="PYTHON=true"
paket xcb-proto
KURULUM_EK=
paket libxcb --disable-devel-docs --without-doxygen
paket xtrans
paket libX11 --disable-specs --without-xmlto --without-fop --disable-loadable-i18n --disable-xf86bigfont
paket libXext --disable-specs --without-xmlto --without-fop
paket pixman --disable-gtk --disable-libpng --disable-openmp
paket libfontenc
paket libXfont2 --disable-devel-docs --disable-freetype --disable-ipv6 --disable-fc --disable-bdfformat --disable-snfformat
paket libxkbfile
paket xkbcomp --with-xkb-config-root=/usr/share/X11/xkb

# XKB verisi: kurallar elle (ui/xkb), tanım dizinleri xkeyboard-config'ten
xkb_veri() {
    d=$(ac xkeyboard-config) || return 1
    x="$KOK/usr/share/X11/xkb"
    rm -rf "$x"
    mkdir -p "$x/rules" || return 1
    for alt in keycodes types compat symbols geometry; do
        cp -R "$d/$alt" "$x/$alt" || return 1
        find "$x/$alt" \( -name 'Makefile*' -o -name '*.in' \) -delete
    done
    cp "$BURA/ui/xkb/evdev" "$x/rules/evdev" || return 1
    cp "$BURA/ui/xkb/evdev" "$x/rules/base" || return 1
}
if ! bitti_mi xkb; then
    bilgi "xkb verisi"
    adim xkb xkb_veri
    bitir xkb
fi

paket xorg-server \
    --disable-xorg --disable-dmx --disable-xvfb --disable-xnest --disable-xwayland --disable-xquartz --disable-xwin \
    --enable-kdrive --enable-xfbdev --disable-xephyr --disable-xfake \
    --enable-kdrive-evdev --enable-kdrive-kbd --enable-kdrive-mouse \
    --disable-glx --disable-dri --disable-dri2 --disable-dri3 --disable-present --disable-glamor --disable-libdrm \
    --disable-config-udev --disable-config-udev-kms --disable-config-hal --disable-systemd-logind --disable-pciaccess \
    --disable-input-thread --disable-mitshm --disable-xinerama --disable-xselinux --disable-xcsecurity \
    --disable-xdmcp --disable-xdm-auth-1 --disable-secure-rpc --disable-ipv6 --disable-tcp-transport \
    --disable-unit-tests --disable-docs --disable-devel-docs --disable-strict-compilation \
    --with-sha1=libsha1 --with-default-font-path=built-ins --with-fontrootdir=/usr/share/fonts/X11 \
    --with-xkb-path=/usr/share/X11/xkb --with-xkb-bin-directory=/usr/bin --with-xkb-output=/tmp \
    --with-default-xkb-rules=evdev --with-default-xkb-model=pc105 --with-default-xkb-layout=us

paket jwm LIBS="-lX11 -lxcb -lXau" --x-includes="$KOK/usr/include" --x-libraries="$KOK/usr/lib" --disable-xft --disable-xrender --disable-png --disable-jpeg --disable-xpm --disable-cairo \
    --disable-rsvg --disable-fribidi --disable-xinerama --disable-xmu --disable-nls --disable-confirm \
    --sysconfdir=/etc/jwm

# kullanici/x11/*.c: libX11 programları (ortak başlık arayuz.h)
istemci_derle() {
    for c in "$BURA"/kullanici/x11/*.c; do
        musl-gcc $CFLAGS $CPPFLAGS -o "$IS/$(basename "$c" .c)" "$c" $LDFLAGS -lX11 -lxcb -lXau || return 1
    done
}
bilgi "X programları (uterm, ayarlar, oturum, xdeneme)"
adim istemci istemci_derle

# ---- Uppix'e girecekler ----
rm -rf "$KUR"
mkdir -p "$KUR/usr/bin" "$KUR/usr/share/X11"
for p in Xfbdev xkbcomp jwm; do
    cp "$KOK/usr/bin/$p" "$KUR/usr/bin/$p"
    strip "$KUR/usr/bin/$p" 2>/dev/null || true
done
for c in "$BURA"/kullanici/x11/*.c; do
    p=$(basename "$c" .c)
    cp "$IS/$p" "$KUR/usr/bin/$p"
    strip "$KUR/usr/bin/$p" 2>/dev/null || true
done
cp -R "$KOK/usr/share/X11/xkb" "$KUR/usr/share/X11/xkb"
echo "$ozet_hepsi" > "$KUR/.ozet"
bilgi "hazır: $KUR ($(du -sh "$KUR" | cut -f1))"
