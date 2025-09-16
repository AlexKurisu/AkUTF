#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "akutf.h"
#include "utf8_constants.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Not enough arguments\n");
        exit(EXIT_FAILURE);
    }

    const char *str_in = argv[1];
    size_t len = strlen(str_in);
    uint32_t *u32 = u8dec(str_in, len);
    if (!u32) {
        perror("u8dec");
        exit(EXIT_FAILURE);
    }

    printf("%s\n", str_in);

    for (uint32_t *it = u32; *it != UTF8_NULL_BYTE; it++) {
        printf("%x ", *it);
    }
    puts("");

    free(u32);
    return 0;
}
