/* libsha1 arabirimi (xorg-server --with-sha1=libsha1 bunu kullanır): RFC 3174 SHA-1.
 * Uppix grafik arayüzü derlemesi için yazılmış küçük bir uygulama. */
#ifndef UPPIX_LIBSHA1_H
#define UPPIX_LIBSHA1_H

#include <stddef.h>
#include <stdint.h>

#define SHA1_DIGEST_SIZE 20
#define SHA1_BLOCK_SIZE 64

typedef struct {
    uint32_t h[5];
    uint64_t uzunluk;
    unsigned char blok[SHA1_BLOCK_SIZE];
    size_t dolu;
} sha1_ctx;

void sha1_begin(sha1_ctx* ctx);
void sha1_hash(const unsigned char* veri, unsigned long n, sha1_ctx* ctx);
void sha1_end(unsigned char* sonuc, sha1_ctx* ctx);

#endif
