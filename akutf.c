#include "akutf.h"

#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

typedef enum u8_decoder_state {
    U8_DECODE_OK = 0,
    U8_DECODE_FINISH = 1,
    U8_DECODE_ERROR = 2,
    U8_DECODE_CONTINUE = 3,
} u8_decoder_state;

typedef struct u8_decoder_ctx {
    /* UTF-8 code point */
    uint32_t codepoint;
    /* UTF-8 bytes seen */
    size_t seen;
    /* UTF-8 bytes needed */
    size_t need;
    /* UTF-8 lower boundary */
    size_t lower;
    /* UTF-8 upper boundary */
    size_t upper;

    /* State */
    u8_decoder_state state;

    /* Pointer to last item in string to decode */
    const char *saveptr;
} *u8_decoder_ctx;

u8_decoder_ctx u8_decode_ctx(void) {
    u8_decoder_ctx ctx = calloc(1, sizeof(*ctx));
    if (!ctx)
        return NULL;

    ctx->lower = 0x80;
    ctx->upper = 0xBF;
    ctx->saveptr = NULL;

    return ctx;
}

uint32_t u8_decode_once(u8_decoder_ctx ctx, const char *src) {
    if (!ctx->saveptr)
        ctx->saveptr = src;
    uint8_t byte = *ctx->saveptr;
    if (byte != 0x0)
        ctx->saveptr++;

    if (byte == 0x0 && ctx->need > 0) {
        ctx->need = 0;
        ctx->state = U8_DECODE_ERROR;
        return 0;
    } else if (byte == 0x0) {
        ctx->state = U8_DECODE_FINISH;
        return 0;
    }

    if (ctx->need == 0) {
        if (byte > 0x0 && byte <= 0x7F) {
            ctx->state = U8_DECODE_OK;
            return byte;
        } else if (byte >= 0xC2 && byte <= 0xDF) {
            ctx->need = 1;
            ctx->codepoint = byte & 0x1F;
        } else if (byte >= 0xE0 && byte <= 0xEF) {
            if (byte == 0xE0)
                ctx->lower = 0xA0;
            else if (byte == 0xED)
                ctx->upper = 0x9F;

            ctx->need = 2;
            ctx->codepoint = byte & 0xF;
        } else if (byte >= 0xF0 && byte <= 0xF4) {
            if (byte == 0xF0)
                ctx->lower = 0x90;
            else if (byte == 0xF4)
                ctx->upper = 0x8F;

            ctx->need = 3;
            ctx->codepoint = byte & 0x7;
        } else {
            ctx->state = U8_DECODE_ERROR;
            return 0;
        }

        ctx->state = U8_DECODE_CONTINUE;
        return 0;
    }

    if (byte < ctx->lower || byte > ctx->upper) {
        ctx->codepoint = ctx->need = ctx->seen = 0;
        ctx->lower = 0x80;
        ctx->upper = 0xBF;

        ctx->saveptr--;
        ctx->state = U8_DECODE_ERROR;
        return 0;
    }

    ctx->lower = 0x80;
    ctx->upper = 0xBF;
    ctx->codepoint = (ctx->codepoint << 6) | (byte & 0x3F);
    ctx->seen++;

    if (ctx->seen != ctx->need) {
        ctx->state = U8_DECODE_CONTINUE;
        return 0;
    }

    uint32_t cp = ctx->codepoint;
    ctx->codepoint = ctx->need = ctx->seen = 0;

    ctx->state = U8_DECODE_OK;
    return cp;
}

uint32_t *u8_decode(const char *src, _Bool replace) {
    if (!src) {
        errno = EINVAL;
        return NULL;
    }

    ptrdiff_t dest_len = strlen(src) + 1;
    uint32_t *dest = calloc(dest_len, sizeof(*dest));
    if (!dest)
        return NULL;

    uint32_t *start = dest;
    u8_decoder_ctx dctx = u8_decode_ctx();
    uint32_t rcp = 0x0;
    while (1) {
        rcp = u8_decode_once(dctx, src);

        if (dctx->state == U8_DECODE_FINISH) {
            *dest = 0x0;
            free(dctx);
            return start;
        } else if (rcp > 0) {
            *dest = rcp;
        } else if (dctx->state == U8_DECODE_ERROR) {
            if (replace) {
                *dest = 0xFFFD;
            } else {
                free(start);
                free(dctx);
                errno = EILSEQ;
                return NULL;
            }
        }

        if (dctx->state != U8_DECODE_CONTINUE && (dest - start) < dest_len)
            dest++;
    }
}

char *u8_encode(const uint32_t *src) {
    if (!src)
        return NULL;

    /* Calculate maximum possible length (each codepoint can be up to 4 bytes)
     */
    size_t src_len = 0;
    while (src[src_len] != 0)
        src_len++;

    size_t dest_max_len = src_len * 4 + 1;
    char *dest = calloc(dest_max_len, sizeof(*dest));
    if (!dest)
        return NULL;

    char *dest_ptr = dest;
    size_t i = 0;

    while (src[i] != 0) {
        uint32_t cp = src[i];

        /* ASCII range: 0x00-0x7F */
        if (cp <= 0x7F) {
            *dest_ptr++ = (char)cp;
        }
        /* 2-byte sequence: 0x80-0x7FF */
        else if (cp <= 0x7FF) {
            *dest_ptr++ = (char)(0xC0 | (cp >> 6));
            *dest_ptr++ = (char)(0x80 | (cp & 0x3F));
        }
        /* 3-byte sequence: 0x800-0xFFFF */
        else if (cp <= 0xFFFF) {
            /* Check for surrogate pairs (invalid in UTF-8) */
            if (cp >= 0xD800 && cp <= 0xDFFF) {
                free(dest);
                errno = EILSEQ;
                return NULL;
            }
            *dest_ptr++ = (char)(0xE0 | (cp >> 12));
            *dest_ptr++ = (char)(0x80 | ((cp >> 6) & 0x3F));
            *dest_ptr++ = (char)(0x80 | (cp & 0x3F));
        }
        /* 4-byte sequence: 0x10000-0x10FFFF */
        else if (cp <= 0x10FFFF) {
            *dest_ptr++ = (char)(0xF0 | (cp >> 18));
            *dest_ptr++ = (char)(0x80 | ((cp >> 12) & 0x3F));
            *dest_ptr++ = (char)(0x80 | ((cp >> 6) & 0x3F));
            *dest_ptr++ = (char)(0x80 | (cp & 0x3F));
        }
        /* Invalid codepoint */
        else {
            free(dest);
            errno = EILSEQ;
            return NULL;
        }

        i++;
    }

    *dest_ptr = '\0';
    return dest;
}
