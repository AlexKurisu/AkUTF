#include <stdio.h>
#include <stdlib.h>

#include "akutf.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Not enough arguments\n");
        exit(EXIT_FAILURE);
    }

    const char *str_in = argv[1];
    uint32_t *u32 = u8_decode(str_in, 0);
    if (!u32) {
        perror("u8_decode");
        exit(EXIT_FAILURE);
    }

    printf("%s\n", str_in);

    for (uint32_t *it = u32; *it != 0x0; it++) {
        printf("%x ", *it);
    }
    puts("");

    free(u32);
    return 0;
}
