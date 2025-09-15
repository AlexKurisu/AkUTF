#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include "akutf.h"

#define BENCHMARK_ITERATIONS 1000000

static double get_time(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

static int test_roundtrip(const char *input) {
    uint32_t *codepoints = u8_decode(input, 0);
    if (!codepoints) return 0;
    
    char *encoded = u8_encode(codepoints);
    if (!encoded) {
        free(codepoints);
        return 0;
    }
    
    int result = strcmp(input, encoded) == 0;
    free(codepoints);
    free(encoded);
    return result;
}

static int test_pure_encode(const uint32_t *codepoints, const char *expected) {
    char *encoded = u8_encode(codepoints);
    if (!encoded) return 0;
    
    int result = strcmp(encoded, expected) == 0;
    free(encoded);
    return result;
}

static void run_tests(void) {
    const char *tests[] = {
        "Hello",
        "café",
        "你好",
        "🚀",
        "你好世界! 👋 café résumé 🌍",
        ""
    };
    
    int passed = 0, total = 0;
    
    /* Roundtrip tests */
    for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
        total++;
        if (test_roundtrip(tests[i])) {
            printf("[+] %s\n", tests[i][0] ? tests[i] : "(empty)");
            passed++;
        } else {
            printf("[-] %s\n", tests[i][0] ? tests[i] : "(empty)");
        }
    }
    
    /* Pure encode tests */
    struct {
        uint32_t codepoints[8];
        const char *expected;
        const char *name;
    } encode_tests[] = {
        {{0x41, 0}, "A", "ASCII"},
        {{0x41, 0x42, 0x43, 0}, "ABC", "ASCII multi"},
        {{0xE9, 0}, "é", "Latin-1 extended"},
        {{0x4F60, 0x597D, 0}, "你好", "Chinese"},
        {{0x1F680, 0}, "🚀", "Emoji"},
        {{0x41, 0xE9, 0x4F60, 0x1F680, 0}, "Aé你🚀", "Mixed"},
        {{0x80, 0x7FF, 0x800, 0xFFFF, 0x10000, 0x10FFFF, 0}, 
         "\xC2\x80\xDF\xBF\xE0\xA0\x80\xEF\xBF\xBF\xF0\x90\x80\x80\xF4\x8F\xBF\xBF", 
         "Boundary values"}
    };
    
    for (size_t i = 0; i < sizeof(encode_tests) / sizeof(encode_tests[0]); i++) {
        total++;
        if (test_pure_encode(encode_tests[i].codepoints, encode_tests[i].expected)) {
            printf("[+] %s\n", encode_tests[i].name);
            passed++;
        } else {
            printf("[-] %s\n", encode_tests[i].name);
        }
    }
    
    /* NULL input test */
    total++;
    errno = 0;
    char *null_result = u8_encode(NULL);
    if (null_result == NULL && errno == EINVAL) {
        printf("[+] NULL input errno\n");
        passed++;
    } else {
        printf("[-] NULL input errno\n");
    }
    
    printf("\nTests: %d/%d passed\n", passed, total);
    if (passed != total) exit(1);
}

static void run_benchmark(void) {
    const char *text = "你好世界! 👋 こんにちは🌸 Hello café 🚀";
    size_t len = strlen(text);
    
    double total_decode = 0, total_encode = 0;
    
    for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
        double start = get_time();
        uint32_t *cp = u8_decode(text, 0);
        total_decode += get_time() - start;
        
        if (!cp) break;
        
        start = get_time();
        char *enc = u8_encode(cp);
        total_encode += get_time() - start;
        
        free(cp);
        free(enc);
    }
    
    printf("\nBenchmark (%d iterations):\n", BENCHMARK_ITERATIONS);
    printf("Decode: %.1f MB/s\n", (len * BENCHMARK_ITERATIONS) / (total_decode * 1024 * 1024));
    printf("Encode: %.1f MB/s\n", (len * BENCHMARK_ITERATIONS) / (total_encode * 1024 * 1024));
}

int main(int argc, char *argv[]) {
    if (argc > 1 && strcmp(argv[1], "--benchmark") == 0) {
        run_benchmark();
    } else {
        run_tests();
    }
    return 0;
}
