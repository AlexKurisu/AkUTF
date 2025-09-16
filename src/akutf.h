#ifndef AKUTF_H
#define AKUTF_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

char *u8enc(const uint32_t *src);
uint32_t *u8dec(const char *src, _Bool replace);

#ifdef __cplusplus
};
#endif

#endif /* AKUTF_H */
