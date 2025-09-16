#include "akutf.h"
#include "utf8_constants.h"

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

static u8_decoder_ctx u8_decode_ctx(void) {
    u8_decoder_ctx ctx = calloc(1, sizeof(*ctx));
    if (!ctx)
        return NULL;

    ctx->lower = UTF8_CONT_BYTE_MIN;
    ctx->upper = UTF8_CONT_BYTE_MAX;
    ctx->saveptr = NULL;

    return ctx;
}

static uint32_t u8_decode_once(u8_decoder_ctx ctx, const char *src) {
    if (!ctx->saveptr)
        ctx->saveptr = src;
    uint8_t byte = *ctx->saveptr;
    if (byte != UTF8_NULL_BYTE)
        ctx->saveptr++;

    if (byte == UTF8_NULL_BYTE && ctx->need > 0) {
        ctx->need = 0;
        ctx->state = U8_DECODE_ERROR;
        return 0;
    } else if (byte == UTF8_NULL_BYTE) {
        ctx->state = U8_DECODE_FINISH;
        return 0;
    }

    if (ctx->need == 0) {
        if (byte > UTF8_NULL_BYTE && byte <= UTF8_ASCII_MAX) {
            ctx->state = U8_DECODE_OK;
            return byte;
        } else if (byte >= UTF8_2BYTE_MIN && byte <= UTF8_2BYTE_MAX) {
            ctx->need = 1;
            ctx->codepoint = byte & UTF8_2BYTE_VALUE_MASK;
        } else if (byte >= UTF8_3BYTE_MIN && byte <= UTF8_3BYTE_MAX) {
            if (byte == UTF8_3BYTE_MIN)
                ctx->lower = UTF8_E0_CONT_MIN;
            else if (byte == UTF8_3BYTE_SURROGATE)
                ctx->upper = UTF8_ED_CONT_MAX;

            ctx->need = 2;
            ctx->codepoint = byte & UTF8_3BYTE_VALUE_MASK;
        } else if (byte >= UTF8_4BYTE_MIN && byte <= UTF8_4BYTE_MAX) {
            if (byte == UTF8_4BYTE_MIN)
                ctx->lower = UTF8_F0_CONT_MIN;
            else if (byte == UTF8_4BYTE_MAX)
                ctx->upper = UTF8_F4_CONT_MAX;

            ctx->need = 3;
            ctx->codepoint = byte & UTF8_4BYTE_VALUE_MASK;
        } else {
            ctx->state = U8_DECODE_ERROR;
            return 0;
        }

        ctx->state = U8_DECODE_CONTINUE;
        return 0;
    }

    if (byte < ctx->lower || byte > ctx->upper) {
        ctx->codepoint = ctx->need = ctx->seen = 0;
        ctx->lower = UTF8_CONT_BYTE_MIN;
        ctx->upper = UTF8_CONT_BYTE_MAX;

        ctx->saveptr--;
        ctx->state = U8_DECODE_ERROR;
        return 0;
    }

    ctx->lower = UTF8_CONT_BYTE_MIN;
    ctx->upper = UTF8_CONT_BYTE_MAX;
    ctx->codepoint = (ctx->codepoint << 6) | (byte & UTF8_CONTINUATION_VALUE_MASK);
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

uint32_t *u8dec(const char *src, _Bool replace) {
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
    uint32_t rcp = UTF8_NULL_CODEPOINT;
    while (1) {
        rcp = u8_decode_once(dctx, src);

        if (dctx->state == U8_DECODE_FINISH) {
            *dest = UTF8_NULL_CODEPOINT;
            free(dctx);
            return start;
        } else if (rcp > 0) {
            *dest = rcp;
        } else if (dctx->state == U8_DECODE_ERROR) {
            if (replace) {
                *dest = UTF8_REPLACEMENT_CHARACTER;
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

static int u8_encode_codepoint(uint32_t cp, char *dest) {
    if (cp <= UTF8_CODEPOINT_1BYTE_MAX) {
        dest[0] = (char)cp;
        return 1;
    }
    else if (cp <= UTF8_CODEPOINT_2BYTE_MAX) {
        dest[0] = (char)(UTF8_2BYTE_PATTERN | (cp >> 6));
        dest[1] = (char)(UTF8_CONTINUATION_PATTERN | (cp & UTF8_CONTINUATION_VALUE_MASK));
        return 2;
    }
    else if (cp <= UTF8_CODEPOINT_3BYTE_MAX) {
        if (cp >= UTF8_SURROGATE_MIN && cp <= UTF8_SURROGATE_MAX) {
            errno = EILSEQ;
            return -1;
        }
        dest[0] = (char)(UTF8_3BYTE_PATTERN | (cp >> 12));
        dest[1] = (char)(UTF8_CONTINUATION_PATTERN | ((cp >> 6) & UTF8_CONTINUATION_VALUE_MASK));
        dest[2] = (char)(UTF8_CONTINUATION_PATTERN | (cp & UTF8_CONTINUATION_VALUE_MASK));
        return 3;
    }
    else if (cp <= UTF8_CODEPOINT_4BYTE_MAX) {
        dest[0] = (char)(UTF8_4BYTE_PATTERN | (cp >> 18));
        dest[1] = (char)(UTF8_CONTINUATION_PATTERN | ((cp >> 12) & UTF8_CONTINUATION_VALUE_MASK));
        dest[2] = (char)(UTF8_CONTINUATION_PATTERN | ((cp >> 6) & UTF8_CONTINUATION_VALUE_MASK));
        dest[3] = (char)(UTF8_CONTINUATION_PATTERN | (cp & UTF8_CONTINUATION_VALUE_MASK));
        return 4;
    }
    else {
        errno = EILSEQ;
        return -1;
    }
}

char *u8enc(const uint32_t *src) {
    if (!src) {
        errno = EINVAL;
        return NULL;
    }

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
        int bytes_written = u8_encode_codepoint(cp, dest_ptr);
        
        if (bytes_written < 0) {
            free(dest);
            return NULL;
        }
        
        dest_ptr += bytes_written;
        i++;
    }

    *dest_ptr = '\0';
    return dest;
}
