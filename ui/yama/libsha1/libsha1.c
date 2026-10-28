/* SHA-1 (RFC 3174): libsha1 arabirimiyle. */
#include "libsha1.h"

#include <string.h>

static uint32_t dondur(uint32_t x, int n) { return (x << n) | (x >> (32 - n)); }

static void sha1_blok(sha1_ctx* ctx, const unsigned char* p) {
    uint32_t w[80], a, b, c, d, e, f, k, t;
    int i;
    for (i = 0; i < 16; i++) {
        w[i] = ((uint32_t)p[i * 4] << 24) | ((uint32_t)p[i * 4 + 1] << 16) | ((uint32_t)p[i * 4 + 2] << 8) | p[i * 4 + 3];
    }
    for (i = 16; i < 80; i++) w[i] = dondur(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
    a = ctx->h[0];
    b = ctx->h[1];
    c = ctx->h[2];
    d = ctx->h[3];
    e = ctx->h[4];
    for (i = 0; i < 80; i++) {
        if (i < 20) {
            f = (b & c) | (~b & d);
            k = 0x5A827999u;
        } else if (i < 40) {
            f = b ^ c ^ d;
            k = 0x6ED9EBA1u;
        } else if (i < 60) {
            f = (b & c) | (b & d) | (c & d);
            k = 0x8F1BBCDCu;
        } else {
            f = b ^ c ^ d;
            k = 0xCA62C1D6u;
        }
        t = dondur(a, 5) + f + e + k + w[i];
        e = d;
        d = c;
        c = dondur(b, 30);
        b = a;
        a = t;
    }
    ctx->h[0] += a;
    ctx->h[1] += b;
    ctx->h[2] += c;
    ctx->h[3] += d;
    ctx->h[4] += e;
}

void sha1_begin(sha1_ctx* ctx) {
    ctx->h[0] = 0x67452301u;
    ctx->h[1] = 0xEFCDAB89u;
    ctx->h[2] = 0x98BADCFEu;
    ctx->h[3] = 0x10325476u;
    ctx->h[4] = 0xC3D2E1F0u;
    ctx->uzunluk = 0;
    ctx->dolu = 0;
}

void sha1_hash(const unsigned char* veri, unsigned long n, sha1_ctx* ctx) {
    ctx->uzunluk += n;
    while (n > 0) {
        size_t k = SHA1_BLOCK_SIZE - ctx->dolu;
        if (k > n) k = n;
        memcpy(ctx->blok + ctx->dolu, veri, k);
        ctx->dolu += k;
        veri += k;
        n -= k;
        if (ctx->dolu == SHA1_BLOCK_SIZE) {
            sha1_blok(ctx, ctx->blok);
            ctx->dolu = 0;
        }
    }
}

void sha1_end(unsigned char* sonuc, sha1_ctx* ctx) {
    uint64_t bit = ctx->uzunluk * 8;
    unsigned char dolgu = 0x80;
    unsigned char sifir = 0;
    unsigned char boy[8];
    int i;
    sha1_hash(&dolgu, 1, ctx);
    while (ctx->dolu != 56) sha1_hash(&sifir, 1, ctx);
    for (i = 0; i < 8; i++) boy[i] = (unsigned char)(bit >> (56 - 8 * i));
    sha1_hash(boy, 8, ctx);
    for (i = 0; i < 20; i++) sonuc[i] = (unsigned char)(ctx->h[i / 4] >> (24 - 8 * (i % 4)));
}
