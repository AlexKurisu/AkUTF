#include "utf8_iterator.h"
#include "utf8_constants.h"
#include "akutf.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

/* Internal helper to decode one UTF-8 sequence */
static int utf8_decode_sequence(const char *data, const char *end, uint32_t *codepoint, int *bytes_consumed) {
    if (!data || !end || !codepoint || !bytes_consumed || data >= end) {
        errno = EINVAL;
        return -1;
    }
    
    *bytes_consumed = 0;
    *codepoint = 0;
    
    unsigned char first = (unsigned char)*data;
    
    
    if (first <= UTF8_ASCII_MAX) {
        *codepoint = first;
        *bytes_consumed = 1;
        return 0;
    }
    
    
    if ((first & UTF8_2BYTE_MASK) == UTF8_2BYTE_PATTERN) {
        if (data + 2 > end) {
            errno = EILSEQ;
            return -1;
        }
        unsigned char second = (unsigned char)data[1];
        if ((second & UTF8_CONTINUATION_MASK) != UTF8_CONTINUATION_PATTERN) {
            errno = EILSEQ;
            return -1;
        }
        *codepoint = ((first & UTF8_2BYTE_VALUE_MASK) << 6) | (second & UTF8_CONTINUATION_VALUE_MASK);
        *bytes_consumed = 2;
        return 0;
    }
    
    
    if ((first & UTF8_3BYTE_MASK) == UTF8_3BYTE_PATTERN) {
        if (data + 3 > end) {
            errno = EILSEQ;
            return -1;
        }
        unsigned char second = (unsigned char)data[1];
        unsigned char third = (unsigned char)data[2];
        if ((second & UTF8_CONTINUATION_MASK) != UTF8_CONTINUATION_PATTERN || 
            (third & UTF8_CONTINUATION_MASK) != UTF8_CONTINUATION_PATTERN) {
            errno = EILSEQ;
            return -1;
        }
        *codepoint = ((first & UTF8_3BYTE_VALUE_MASK) << 12) | 
                     ((second & UTF8_CONTINUATION_VALUE_MASK) << 6) | 
                     (third & UTF8_CONTINUATION_VALUE_MASK);
        *bytes_consumed = 3;
        return 0;
    }
    
    
    if ((first & UTF8_4BYTE_MASK) == UTF8_4BYTE_PATTERN) {
        if (data + 4 > end) {
            errno = EILSEQ;
            return -1;
        }
        unsigned char second = (unsigned char)data[1];
        unsigned char third = (unsigned char)data[2];
        unsigned char fourth = (unsigned char)data[3];
        if ((second & UTF8_CONTINUATION_MASK) != UTF8_CONTINUATION_PATTERN || 
            (third & UTF8_CONTINUATION_MASK) != UTF8_CONTINUATION_PATTERN || 
            (fourth & UTF8_CONTINUATION_MASK) != UTF8_CONTINUATION_PATTERN) {
            errno = EILSEQ;
            return -1;
        }
        *codepoint = ((first & UTF8_4BYTE_VALUE_MASK) << 18) | 
                     ((second & UTF8_CONTINUATION_VALUE_MASK) << 12) | 
                     ((third & UTF8_CONTINUATION_VALUE_MASK) << 6) | 
                     (fourth & UTF8_CONTINUATION_VALUE_MASK);
        *bytes_consumed = 4;
        return 0;
    }
    
    errno = EILSEQ;
    return -1;
}

/* Internal helper to find start of previous UTF-8 sequence */
static const char *utf8_find_prev_start(const char *current, const char *start) {
    if (!current || !start || current <= start) {
        return NULL;
    }
    
    const char *ptr = current - 1;
    
    /* Look backwards for a valid UTF-8 start byte */
    while (ptr >= start) {
        unsigned char byte = (unsigned char)*ptr;

        /* Any non-continuation byte marks the start of a UTF-8 sequence */
        if ((byte & UTF8_CONTINUATION_MASK) != UTF8_CONTINUATION_PATTERN) {
            return ptr;
        }

        ptr--;

        /* Don't look back more than 4 bytes */
        if (current - ptr > 4) {
            break;
        }
    }
    
    return NULL;
}

utf8_iterator u8i_new(const char *utf8_data) {
    utf8_iterator iter = {0};
    
    if (!utf8_data) {
        iter.error = 1;
        errno = EINVAL;
        return iter;
    }
    
    iter.data = utf8_data;
    iter.current = utf8_data;
    iter.end = utf8_data + strlen(utf8_data);
    iter.position = 0;
    iter.error = 0;
    iter.codepoint = 0; /* No codepoint initially */
    
    return iter;
}

utf8_iterator u8i_new_with_length(const char *utf8_data, size_t byte_len) {
    utf8_iterator iter = {0};
    
    if (!utf8_data) {
        iter.error = 1;
        errno = EINVAL;
        return iter;
    }
    
    iter.data = utf8_data;
    iter.current = utf8_data;
    iter.end = utf8_data + byte_len;
    iter.position = 0;
    iter.error = 0;
    iter.codepoint = 0; /* No codepoint initially */
    
    return iter;
}

int u8i_next(utf8_iterator *iter) {
    if (!iter || iter->error) {
        errno = EINVAL;
        return -1;
    }
    
    if (iter->current >= iter->end) {
        return 0; /* End of data */
    }
    
    /* Decode current sequence */
    int bytes_consumed;
    if (utf8_decode_sequence(iter->current, iter->end, &iter->codepoint, &bytes_consumed) != 0) {
        iter->error = 1;
        return -1;
    }
    
    /* Move to next sequence */
    iter->current += bytes_consumed;
    iter->position++;
    
    return 1;
}

int u8i_has_next(const utf8_iterator *iter) {
    if (!iter || iter->error) {
        return 0;
    }
    
    return iter->current < iter->end;
}

int u8i_prev(utf8_iterator *iter) {
    if (!iter || iter->error) {
        errno = EINVAL;
        return -1;
    }
    
    if (iter->current <= iter->data) {
        return 0; /* At start of data */
    }
    
    /* Find start of previous UTF-8 sequence */
    const char *prev_start = utf8_find_prev_start(iter->current, iter->data);
    if (!prev_start) {
        iter->error = 1;
        errno = EILSEQ;
        return -1;
    }
    
    /* Decode the previous codepoint */
    int bytes_consumed;
    if (utf8_decode_sequence(prev_start, iter->end, &iter->codepoint, &bytes_consumed) != 0) {
        iter->error = 1;
        return -1;
    }
    
    iter->current = prev_start;
    iter->position--;
    
    return 1;
}

int u8i_has_prev(const utf8_iterator *iter) {
    if (!iter || iter->error) {
        return 0;
    }
    
    return iter->current > iter->data;
}

uint32_t u8i_codepoint(const utf8_iterator *iter) {
    return iter ? iter->codepoint : 0;
}

size_t u8i_position(const utf8_iterator *iter) {
    return iter ? iter->position : 0;
}

const char *u8i_current_ptr(const utf8_iterator *iter) {
    return iter ? iter->current : NULL;
}

int u8i_error(const utf8_iterator *iter) {
    return iter ? iter->error : 1;
}

int u8i_reset(utf8_iterator *iter) {
    if (!iter) {
        errno = EINVAL;
        return -1;
    }
    
    iter->current = iter->data;
    iter->position = 0;
    iter->error = 0;
    
    /* Decode first codepoint if data is not empty */
    if (iter->current < iter->end) {
        int bytes_consumed;
        if (utf8_decode_sequence(iter->current, iter->end, &iter->codepoint, &bytes_consumed) != 0) {
            iter->error = 1;
            return -1;
        }
    } else {
        iter->codepoint = 0;
    }
    
    return 0;
}

int u8i_seek(utf8_iterator *iter, size_t position) {
    if (!iter) {
        errno = EINVAL;
        return -1;
    }
    
    /* Reset to beginning */
    if (u8i_reset(iter) != 0) {
        return -1;
    }
    
    /* Move forward to desired position */
    for (size_t i = 0; i < position && u8i_has_next(iter); i++) {
        if (u8i_next(iter) < 0) {
            return -1;
        }
    }
    
    return 0;
}

size_t u8i_remaining(const utf8_iterator *iter) {
    if (!iter || iter->error) {
        return 0;
    }
    
    /* Count remaining codepoints by iterating */
    utf8_iterator temp = *iter;
    size_t count = 0;
    
    while (u8i_has_next(&temp)) {
        if (u8i_next(&temp) < 0) {
            break;
        }
        count++;
    }
    
    return count;
}

/* Relative access */
uint32_t u8i_at(const utf8_iterator *iter, int offset, _Bool is_byte_offset) {
    if (!iter || iter->error) {
        errno = EINVAL;
        return 0;
    }
    
    /* Create a copy to avoid modifying the original iterator */
    utf8_iterator temp = *iter;
    
    if (is_byte_offset) {
        /* Byte offset relative to current position */
        const char *target_ptr = temp.current + offset;

        if (target_ptr < temp.data || target_ptr >= temp.end) {
            errno = EINVAL;
            return 0;
        }

        /* Decode UTF-8 sequence at the target position directly */
        uint32_t cp = 0;
        int bytes_consumed = 0;
        if (utf8_decode_sequence(target_ptr, temp.end, &cp, &bytes_consumed) != 0) {
            errno = EILSEQ;
            return 0;
        }
        return cp;

    } else {
        /* Codepoint offset relative to current position */
        if (offset > 0) {
            /* Move forward */
            for (int i = 0; i < offset; i++) {
                if (u8i_next(&temp) < 0) {
                    errno = EINVAL;
                    return 0;
                }
            }
        } else if (offset < 0) {
            /* Move backward */
            for (int i = 0; i > offset; i--) {
                if (u8i_prev(&temp) < 0) {
                    errno = EINVAL;
                    return 0;
                }
            }
        }
        
        /* Return current codepoint */
        if (temp.error) {
            errno = EILSEQ;
            return 0;
        }
        
        return temp.codepoint;
    }
}
