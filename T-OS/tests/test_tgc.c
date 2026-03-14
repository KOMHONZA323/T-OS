#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

// Mocking needed for tgc tests
#define T_ID 1
#define T_NUM 2
// ... (rest of TokenType)

typedef struct {
    char name[32];
    int val;
} Var;

Var vars[128];
int var_cnt = 0;

int tgc_strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

void tgc_strncpy(char *d, const char *s, int n) {
    int i = 0;
    while (*s && i < n - 1) { *d++ = *s++; i++; }
    *d = 0;
}

int get_var(char *name) {
    for (int i = 0; i < var_cnt; i++) {
        if (tgc_strcmp(vars[i].name, name) == 0) return i;
    }
    tgc_strncpy(vars[var_cnt].name, name, 32);
    vars[var_cnt].val = 0;
    return var_cnt++;
}

int tgc_is_digit(char c) { return c >= '0' && c <= '9'; }
int tgc_is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

int tests_run = 0;
int tests_failed = 0;

#define ASSERT_EQ(actual, expected, msg) \
    tests_run++; \
    if ((actual) != (expected)) { \
        printf("FAILED: %s (Expected %d, got %d)\n", msg, (int)(expected), (int)(actual)); \
        tests_failed++; \
    } else { \
        printf("PASSED: %s\n", msg); \
    }

void test_tgc_utils() {
    printf("Running tgc utility tests...\n");
    ASSERT_EQ(tgc_is_digit('5'), 1, "is_digit('5') should be true");
    ASSERT_EQ(tgc_is_digit('a'), 0, "is_digit('a') should be false");
    ASSERT_EQ(tgc_is_alpha('a'), 1, "is_alpha('a') should be true");
    ASSERT_EQ(tgc_is_alpha('_'), 1, "is_alpha('_') should be true");
    ASSERT_EQ(tgc_is_alpha('1'), 0, "is_alpha('1') should be false");
    
    ASSERT_EQ(tgc_strcmp("abc", "abc"), 0, "tgc_strcmp identical");
    ASSERT_EQ(tgc_strcmp("abc", "abd") < 0, 1, "tgc_strcmp less");
}

void test_get_var() {
    printf("Running get_var tests...\n");
    var_cnt = 0;
    int v1 = get_var("test");
    ASSERT_EQ(v1, 0, "First var should be index 0");
    int v2 = get_var("abc");
    ASSERT_EQ(v2, 1, "Second var should be index 1");
    int v3 = get_var("test");
    ASSERT_EQ(v3, 0, "Existing var should return same index");
}

int main() {
    test_tgc_utils();
    test_get_var();
    
    printf("\nTest Summary: %d run, %d failed\n", tests_run, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
