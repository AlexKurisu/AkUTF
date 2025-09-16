#include "utf8_string.h"
#include "utf8_constants.h"
#include "utf8_decode.h"
#include "akutf.h"
#include "utf8_utils.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#define UTF8_STRING_MIN_CAPACITY 16

/* Internal helper to calculate codepoint count */
static size_t utf8_string_count_codepoints(const unsigned char *data, size_t byte_len) {
    if (!data || byte_len == 0) return 0;
    
    size_t count = 0;
    const unsigned char *ptr = data;
    const unsigned char *end = data + byte_len;
    
    while (ptr < end) {
        int seqlen = u8_seqlen(*ptr);
        if (seqlen <= 0 || ptr + seqlen > end) {
            break; /* Invalid sequence */
        }
        ptr += seqlen;
        count++;
    }
    
    return count;
}

/* Internal helper to ensure capacity */
static int utf8_string_ensure_capacity(utf8_string *str, size_t needed_capacity) {
    if (!str) {
        errno = EINVAL;
        return -1;
    }
    
    if (str->capacity >= needed_capacity) {
        return 0;
    }
    
    size_t new_capacity = str->capacity;
    if (new_capacity == 0) {
        new_capacity = UTF8_STRING_MIN_CAPACITY;
    }
    
    while (new_capacity < needed_capacity) {
        new_capacity *= 2;
    }
    
    char *new_data = realloc(str->data, new_capacity);
    if (!new_data) {
        return -1;
    }
    
    str->data = new_data;
    str->capacity = new_capacity;
    return 0;
}

utf8_string *u8s_new(const char *utf8_data) {
    if (!utf8_data) {
        errno = EINVAL;
        return NULL;
    }
    
    size_t byte_len = strlen(utf8_data);
    utf8_string *str = u8s_new_with_capacity(byte_len + 1);
    if (!str) return NULL;
    
    memcpy(str->data, utf8_data, byte_len);
    str->data[byte_len] = '\0';
    str->byte_len = byte_len;
    str->codepoint_len = utf8_string_count_codepoints(str->data, byte_len);
    
    return str;
}

utf8_string *u8s_new_with_capacity(size_t capacity) {
    utf8_string *str = malloc(sizeof(*str));
    if (!str) {
        errno = ENOMEM;
        return NULL;
    }
    
    if (capacity < UTF8_STRING_MIN_CAPACITY) {
        capacity = UTF8_STRING_MIN_CAPACITY;
    }
    
    str->data = malloc(capacity);
    if (!str->data) {
        free(str);
        errno = ENOMEM;
        return NULL;
    }
    
    str->data[0] = '\0';
    str->byte_len = 0;
    str->codepoint_len = 0;
    str->capacity = capacity;
    
    return str;
}

utf8_string *u8s_copy(const utf8_string *src) {
    if (!src) {
        errno = EINVAL;
        return NULL;
    }
    
    utf8_string *copy = u8s_new_with_capacity(src->capacity);
    if (!copy) return NULL;
    
    memcpy(copy->data, src->data, src->byte_len + 1);
    copy->byte_len = src->byte_len;
    copy->codepoint_len = src->codepoint_len;
    
    return copy;
}

utf8_string *u8s_move(utf8_string *dest, utf8_string *src) {
    if (!dest || !src) {
        errno = EINVAL;
        return NULL;
    }
    
    /* Free destination data if it exists */
    free(dest->data);
    
    /* Move data from src to dest */
    dest->data = src->data;
    dest->byte_len = src->byte_len;
    dest->codepoint_len = src->codepoint_len;
    dest->capacity = src->capacity;
    
    /* Clear src */
    src->data = NULL;
    src->byte_len = 0;
    src->codepoint_len = 0;
    src->capacity = 0;
    
    return dest;
}

void u8s_free(utf8_string *str) {
    if (!str) return;
    
    free(str->data);
    free(str);
}

int u8s_empty(const utf8_string *str) {
    return !str || str->byte_len == 0;
}

int u8s_cat(utf8_string *dest, const utf8_string *src) {
    if (!dest || !src) {
        errno = EINVAL;
        return -1;
    }
    
    return u8s_cat_cstr(dest, (const char *)src->data);
}

int u8s_cat_cstr(utf8_string *dest, const char *src) {
    if (!dest || !src) {
        errno = EINVAL;
        return -1;
    }
    
    size_t src_len = strlen(src);
    if (src_len == 0) return 0;
    
    size_t needed_capacity = dest->byte_len + src_len + 1;
    if (utf8_string_ensure_capacity(dest, needed_capacity) != 0) {
        return -1;
    }
    
    memcpy(dest->data + dest->byte_len, src, src_len);
    dest->byte_len += src_len;
    dest->data[dest->byte_len] = '\0';
    dest->codepoint_len += utf8_string_count_codepoints(src, src_len);
    
    return 0;
}

int u8s_cat_cp(utf8_string *dest, uint32_t codepoint) {
    if (!dest) {
        errno = EINVAL;
        return -1;
    }
    
    /* Encode the codepoint to UTF-8 */
    uint32_t codepoints[2] = {codepoint, 0};
    char *encoded = u8enc(codepoints);
    if (!encoded) return -1;
    
    int result = u8s_cat_cstr(dest, encoded);
    free(encoded);
    return result;
}

utf8_string *u8s_concat(const utf8_string *a, const utf8_string *b) {
    if (!a && !b) {
        errno = EINVAL;
        return NULL;
    }
    if (!a) return u8s_copy(b);
    if (!b) return u8s_copy(a);
    
    utf8_string *result = u8s_new_with_capacity(a->byte_len + b->byte_len + 1);
    if (!result) return NULL;
    
    if (u8s_cat(result, a) != 0 || u8s_cat(result, b) != 0) {
        u8s_free(result);
        return NULL;
    }
    
    return result;
}

utf8_string *u8s_substr(const utf8_string *str, size_t start, size_t len) {
    if (!str || start >= str->codepoint_len) {
        errno = EINVAL;
        return NULL;
    }
    
    if (len == 0) {
        return u8s_new("");
    }
    
    /* Convert to codepoints, extract substring, convert back */
    uint32_t *codepoints = u8dec(str->data, 0);
    if (!codepoints) return NULL;
    
    /* Adjust length to not exceed bounds */
    if (start + len > str->codepoint_len) {
        len = str->codepoint_len - start;
    }
    
    /* Create subset array */
    uint32_t *subset = malloc((len + 1) * sizeof(*subset));
    if (!subset) {
        free(codepoints);
        errno = ENOMEM;
        return NULL;
    }
    
    memcpy(subset, codepoints + start, len * sizeof(*subset));
    subset[len] = 0;
    
    /* Encode back to UTF-8 */
    char *encoded = u8enc(subset);
    free(codepoints);
    free(subset);
    
    if (!encoded) return NULL;
    
    utf8_string *result = u8s_new(encoded);
    free(encoded);
    return result;
}

utf8_string *u8s_substr_bytes(const utf8_string *str, size_t start_byte, size_t len_bytes) {
    if (!str || start_byte >= str->byte_len) {
        errno = EINVAL;
        return NULL;
    }
    
    if (len_bytes == 0) {
        return u8s_new("");
    }
    
    /* Adjust length to not exceed bounds */
    if (start_byte + len_bytes > str->byte_len) {
        len_bytes = str->byte_len - start_byte;
    }
    
    /* Create temporary null-terminated string */
    char *temp = malloc(len_bytes + 1);
    if (!temp) {
        errno = ENOMEM;
        return NULL;
    }
    
    memcpy(temp, str->data + start_byte, len_bytes);
    temp[len_bytes] = '\0';
    
    utf8_string *result = u8s_new(temp);
    free(temp);
    return result;
}

int u8s_reserve(utf8_string *str, size_t new_capacity) {
    if (!str) {
        errno = EINVAL;
        return -1;
    }
    
    if (new_capacity <= str->capacity) {
        return 0;
    }
    
    return utf8_string_ensure_capacity(str, new_capacity);
}

int u8s_shrink(utf8_string *str) {
    if (!str) {
        errno = EINVAL;
        return -1;
    }
    
    size_t needed = str->byte_len + 1;
    if (needed >= str->capacity) {
        return 0;
    }
    
    char *new_data = realloc(str->data, needed);
    if (!new_data) return -1;
    
    str->data = new_data;
    str->capacity = needed;
    return 0;
}

void u8s_clear(utf8_string *str) {
    if (!str) return;
    
    str->data[0] = '\0';
    str->byte_len = 0;
    str->codepoint_len = 0;
}

/* Access operations */
uint32_t u8s_at(const utf8_string *str, size_t offset, _Bool is_byte_offset) {
    if (!str || !str->data) {
        errno = EINVAL;
        return 0;
    }
    
    if (is_byte_offset) {
        /* Byte offset */
        if (offset >= str->byte_len) {
            errno = EINVAL;
            return 0;
        }
        
        /* Decode UTF-8 sequence at byte offset */
        const char *ptr = str->data + offset;
        const char *end = str->data + str->byte_len;
        
        if (ptr >= end) {
            errno = EINVAL;
            return 0;
        }
        
        /* Manual UTF-8 decoding for single codepoint */
        unsigned char first = (unsigned char)*ptr;
        uint32_t codepoint = 0;
        int seq_len = 0;
        
        if ((first & 0x80) == 0) {
            /* ASCII: 0xxxxxxx */
            codepoint = first;
            seq_len = 1;
        } else if ((first & UTF8_2BYTE_MASK) == UTF8_2BYTE_PATTERN) {
            /* 2-byte: 110xxxxx 10xxxxxx */
            if (ptr + 1 >= end) { errno = EILSEQ; return 0; }
            codepoint = ((first & UTF8_2BYTE_VALUE_MASK) << 6) | (ptr[1] & UTF8_CONTINUATION_VALUE_MASK);
            seq_len = 2;
        } else if ((first & UTF8_3BYTE_MASK) == UTF8_3BYTE_PATTERN) {
            /* 3-byte: 1110xxxx 10xxxxxx 10xxxxxx */
            if (ptr + 2 >= end) { errno = EILSEQ; return 0; }
            codepoint = ((first & UTF8_3BYTE_VALUE_MASK) << 12) | ((ptr[1] & UTF8_CONTINUATION_VALUE_MASK) << 6) | (ptr[2] & UTF8_CONTINUATION_VALUE_MASK);
            seq_len = 3;
        } else if ((first & UTF8_4BYTE_MASK) == UTF8_4BYTE_PATTERN) {
            /* 4-byte: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx */
            if (ptr + 3 >= end) { errno = EILSEQ; return 0; }
            codepoint = ((first & UTF8_4BYTE_VALUE_MASK) << 18) | ((ptr[1] & UTF8_CONTINUATION_VALUE_MASK) << 12) | ((ptr[2] & UTF8_CONTINUATION_VALUE_MASK) << 6) | (ptr[3] & UTF8_CONTINUATION_VALUE_MASK);
            seq_len = 4;
        } else {
            errno = EILSEQ;
            return 0;
        }
        
        /* Validate continuation bytes */
        for (int i = 1; i < seq_len; i++) {
            if ((ptr[i] & UTF8_CONTINUATION_MASK) != UTF8_CONTINUATION_PATTERN) {
                errno = EILSEQ;
                return 0;
            }
        }
        
        return codepoint;
        
    } else {
        /* Codepoint offset */
        if (offset >= str->codepoint_len) {
            errno = EINVAL;
            return 0;
        }
        
        /* Iterate through codepoints to find the offset */
        const char *ptr = str->data;
        const char *end = str->data + str->byte_len;
        size_t current_cp = 0;
        
        while (ptr < end && current_cp < offset) {
            /* Skip one UTF-8 character */
            if ((*ptr & 0x80) == 0) {
                /* ASCII character (0xxxxxxx) */
                ptr += 1;
            } else if ((*ptr & UTF8_2BYTE_MASK) == UTF8_2BYTE_PATTERN) {
                ptr += 2;
            } else if ((*ptr & UTF8_3BYTE_MASK) == UTF8_3BYTE_PATTERN) {
                ptr += 3;
            } else if ((*ptr & UTF8_4BYTE_MASK) == UTF8_4BYTE_PATTERN) {
                ptr += 4;
            } else {
                errno = EILSEQ;
                return 0;
            }
            current_cp++;
        }
        
        if (ptr >= end) {
            errno = EINVAL;
            return 0;
        }
        
        /* Decode UTF-8 sequence at this position */
        unsigned char first = (unsigned char)*ptr;
        uint32_t codepoint = 0;
        int seq_len = 0;
        
        if ((first & 0x80) == 0) {
            /* ASCII: 0xxxxxxx */
            codepoint = first;
            seq_len = 1;
        } else if ((first & UTF8_2BYTE_MASK) == UTF8_2BYTE_PATTERN) {
            /* 2-byte: 110xxxxx 10xxxxxx */
            if (ptr + 1 >= end) { errno = EILSEQ; return 0; }
            codepoint = ((first & UTF8_2BYTE_VALUE_MASK) << 6) | (ptr[1] & UTF8_CONTINUATION_VALUE_MASK);
            seq_len = 2;
        } else if ((first & UTF8_3BYTE_MASK) == UTF8_3BYTE_PATTERN) {
            /* 3-byte: 1110xxxx 10xxxxxx 10xxxxxx */
            if (ptr + 2 >= end) { errno = EILSEQ; return 0; }
            codepoint = ((first & UTF8_3BYTE_VALUE_MASK) << 12) | ((ptr[1] & UTF8_CONTINUATION_VALUE_MASK) << 6) | (ptr[2] & UTF8_CONTINUATION_VALUE_MASK);
            seq_len = 3;
        } else if ((first & UTF8_4BYTE_MASK) == UTF8_4BYTE_PATTERN) {
            /* 4-byte: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx */
            if (ptr + 3 >= end) { errno = EILSEQ; return 0; }
            codepoint = ((first & UTF8_4BYTE_VALUE_MASK) << 18) | ((ptr[1] & UTF8_CONTINUATION_VALUE_MASK) << 12) | ((ptr[2] & UTF8_CONTINUATION_VALUE_MASK) << 6) | (ptr[3] & UTF8_CONTINUATION_VALUE_MASK);
            seq_len = 4;
        } else {
            errno = EILSEQ;
            return 0;
        }
        
        /* Validate continuation bytes */
        for (int i = 1; i < seq_len; i++) {
            if ((ptr[i] & UTF8_CONTINUATION_MASK) != UTF8_CONTINUATION_PATTERN) {
                errno = EILSEQ;
                return 0;
            }
        }
        
        return codepoint;
    }
}

/* Helper function to convert codepoint offset to byte offset */
static size_t codepoint_to_byte_offset(const utf8_string *str, size_t cp_offset) {
    if (!str || !str->data || cp_offset >= str->codepoint_len) {
        return SIZE_MAX; /* Invalid */
    }
    
    const unsigned char *ptr = str->data;
    const unsigned char *end = str->data + str->byte_len;
    size_t current_cp = 0;
    
    while (ptr < end && current_cp < cp_offset) {
        if ((*ptr & 0x80) == 0) {
            ptr += 1;
        } else if ((*ptr & UTF8_2BYTE_MASK) == UTF8_2BYTE_PATTERN) {
            ptr += 2;
        } else if ((*ptr & UTF8_3BYTE_MASK) == UTF8_3BYTE_PATTERN) {
            ptr += 3;
        } else if ((*ptr & UTF8_4BYTE_MASK) == UTF8_4BYTE_PATTERN) {
            ptr += 4;
        } else {
            return SIZE_MAX; /* Invalid UTF-8 */
        }
        current_cp++;
    }
    
    return ptr - str->data;
}

/* Helper function to encode a codepoint to UTF-8 bytes */
static int encode_codepoint(uint32_t codepoint, unsigned char *buffer, size_t *bytes_written) {
    if (!buffer || !bytes_written) {
        errno = EINVAL;
        return -1;
    }
    
    if (codepoint <= UTF8_CODEPOINT_1BYTE_MAX) {
        /* 1-byte sequence */
        buffer[0] = (unsigned char)codepoint;
        *bytes_written = 1;
    } else if (codepoint <= UTF8_CODEPOINT_2BYTE_MAX) {
        /* 2-byte sequence */
        buffer[0] = (unsigned char)(UTF8_2BYTE_PATTERN | (codepoint >> 6));
        buffer[1] = (unsigned char)(UTF8_CONTINUATION_PATTERN | (codepoint & UTF8_CONTINUATION_VALUE_MASK));
        *bytes_written = 2;
    } else if (codepoint <= UTF8_CODEPOINT_3BYTE_MAX) {
        /* 3-byte sequence */
        buffer[0] = (unsigned char)(UTF8_3BYTE_PATTERN | (codepoint >> 12));
        buffer[1] = (unsigned char)(UTF8_CONTINUATION_PATTERN | ((codepoint >> 6) & UTF8_CONTINUATION_VALUE_MASK));
        buffer[2] = (unsigned char)(UTF8_CONTINUATION_PATTERN | (codepoint & UTF8_CONTINUATION_VALUE_MASK));
        *bytes_written = 3;
    } else if (codepoint <= UTF8_CODEPOINT_4BYTE_MAX) {
        /* 4-byte sequence */
        buffer[0] = (unsigned char)(UTF8_4BYTE_PATTERN | (codepoint >> 18));
        buffer[1] = (unsigned char)(UTF8_CONTINUATION_PATTERN | ((codepoint >> 12) & UTF8_CONTINUATION_VALUE_MASK));
        buffer[2] = (unsigned char)(UTF8_CONTINUATION_PATTERN | ((codepoint >> 6) & UTF8_CONTINUATION_VALUE_MASK));
        buffer[3] = (unsigned char)(UTF8_CONTINUATION_PATTERN | (codepoint & UTF8_CONTINUATION_VALUE_MASK));
        *bytes_written = 4;
    } else {
        errno = EINVAL;
        return -1;
    }
    
    return 0;
}

int u8s_insert_cp(utf8_string *str, size_t offset, uint32_t codepoint, _Bool is_byte_offset) {
    if (!str || !str->data) {
        errno = EINVAL;
        return -1;
    }
    
    size_t byte_offset;
    if (is_byte_offset) {
        if (offset > str->byte_len) {
            errno = EINVAL;
            return -1;
        }
        byte_offset = offset;
    } else {
        if (offset > str->codepoint_len) {
            errno = EINVAL;
            return -1;
        }
        if (offset == str->codepoint_len) {
            /* Append at end */
            byte_offset = str->byte_len;
        } else {
            byte_offset = codepoint_to_byte_offset(str, offset);
            if (byte_offset == SIZE_MAX) {
                errno = EINVAL;
                return -1;
            }
        }
    }
    
    /* Encode the codepoint */
    unsigned char encoded[4];
    size_t encoded_len;
    if (encode_codepoint(codepoint, encoded, &encoded_len) < 0) {
        return -1;
    }
    
    /* Ensure capacity */
    if (str->byte_len + encoded_len >= str->capacity) {
        size_t new_capacity = (str->capacity == 0) ? 16 : str->capacity * 2;
        while (new_capacity <= str->byte_len + encoded_len) {
            new_capacity *= 2;
        }
        if (u8s_reserve(str, new_capacity) < 0) {
            return -1;
        }
    }
    
    /* Move existing data to make room */
    if (byte_offset < str->byte_len) {
        memmove(str->data + byte_offset + encoded_len,
                str->data + byte_offset,
                str->byte_len - byte_offset);
    }
    
    /* Insert the encoded codepoint */
    memcpy(str->data + byte_offset, encoded, encoded_len);
    str->byte_len += encoded_len;
    str->codepoint_len++;
    str->data[str->byte_len] = '\0';
    
    return 0;
}

int u8s_insert_str(utf8_string *str, size_t offset, const char *src, _Bool is_byte_offset) {
    if (!str || !str->data || !src) {
        errno = EINVAL;
        return -1;
    }
    
    size_t src_byte_len = strlen(src);
    if (src_byte_len == 0) {
        return 0; /* Nothing to insert */
    }
    
    /* Count codepoints in src */
    size_t src_cp_len = u8_len(src, src_byte_len);
    if (src_cp_len == 0) {
        errno = EILSEQ; /* Invalid UTF-8 */
        return -1;
    }
    
    size_t byte_offset;
    if (is_byte_offset) {
        if (offset > str->byte_len) {
            errno = EINVAL;
            return -1;
        }
        byte_offset = offset;
    } else {
        if (offset > str->codepoint_len) {
            errno = EINVAL;
            return -1;
        }
        if (offset == str->codepoint_len) {
            byte_offset = str->byte_len;
        } else {
            byte_offset = codepoint_to_byte_offset(str, offset);
            if (byte_offset == SIZE_MAX) {
                errno = EINVAL;
                return -1;
            }
        }
    }
    
    /* Ensure capacity */
    if (str->byte_len + src_byte_len >= str->capacity) {
        size_t new_capacity = (str->capacity == 0) ? 16 : str->capacity * 2;
        while (new_capacity <= str->byte_len + src_byte_len) {
            new_capacity *= 2;
        }
        if (u8s_reserve(str, new_capacity) < 0) {
            return -1;
        }
    }
    
    /* Move existing data to make room */
    if (byte_offset < str->byte_len) {
        memmove(str->data + byte_offset + src_byte_len,
                str->data + byte_offset,
                str->byte_len - byte_offset);
    }
    
    /* Insert the source string */
    memcpy(str->data + byte_offset, src, src_byte_len);
    str->byte_len += src_byte_len;
    str->codepoint_len += src_cp_len;
    str->data[str->byte_len] = '\0';
    
    return 0;
}

int u8s_insert_u8s(utf8_string *str, size_t offset, const utf8_string *src, _Bool is_byte_offset) {
    if (!str || !str->data || !src || !src->data) {
        errno = EINVAL;
        return -1;
    }
    
    if (src->byte_len == 0) {
        return 0; /* Nothing to insert */
    }
    
    size_t byte_offset;
    if (is_byte_offset) {
        if (offset > str->byte_len) {
            errno = EINVAL;
            return -1;
        }
        byte_offset = offset;
    } else {
        if (offset > str->codepoint_len) {
            errno = EINVAL;
            return -1;
        }
        if (offset == str->codepoint_len) {
            byte_offset = str->byte_len;
        } else {
            byte_offset = codepoint_to_byte_offset(str, offset);
            if (byte_offset == SIZE_MAX) {
                errno = EINVAL;
                return -1;
            }
        }
    }
    
    /* Ensure capacity */
    if (str->byte_len + src->byte_len >= str->capacity) {
        size_t new_capacity = (str->capacity == 0) ? 16 : str->capacity * 2;
        while (new_capacity <= str->byte_len + src->byte_len) {
            new_capacity *= 2;
        }
        if (u8s_reserve(str, new_capacity) < 0) {
            return -1;
        }
    }
    
    /* Move existing data to make room */
    if (byte_offset < str->byte_len) {
        memmove(str->data + byte_offset + src->byte_len,
                str->data + byte_offset,
                str->byte_len - byte_offset);
    }
    
    /* Insert the source string */
    memcpy(str->data + byte_offset, src->data, src->byte_len);
    str->byte_len += src->byte_len;
    str->codepoint_len += src->codepoint_len;
    str->data[str->byte_len] = '\0';
    
    return 0;
}
