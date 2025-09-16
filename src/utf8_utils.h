#ifndef UTF8_UTILS_H
#define UTF8_UTILS_H

#include <stddef.h>
#include <stdint.h>

/* Validation functions */
int u8_valid(const char *data, size_t len);
int u8_valid_cstr(const char *str);
size_t u8_len(const char *data, size_t byte_len);
size_t u8_len_cstr(const char *str);

/* Search functions */
const char *u8_chr(const char *str, uint32_t cp);
const char *u8_chr_n(const char *str, size_t len, uint32_t cp);
const char *u8_str(const char *haystack, const char *needle);
const char *u8_str_n(const char *haystack, size_t haystack_len, const char *needle);

/* Case conversion functions */
char *u8_upper(const char *str);
char *u8_lower(const char *str);
char *u8_upper_n(const char *str, size_t len);
char *u8_lower_n(const char *str, size_t len);

/* Character classification */
int u8_ascii(uint32_t codepoint);
int u8_alpha(uint32_t codepoint);
int u8_digit(uint32_t codepoint);
int u8_space(uint32_t codepoint);
int u8_upper_cp(uint32_t codepoint);
int u8_lower_cp(uint32_t codepoint);

/* Codepoint conversion */
uint32_t u8_toupper(uint32_t codepoint);
uint32_t u8_tolower(uint32_t codepoint);

/* Utility functions */
size_t u8_cp_bytelen(uint32_t codepoint);
int u8_cp_to_bytes(uint32_t codepoint, char *buffer, size_t buffer_size);

#endif /* UTF8_UTILS_H */
