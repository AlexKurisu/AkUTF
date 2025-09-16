#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include "../src/akutf.h"
#include "../src/utf8_string.h"
#include "../src/utf8_iterator.h"
#include "../src/utf8_utils.h"

static void run_comprehensive_tests(void) {
    int passed = 0, total = 0;

    /* ============ akutf.h tests ============ */
    printf("Testing akutf.h functions:\n");
    
    /* test u8enc */
    total++;
    errno = 0;
    char *enc_null = u8enc(NULL);
    if (!enc_null && errno == EINVAL) {
        printf("[+] u8enc NULL input\n");
        passed++;
    } else {
        printf("[-] u8enc NULL input\n");
    }
    
    total++;
    uint32_t test_cps[] = {0x41, 0x42, 0x43, 0}; /* ABC */
    char *encoded = u8enc(test_cps);
    if (encoded && strcmp(encoded, "ABC") == 0) {
        printf("[+] u8enc basic test\n");
        passed++;
    } else {
        printf("[-] u8enc basic test\n");
    }
    free(encoded);
    
    /* test u8dec */
    total++;
    errno = 0;
    uint32_t *dec_null = u8dec(NULL, 0);
    if (!dec_null && errno == EINVAL) {
        printf("[+] u8dec NULL input\n");
        passed++;
    } else {
        printf("[-] u8dec NULL input\n");
    }
    
    total++;
    uint32_t *decoded = u8dec("ABC", 0);
    if (decoded && decoded[0] == 0x41 && decoded[1] == 0x42 && decoded[2] == 0x43 && decoded[3] == 0) {
        printf("[+] u8dec basic test\n");
        passed++;
    } else {
        printf("[-] u8dec basic test\n");
    }
    free(decoded);
    
    /* ============ utf8_string.h tests ============ */
    printf("\nTesting utf8_string.h functions:\n");
    
    /* Creation functions */
    total++;
    errno = 0;
    utf8_string *str_null = u8s_new(NULL);
    if (!str_null && errno == EINVAL) {
        printf("[+] u8s_new NULL input\n");
        passed++;
    } else {
        printf("[-] u8s_new NULL input\n");
    }
    
    total++;
    utf8_string *str1 = u8s_new("Hello");
    if (str1 && str1->byte_len == 5 && str1->codepoint_len == 5) {
        printf("[+] u8s_new basic\n");
        passed++;
    } else {
        printf("[-] u8s_new basic\n");
    }
    
    total++;
    utf8_string *str_cap = u8s_new_with_capacity(100);
    if (str_cap && str_cap->capacity >= 100) {
        printf("[+] u8s_new_with_capacity\n");
        passed++;
    } else {
        printf("[-] u8s_new_with_capacity\n");
    }
    
    total++;
    utf8_string *str_copy = u8s_copy(str1);
    if (str_copy && str_copy->byte_len == str1->byte_len) {
        printf("[+] u8s_copy\n");
        passed++;
    } else {
        printf("[-] u8s_copy\n");
    }
    
    total++;
    utf8_string *move_src = u8s_new("TestMove");
    utf8_string *move_dest = u8s_new_with_capacity(50);
    if (move_src && move_dest && u8s_move(move_dest, move_src)) {
        printf("[+] u8s_move\n");
        passed++;
    } else {
        printf("[-] u8s_move\n");
    }
    
    /* Basic operations */
    total++;
    utf8_string *empty_str = u8s_new("");
    if (empty_str && u8s_empty(empty_str) && !u8s_empty(str1)) {
        printf("[+] u8s_empty\n");
        passed++;
    } else {
        printf("[-] u8s_empty\n");
    }
    
    /* Concatenation with CORRECT TESTS */
    total++;
    utf8_string *cat_test = u8s_new("Hello");
    utf8_string *cat_src = u8s_new(" World");
    if (cat_test && cat_src && u8s_cat(cat_test, cat_src) == 0) {
        printf("[+] u8s_cat\n");
        passed++;
    } else {
        printf("[-] u8s_cat\n");
    }
    
    total++;
    utf8_string *cat_cstr_test = u8s_new("Test");
    if (cat_cstr_test && u8s_cat_cstr(cat_cstr_test, "!") == 0) {
        printf("[+] u8s_cat_cstr\n");
        passed++;
    } else {
        printf("[-] u8s_cat_cstr\n");
    }
    
    total++;
    utf8_string *cat_cp_test = u8s_new("Hi");
    if (cat_cp_test && u8s_cat_cp(cat_cp_test, 0x41) == 0) { /* A */
        printf("[+] u8s_cat_cp\n");
        passed++;
    } else {
        printf("[-] u8s_cat_cp\n");
    }
    
    /* Access operations */
    total++;
    utf8_string *at_test = u8s_new("ABC");
    if (at_test && u8s_at(at_test, 1, 0) == 0x42) { /* B */
        printf("[+] u8s_at\n");
        passed++;
    } else {
        printf("[-] u8s_at\n");
    }
    
    /* Memory management */
    total++;
    utf8_string *clear_test = u8s_new("Test");
    if (clear_test) {
        u8s_clear(clear_test);
        if (clear_test->byte_len == 0 && clear_test->codepoint_len == 0) {
            printf("[+] u8s_clear\n");
            passed++;
        } else {
            printf("[-] u8s_clear\n");
        }
    } else {
        total--;
    }
    
    /* ============ utf8_iterator.h tests ============ */
    printf("\nTesting utf8_iterator.h functions:\n");

    /* Creation and basic state */
    total++;
    errno = 0;
    utf8_iterator bad_iter = u8i_new(NULL);
    if (u8i_error(&bad_iter) && errno == EINVAL) {
        printf("[+] u8i_new NULL input\n");
        passed++;
    } else {
        printf("[-] u8i_new NULL input\n");
    }

    const char *iter_s = "A\xF0\x9F\x98\x80" "B"; /* A + 😀 + B */
    utf8_iterator it = u8i_new(iter_s);

    total++;
    if (!u8i_error(&it) && u8i_has_next(&it) && u8i_position(&it) == 0) {
        printf("[+] u8i_new basic state\n");
        passed++;
    } else {
        printf("[-] u8i_new basic state\n");
    }

    /* Forward iteration */
    total++;
    if (u8i_next(&it) == 1 && u8i_codepoint(&it) == 0x41 && u8i_position(&it) == 1) {
        printf("[+] u8i_next #1 (A)\n");
        passed++;
    } else {
        printf("[-] u8i_next #1 (A)\n");
    }

    total++;
    if (u8i_next(&it) == 1 && u8i_codepoint(&it) == 0x1F600 && u8i_position(&it) == 2) {
        printf("[+] u8i_next #2 (😀)\n");
        passed++;
    } else {
        printf("[-] u8i_next #2 (😀)\n");
    }

    total++;
    if (u8i_next(&it) == 1 && u8i_codepoint(&it) == 0x42 && u8i_position(&it) == 3 && !u8i_has_next(&it)) {
        printf("[+] u8i_next #3 (B) and end\n");
        passed++;
    } else {
        printf("[-] u8i_next #3 (B) and end\n");
    }

    total++;
    if (u8i_next(&it) == 0 && u8i_position(&it) == 3) {
        printf("[+] u8i_next at end returns 0\n");
        passed++;
    } else {
        printf("[-] u8i_next at end returns 0\n");
    }

    /* Backward iteration */
    total++;
    if (u8i_has_prev(&it) && u8i_prev(&it) == 1 && u8i_codepoint(&it) == 0x42 && u8i_position(&it) == 2) {
        printf("[+] u8i_prev from end (B)\n");
        passed++;
    } else {
        printf("[-] u8i_prev from end (B)\n");
    }

    /* Reset and seek */
    total++;
    if (u8i_reset(&it) == 0 && u8i_position(&it) == 0 && u8i_codepoint(&it) == 0x41) {
        printf("[+] u8i_reset sets to start and codepoint (A)\n");
        passed++;
    } else {
        printf("[-] u8i_reset sets to start and codepoint (A)\n");
    }

    total++;
    if (u8i_seek(&it, 2) == 0 && u8i_position(&it) == 2 && u8i_codepoint(&it) == 0x1F600) {
        printf("[+] u8i_seek to position 2 (😀)\n");
        passed++;
    } else {
        printf("[-] u8i_seek to position 2 (😀)\n");
    }

    /* Remaining count */
    total++;
    if (u8i_reset(&it) == 0 && u8i_remaining(&it) == 3) {
        printf("[+] u8i_remaining from start is 3\n");
        passed++;
    } else {
        printf("[-] u8i_remaining from start is 3\n");
    }

    total++;
    (void)u8i_next(&it);
    if (u8i_remaining(&it) == 2) {
        printf("[+] u8i_remaining after one next is 2\n");
        passed++;
    } else {
        printf("[-] u8i_remaining after one next is 2\n");
    }

    /* Pointer movement */
    total++;
    (void)u8i_reset(&it);
    (void)u8i_next(&it);
    if ((size_t)(u8i_current_ptr(&it) - iter_s) == 1) {
        printf("[+] u8i_current_ptr moved by 1 byte for ASCII\n");
        passed++;
    } else {
        printf("[-] u8i_current_ptr moved by 1 byte for ASCII\n");
    }

    /* Relative access: byte offsets */
    total++;
    (void)u8i_reset(&it);
    uint32_t cp_at0 = u8i_at(&it, 0, 1);
    if (cp_at0 == 0x41) {
        printf("[+] u8i_at byte offset 0 -> A\n");
        passed++;
    } else {
        printf("[-] u8i_at byte offset 0 -> A\n");
    }

    total++;
    uint32_t cp_at1 = u8i_at(&it, 1, 1);
    if (cp_at1 == 0x1F600) {
        printf("[+] u8i_at byte offset 1 -> 😀\n");
        passed++;
    } else {
        printf("[-] u8i_at byte offset 1 -> 😀\n");
    }

    /* Relative access: codepoint offsets */
    total++;
    (void)u8i_reset(&it);
    uint32_t cp_off2 = u8i_at(&it, 2, 0);
    if (cp_off2 == 0x1F600) {
        printf("[+] u8i_at codepoint offset 2 -> 😀\n");
        passed++;
    } else {
        printf("[-] u8i_at codepoint offset 2 -> 😀\n");
    }

    /* Error handling on invalid sequence */
    total++;
    const char *bad_s = "A\xFF" "B"; /* invalid continuation */
    utf8_iterator it_bad = u8i_new(bad_s);
    int step1 = u8i_next(&it_bad); /* 'A' ok */
    int step2 = u8i_next(&it_bad); /* should fail on 0xFF */
    if (step1 == 1 && step2 < 0 && u8i_error(&it_bad)) {
        printf("[+] u8i_next detects invalid UTF-8\n");
        passed++;
    } else {
        printf("[-] u8i_next detects invalid UTF-8\n");
    }

    /* ============ utf8_utils.h tests ============ */
    printf("\nTesting utf8_utils.h functions:\n");
    
    /* Validation */
    total++;
    if (u8_valid_cstr("Hello") && !u8_valid_cstr("\xFF\xFF")) {
        printf("[+] u8_valid_cstr\n");
        passed++;
    } else {
        printf("[-] u8_valid_cstr\n");
    }
    
    total++;
    if (u8_len_cstr("Hello") == 5) {
        printf("[+] u8_len_cstr\n");
        passed++;
    } else {
        printf("[-] u8_len_cstr\n");
    }
    
    /* Search */
    total++;
    const char *found = u8_chr("Hello", 'l');
    if (found && *found == 'l') {
        printf("[+] u8_chr\n");
        passed++;
    } else {
        printf("[-] u8_chr\n");
    }
    
    /* Character classification */
    total++;
    if (u8_ascii(0x41) && !u8_ascii(0x1F600)) { /* A vs emoji */
        printf("[+] u8_ascii\n");
        passed++;
    } else {
        printf("[-] u8_ascii\n");
    }
    
    total++;
    if (u8_alpha(0x41) && !u8_alpha(0x31)) { /* A vs 1 */
        printf("[+] u8_alpha\n");
        passed++;
    } else {
        printf("[-] u8_alpha\n");
    }
    
    total++;
    if (u8_digit(0x31) && !u8_digit(0x41)) { /* 1 vs A */
        printf("[+] u8_digit\n");
        passed++;
    } else {
        printf("[-] u8_digit\n");
    }
    
    /* Utility */
    total++;
    if (u8_cp_bytelen(0x41) == 1 && u8_cp_bytelen(0x1F600) == 4) { /* A vs emoji */
        printf("[+] u8_cp_bytelen\n");
        passed++;
    } else {
        printf("[-] u8_cp_bytelen\n");
    }
    
    /* Cleanup */
    u8s_free(str1);
    u8s_free(str_cap);
    u8s_free(str_copy);
    u8s_free(move_src);
    u8s_free(move_dest);
    u8s_free(empty_str);
    u8s_free(cat_src);
    u8s_free(cat_test);
    u8s_free(cat_cstr_test);
    u8s_free(cat_cp_test);
    u8s_free(at_test);
    u8s_free(clear_test);
    
    printf("\nRESULTS:\n");
    printf("    - Total functions tested: %d\n", total);
    printf("    - Tests passed: %d\n", passed);
    printf("    - Tests failed: %d\n\n", total - passed);

    if (passed == total) {
        printf("[DONE] ALL TESTS PASSED!\n");
    } else {
        printf("[FAIL] Some tests failed.\n");
        exit(1);
    }
}

static double timespec_diff_sec(struct timespec a, struct timespec b) {
    return (b.tv_sec - a.tv_sec) + (b.tv_nsec - a.tv_nsec) / 1e9;
}

static void run_benchmarks(unsigned long iters) {
    printf(">> BENCHMARKS (iterations: %lu)\n\n", iters);

    volatile unsigned long sink = 0; /* prevent optimization */
    int errors = 0;
    struct timespec t0, t1;

    /* Sample data */
    const char *s_utf8 = "A\xF0\x9F\x98\x80" "B"; /* A😀B */
    uint32_t cps[] = {0x41, 0x1F600, 0x42, 0};

    /* u8dec */
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (unsigned long i = 0; i < iters; i++) {
        uint32_t *arr = u8dec(s_utf8, 0);
        if (!arr) { errors++; continue; }
        sink += arr[0];
        free(arr);
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double dec_sec = timespec_diff_sec(t0, t1);
    printf("u8dec:   %8.3f ms total, %6.1f ns/op, %7.1f Mops/s\n",
           dec_sec * 1e3, (dec_sec * 1e9) / iters, (iters / dec_sec) / 1e6);

    /* u8enc */
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (unsigned long i = 0; i < iters; i++) {
        char *enc = u8enc(cps);
        if (!enc) { errors++; continue; }
        sink += (unsigned long)(unsigned char)enc[0];
        free(enc);
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double enc_sec = timespec_diff_sec(t0, t1);
    printf("u8enc:   %8.3f ms total, %6.1f ns/op, %7.1f Mops/s\n",
           enc_sec * 1e3, (enc_sec * 1e9) / iters, (iters / enc_sec) / 1e6);

    /* utf8_iterator: iterate over string */
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (unsigned long i = 0; i < iters; i++) {
        utf8_iterator it = u8i_new(s_utf8);
        while (u8i_next(&it) > 0) {
            sink += u8i_codepoint(&it);
        }
        if (u8i_error(&it)) errors++;
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double iter_sec = timespec_diff_sec(t0, t1);
    printf("iterator:%8.3f ms total, %6.1f ns/op, %7.1f Mops/s\n",
           iter_sec * 1e3, (iter_sec * 1e9) / iters, (iters / iter_sec) / 1e6);

    /* utf8_utils: length + validation + cp_bytelen */
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (unsigned long i = 0; i < iters; i++) {
        sink += u8_len_cstr(s_utf8);
        sink += u8_valid_cstr(s_utf8);
        sink += u8_cp_bytelen(0x41);
        sink += u8_cp_bytelen(0x1F600);
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double utils_sec = timespec_diff_sec(t0, t1);
    printf("utils:   %8.3f ms total, %6.1f ns/op, %7.1f Mops/s\n",
           utils_sec * 1e3, (utils_sec * 1e9) / iters, (iters / utils_sec) / 1e6);

    /* utf8_string: new + cat + free */
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (unsigned long i = 0; i < iters; i++) {
        utf8_string *a = u8s_new("Hello");
        utf8_string *b = u8s_new(" ");
        (void)u8s_cat(b, a);
        sink += b ? b->byte_len : 0;
        u8s_free(a);
        u8s_free(b);
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double str_sec = timespec_diff_sec(t0, t1);
    printf("string:  %8.3f ms total, %6.1f ns/op, %7.1f Mops/s\n",
           str_sec * 1e3, (str_sec * 1e9) / iters, (iters / str_sec) / 1e6);

    if (errors) {
        printf("\nBenchmark completed with %d internal errors (ignored).\n", errors);
    }
    /* prevent sink from being optimized away */
    if (sink == 42) puts("");
}

int main(int argc, char **argv) {
    if (argc > 1 && strcmp(argv[1], "--benchmark") == 0) {
        unsigned long iters = 10000000UL;
        const char *env_iters = getenv("AKUTF_BENCH_ITERS");
        if (env_iters) {
            unsigned long v = strtoul(env_iters, NULL, 10);
            if (v > 0) iters = v;
        }
        run_benchmarks(iters);
        return 0;
    }

    run_comprehensive_tests();
    return 0;
}
