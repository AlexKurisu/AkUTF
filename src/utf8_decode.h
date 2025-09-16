#ifndef UTF8_DECODE_H
#define UTF8_DECODE_H

#include <stdint.h>
#include <errno.h>
#include "utf8_constants.h"

/* Optimized UTF-8 decoder based on byte lookup table */
static inline int32_t u8_decode_one(const unsigned char *in) {
    if (!in) {
        errno = EINVAL;
        return -1;
    }
    
    const unsigned char *s = in;
    if (*s <= UTF8_ASCII_MAX) {
        return *s;
    }

    /* UTF-8 sequence length lookup table */
    static const int lengths[] = {
        /* 0xxxx */
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        /* 10xxx */
        0, 0, 0, 0, 0, 0, 0, 0,
        /* 110xx */
        2, 2, 2, 2,
        /* 1110x */
        3, 3,
        /* 11110 */
        4,
        /* 11111 */
        0,
    };

    int len = lengths[*s >> 3];
    if (len == 0) {
        errno = EILSEQ;
        return -1;
    }

    static const unsigned char masks[] = {
        0x0, UTF8_ASCII_MAX, UTF8_2BYTE_VALUE_MASK, UTF8_3BYTE_VALUE_MASK, UTF8_4BYTE_VALUE_MASK,
    };
    
    uint32_t code = (*s & masks[len]) << ((len - 1) * 6);
    unsigned char b1 = *s;
    s++;
    
    for (int i = len - 2; i >= 0; s++, i--) {
        if (*s < UTF8_CONT_BYTE_MIN || *s > UTF8_CONT_BYTE_MAX) {
            errno = EILSEQ;
            return -1;
        }

        switch (len) {
        case 2:
            code |= (*s & UTF8_CONTINUATION_VALUE_MASK);
            return (int32_t)code;
        case 3:
            if (i == len - 2) {
                if (b1 == UTF8_3BYTE_MIN && *s < UTF8_E0_CONT_MIN) {
                    errno = EILSEQ;
                    return -1;
                } else if (b1 == 0xED && *s > UTF8_ED_CONT_MAX) {
                    errno = EILSEQ;
                    return -1;
                }
            }
            code |= (*s & UTF8_CONTINUATION_VALUE_MASK) << (6 * i);
            break;
        case 4:
            if (i == len - 2) {
                if (b1 == UTF8_4BYTE_MIN && *s < UTF8_F0_CONT_MIN) {
                    errno = EILSEQ;
                    return -1;
                } else if (b1 == UTF8_4BYTE_MAX && *s > UTF8_F4_CONT_MAX) {
                    errno = EILSEQ;
                    return -1;
                }
            }
            code |= (*s & UTF8_CONTINUATION_VALUE_MASK) << (6 * i);
            break;
        }
    }

    return (int32_t)code;
}

/* Get UTF-8 sequence length from first byte */
static inline int u8_seqlen(unsigned char first_byte) {
    if (first_byte <= UTF8_ASCII_MAX) return 1;
    
    static const int lengths[] = {
        /* 0xxxx */
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        /* 10xxx */
        0, 0, 0, 0, 0, 0, 0, 0,
        /* 110xx */
        2, 2, 2, 2,
        /* 1110x */
        3, 3,
        /* 11110 */
        4,
        /* 11111 */
        0,
    };
    
    return lengths[first_byte >> 3];
}

#endif /* UTF8_DECODE_H */
