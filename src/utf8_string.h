#ifndef UTF8_STRING_H
#define UTF8_STRING_H

#include <stddef.h>
#include <stdint.h>

/* UTF-8 string structure with length tracking */
typedef struct utf8_string {
    unsigned char *data;  /* UTF-8 data buffer */
    size_t byte_len;      /* Length in bytes (excluding null terminator) */
    size_t codepoint_len; /* Length in codepoints */
    size_t capacity;      /* Allocated capacity in bytes */
} utf8_string;

/* Creation and destruction */
utf8_string *u8s_new(const char *utf8_data);
utf8_string *u8s_new_with_capacity(size_t capacity);
utf8_string *u8s_copy(const utf8_string *src);
utf8_string *u8s_move(utf8_string *dest, utf8_string *src);
void u8s_free(utf8_string *str);

/* Basic operations - use str->byte_len, str->codepoint_len, str->data directly */
int u8s_empty(const utf8_string *str);

/* Modification */
int u8s_cat(utf8_string *dest, const utf8_string *src);
int u8s_cat_cstr(utf8_string *dest, const char *src);
int u8s_cat_cp(utf8_string *dest, uint32_t codepoint);
utf8_string *u8s_concat(const utf8_string *a, const utf8_string *b);

/* Substring operations */
utf8_string *u8s_substr(const utf8_string *str, size_t start, size_t len);
utf8_string *u8s_substr_bytes(const utf8_string *str, size_t start_byte, size_t len_bytes);

/* Access operations */
uint32_t u8s_at(const utf8_string *str, size_t offset, _Bool is_byte_offset);

/* Insertion operations */
int u8s_insert_cp(utf8_string *str, size_t offset, uint32_t codepoint, _Bool is_byte_offset);
int u8s_insert_str(utf8_string *str, size_t offset, const char *src, _Bool is_byte_offset);
int u8s_insert_u8s(utf8_string *str, size_t offset, const utf8_string *src, _Bool is_byte_offset);

/* Memory management */
int u8s_reserve(utf8_string *str, size_t new_capacity);
int u8s_shrink(utf8_string *str);
void u8s_clear(utf8_string *str);

#endif /* UTF8_STRING_H */
