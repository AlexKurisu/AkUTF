#ifndef UTF8_CONSTANTS_H
#define UTF8_CONSTANTS_H

/* UTF-8 byte sequence boundaries and masks */

/* Null terminator */
#define UTF8_NULL_BYTE             0x00

/* ASCII range (1-byte sequences) */
#define UTF8_ASCII_MAX             0x7F

/* 2-byte sequence boundaries */
#define UTF8_2BYTE_MIN             0xC2
#define UTF8_2BYTE_MAX             0xDF
#define UTF8_2BYTE_MASK            0xE0
#define UTF8_2BYTE_PATTERN         0xC0
#define UTF8_2BYTE_VALUE_MASK      0x1F

/* 3-byte sequence boundaries */
#define UTF8_3BYTE_MIN             0xE0
#define UTF8_3BYTE_MAX             0xEF
#define UTF8_3BYTE_SURROGATE       0xED
#define UTF8_3BYTE_MASK            0xF0
#define UTF8_3BYTE_PATTERN         0xE0
#define UTF8_3BYTE_VALUE_MASK      0x0F

/* 4-byte sequence boundaries */
#define UTF8_4BYTE_MIN             0xF0
#define UTF8_4BYTE_MAX             0xF4
#define UTF8_4BYTE_MASK            0xF8
#define UTF8_4BYTE_PATTERN         0xF0
#define UTF8_4BYTE_VALUE_MASK      0x07

/* Continuation byte */
#define UTF8_CONTINUATION_MASK     0xC0
#define UTF8_CONTINUATION_PATTERN  0x80
#define UTF8_CONTINUATION_VALUE_MASK 0x3F

/* Special continuation byte ranges for validation */
#define UTF8_CONT_BYTE_MIN         0x80
#define UTF8_CONT_BYTE_MAX         0xBF

/* Special validation ranges for overlong sequences */
#define UTF8_E0_CONT_MIN           0xA0  /* For 0xE0, second byte must be >= 0xA0 */
#define UTF8_ED_CONT_MAX           0x9F  /* For 0xED, second byte must be <= 0x9F */
#define UTF8_F0_CONT_MIN           0x90  /* For 0xF0, second byte must be >= 0x90 */
#define UTF8_F4_CONT_MAX           0x8F  /* For 0xF4, second byte must be <= 0x8F */

/* Unicode codepoint boundaries */
#define UTF8_CODEPOINT_1BYTE_MAX   0x7F      /* ASCII */
#define UTF8_CODEPOINT_2BYTE_MAX   0x7FF     /* 2-byte UTF-8 max */
#define UTF8_CODEPOINT_3BYTE_MAX   0xFFFF    /* 3-byte UTF-8 max */
#define UTF8_CODEPOINT_4BYTE_MAX   0x10FFFF  /* 4-byte UTF-8 max (Unicode limit) */

/* Surrogate pair range (invalid in UTF-8) */
#define UTF8_SURROGATE_MIN         0xD800
#define UTF8_SURROGATE_MAX         0xDFFF

/* Unicode character constants */
#define UTF8_NULL_CODEPOINT        0x0
#define UTF8_REPLACEMENT_CHARACTER 0xFFFD

#endif /* UTF8_CONSTANTS_H */
