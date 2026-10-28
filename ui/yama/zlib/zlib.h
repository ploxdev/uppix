/* zlib yerine-koyucusu: Uppix grafik arayüzünde yazı tipleri yalnızca libXfont2'nin
 * gömülü (built-ins) yazı tipleridir; sıkıştırılmış dosya hiç açılmaz. libfontenc ve
 * libXfont2'nin kullandığı işlevler vardır ama her zaman başarısız döner. */
#ifndef UPPIX_ZLIB_H
#define UPPIX_ZLIB_H

#define ZLIB_VERSION "1.2.13-uppix"
#define Z_OK 0
#define Z_STREAM_END 1
#define Z_NEED_DICT 2
#define Z_ERRNO (-1)
#define Z_STREAM_ERROR (-2)
#define Z_DATA_ERROR (-3)
#define Z_MEM_ERROR (-4)
#define Z_BUF_ERROR (-5)
#define Z_NO_FLUSH 0
#define Z_SYNC_FLUSH 2
#define Z_FINISH 4
#define Z_DEFLATED 8
#define Z_NULL 0
#define MAX_WBITS 15

typedef unsigned char Bytef;
typedef unsigned int uInt;
typedef unsigned long uLong;
typedef void* voidpf;
typedef voidpf (*alloc_func)(voidpf opaque, uInt items, uInt size);
typedef void (*free_func)(voidpf opaque, voidpf address);

typedef struct z_stream_s {
    Bytef* next_in;
    uInt avail_in;
    uLong total_in;
    Bytef* next_out;
    uInt avail_out;
    uLong total_out;
    char* msg;
    void* state;
    alloc_func zalloc;
    free_func zfree;
    voidpf opaque;
    int data_type;
    uLong adler;
    uLong reserved;
} z_stream;

typedef z_stream* z_streamp;
typedef struct gzFile_s* gzFile;

int inflateInit(z_streamp strm);
int inflateInit2(z_streamp strm, int pencere);
int inflate(z_streamp strm, int flush);
int inflateEnd(z_streamp strm);
int inflateReset(z_streamp strm);
gzFile gzopen(const char* yol, const char* kip);
gzFile gzdopen(int fd, const char* kip);
int gzread(gzFile f, void* tampon, unsigned n);
int gzgetc(gzFile f);
int gzclose(gzFile f);
int gzeof(gzFile f);
const char* zlibVersion(void);

#endif
