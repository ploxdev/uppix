/* zlib yerine-koyucusu (bkz. zlib.h): bütün işlevler başarısızdır. */
#include "zlib.h"

#include <errno.h>
#include <stddef.h>

int inflateInit(z_streamp strm) {
    (void)strm;
    return Z_STREAM_ERROR;
}

int inflateInit2(z_streamp strm, int pencere) {
    (void)strm;
    (void)pencere;
    return Z_STREAM_ERROR;
}

int inflate(z_streamp strm, int flush) {
    (void)strm;
    (void)flush;
    return Z_STREAM_ERROR;
}

int inflateEnd(z_streamp strm) {
    (void)strm;
    return Z_OK;
}

int inflateReset(z_streamp strm) {
    (void)strm;
    return Z_STREAM_ERROR;
}

gzFile gzopen(const char* yol, const char* kip) {
    (void)yol;
    (void)kip;
    errno = ENOSYS;
    return NULL;
}

gzFile gzdopen(int fd, const char* kip) {
    (void)fd;
    (void)kip;
    errno = ENOSYS;
    return NULL;
}

int gzread(gzFile f, void* tampon, unsigned n) {
    (void)f;
    (void)tampon;
    (void)n;
    return -1;
}

int gzgetc(gzFile f) {
    (void)f;
    return -1;
}

int gzclose(gzFile f) {
    (void)f;
    return Z_STREAM_ERROR;
}

int gzeof(gzFile f) {
    (void)f;
    return 1;
}

const char* zlibVersion(void) { return ZLIB_VERSION; }
