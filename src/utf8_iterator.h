#ifndef UTF8_ITERATOR_H
#define UTF8_ITERATOR_H

#include <stddef.h>
#include <stdint.h>

/* UTF-8 iterator for efficient codepoint iteration */
typedef struct utf8_iterator {
    const char *data;     /* Start of UTF-8 data */
    const char *current;  /* Current position */
    const char *end;      /* End of data */
    uint32_t codepoint;   /* Current codepoint */
    size_t position;      /* Current codepoint index */
    int error;            /* Error flag */
} utf8_iterator;

/* Iterator creation */
utf8_iterator u8i_new(const char *utf8_data);
utf8_iterator u8i_new_with_length(const char *utf8_data, size_t byte_len);

/* Forward iteration */
int u8i_next(utf8_iterator *iter);
int u8i_has_next(const utf8_iterator *iter);

/* Backward iteration */
int u8i_prev(utf8_iterator *iter);
int u8i_has_prev(const utf8_iterator *iter);

/* Current state */
uint32_t u8i_codepoint(const utf8_iterator *iter);
size_t u8i_position(const utf8_iterator *iter);
const char *u8i_current_ptr(const utf8_iterator *iter);
int u8i_error(const utf8_iterator *iter);

/* Positioning */
int u8i_reset(utf8_iterator *iter);
int u8i_seek(utf8_iterator *iter, size_t position);

/* Utility */
size_t u8i_remaining(const utf8_iterator *iter);

/* Relative access */
uint32_t u8i_at(const utf8_iterator *iter, int offset, _Bool is_byte_offset);

#endif /* UTF8_ITERATOR_H */
