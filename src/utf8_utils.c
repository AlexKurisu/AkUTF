#include "utf8_utils.h"
#include "utf8_constants.h"
#include "akutf.h"
#include "utf8_iterator.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

/* Simple uppercase/lowercase mappings for basic Latin */
static uint32_t utf8_simple_to_upper(uint32_t cp) {
    if (cp >= 'a' && cp <= 'z') {
        return cp - 'a' + 'A';
    }
    return cp;
}

static uint32_t utf8_simple_to_lower(uint32_t cp) {
    if (cp >= 'A' && cp <= 'Z') {
        return cp - 'A' + 'a';
    }
    return cp;
}

int u8_valid(const char *data, size_t len) {
    if (!data) {
        errno = EINVAL;
        return 0;
    }
    
    if (len == 0) return 1;
    
    utf8_iterator iter = u8i_new_with_length(data, len);
    if (u8i_error(&iter)) {
        return 0;
    }
    
    /* Iterate through all codepoints to validate */
    while (u8i_has_next(&iter)) {
        if (u8i_next(&iter) < 0) {
            return 0;
        }
    }
    
    return !u8i_error(&iter);
}

int u8_valid_cstr(const char *str) {
    if (!str) {
        errno = EINVAL;
        return 0;
    }
    
    return u8_valid(str, strlen(str));
}

size_t u8_len(const char *data, size_t byte_len) {
    if (!data || byte_len == 0) return 0;
    
    utf8_iterator iter = u8i_new_with_length(data, byte_len);
    if (u8i_error(&iter)) {
        return 0;
    }
    
    size_t count = 0;
    while (u8i_has_next(&iter)) {
        if (u8i_next(&iter) < 0) {
            return 0;
        }
        count++;
    }
    
    return count;
}

size_t u8_len_cstr(const char *str) {
    if (!str) return 0;
    return u8_len(str, strlen(str));
}

const char *u8_chr(const char *str, uint32_t cp) {
    if (!str) {
        errno = EINVAL;
        return NULL;
    }
    
    return u8_chr_n(str, strlen(str), cp);
}

const char *u8_chr_n(const char *str, size_t len, uint32_t cp) {
    if (!str) {
        errno = EINVAL;
        return NULL;
    }
    
    utf8_iterator iter = u8i_new_with_length(str, len);
    if (u8i_error(&iter)) {
        return NULL;
    }
    
    do {
        if (u8i_codepoint(&iter) == cp) {
            return u8i_current_ptr(&iter);
        }
    } while (u8i_has_next(&iter) && u8i_next(&iter) >= 0);
    
    return NULL;
}

const char *u8_str(const char *haystack, const char *needle) {
    if (!haystack || !needle) {
        errno = EINVAL;
        return NULL;
    }
    
    return u8_str_n(haystack, strlen(haystack), needle);
}

const char *u8_str_n(const char *haystack, size_t haystack_len, const char *needle) {
    if (!haystack || !needle) {
        errno = EINVAL;
        return NULL;
    }
    
    size_t needle_len = strlen(needle);
    if (needle_len == 0) {
        return haystack;
    }
    
    if (needle_len > haystack_len) {
        return NULL;
    }
    
    /* Simple byte-based search for UTF-8 strings */
    for (size_t i = 0; i <= haystack_len - needle_len; i++) {
        if (memcmp(haystack + i, needle, needle_len) == 0) {
            return haystack + i;
        }
    }
    
    return NULL;
}

static char *utf8_transform_case(const char *str, size_t len, uint32_t (*transform_func)(uint32_t)) {
    if (!str || !transform_func) {
        errno = EINVAL;
        return NULL;
    }
    
    if (len == 0) {
        char *result = malloc(1);
        if (!result) {
            errno = ENOMEM;
            return NULL;
        }
        result[0] = '\0';
        return result;
    }
    
    /* Decode to codepoints */
    uint32_t *codepoints = u8dec(str, 0);
    if (!codepoints) return NULL;
    
    /* Count codepoints */
    size_t cp_count = 0;
    while (codepoints[cp_count] != 0) {
        cp_count++;
    }
    
    /* Transform codepoints */
    for (size_t i = 0; i < cp_count; i++) {
        codepoints[i] = transform_func(codepoints[i]);
    }
    
    /* Encode back to UTF-8 */
    char *result = u8enc(codepoints);
    free(codepoints);
    
    return result;
}

char *u8_upper(const char *str) {
    if (!str) {
        errno = EINVAL;
        return NULL;
    }
    
    return utf8_transform_case(str, strlen(str), utf8_simple_to_upper);
}

char *u8_lower(const char *str) {
    if (!str) {
        errno = EINVAL;
        return NULL;
    }
    
    return utf8_transform_case(str, strlen(str), utf8_simple_to_lower);
}

char *u8_upper_n(const char *str, size_t len) {
    if (!str) {
        errno = EINVAL;
        return NULL;
    }
    
    return utf8_transform_case(str, len, utf8_simple_to_upper);
}

char *u8_lower_n(const char *str, size_t len) {
    if (!str) {
        errno = EINVAL;
        return NULL;
    }
    
    return utf8_transform_case(str, len, utf8_simple_to_lower);
}

int u8_ascii(uint32_t codepoint) {
    return codepoint <= UTF8_ASCII_MAX;
}

int u8_alpha(uint32_t codepoint) {
    /* Basic Latin alphabet only */
    return (codepoint >= 'A' && codepoint <= 'Z') ||
           (codepoint >= 'a' && codepoint <= 'z');
}

int u8_digit(uint32_t codepoint) {
    return codepoint >= '0' && codepoint <= '9';
}

int u8_space(uint32_t codepoint) {
    /* Basic whitespace characters */
    return codepoint == ' ' || codepoint == '\t' || 
           codepoint == '\n' || codepoint == '\r' ||
           codepoint == '\f' || codepoint == '\v';
}

int u8_upper_cp(uint32_t codepoint) {
    return codepoint >= 'A' && codepoint <= 'Z';
}

int u8_lower_cp(uint32_t codepoint) {
    return codepoint >= 'a' && codepoint <= 'z';
}

uint32_t u8_toupper(uint32_t codepoint) {
    return utf8_simple_to_upper(codepoint);
}

uint32_t u8_tolower(uint32_t codepoint) {
    return utf8_simple_to_lower(codepoint);
}

size_t u8_cp_bytelen(uint32_t codepoint) {
    if (codepoint <= UTF8_CODEPOINT_1BYTE_MAX) return 1;
    if (codepoint <= UTF8_CODEPOINT_2BYTE_MAX) return 2;
    if (codepoint <= UTF8_CODEPOINT_3BYTE_MAX) return 3;
    if (codepoint <= UTF8_CODEPOINT_4BYTE_MAX) return 4;
    return 0; /* Invalid codepoint */
}

int u8_cp_to_bytes(uint32_t codepoint, char *buffer, size_t buffer_size) {
    if (!buffer) {
        errno = EINVAL;
        return -1;
    }
    
    size_t needed = u8_cp_bytelen(codepoint);
    if (needed == 0 || buffer_size < needed) {
        errno = ENOBUFS;
        return -1;
    }
    
    /* Use the existing encoder */
    uint32_t codepoints[2] = {codepoint, 0};
    char *encoded = u8enc(codepoints);
    if (!encoded) return -1;
    
    size_t encoded_len = strlen(encoded);
    if (encoded_len != needed || buffer_size < encoded_len) {
        free(encoded);
        errno = ENOBUFS;
        return -1;
    }
    
    memcpy(buffer, encoded, encoded_len);
    free(encoded);
    
    return (int)encoded_len;
}
