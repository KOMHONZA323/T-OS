#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

// Mock UEFI types for host-side testing
typedef uint16_t CHAR16;
typedef uint64_t UINTN;
#define EFIAPI
#define EFI_STATUS UINTN

#define _UEFI_H_ // Prevent including the real uefi.h to avoid redefinitions

// Include the header under test
#include "../string16.h"

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

void test_strcmp16() {
    printf("Running strcmp16 tests...\n");

    CHAR16 s_empty[] = {0};
    CHAR16 s_abc[] = {'a', 'b', 'c', 0};
    CHAR16 s_abc2[] = {'a', 'b', 'c', 0};
    CHAR16 s_abd[] = {'a', 'b', 'd', 0};
    CHAR16 s_ab[] = {'a', 'b', 0};
    CHAR16 s_abcd[] = {'a', 'b', 'c', 'd', 0};

    ASSERT_EQ(strcmp16(s_abc, s_abc2), 0, "Identical strings should return 0");
    ASSERT_EQ(strcmp16(s_empty, s_empty), 0, "Two empty strings should return 0");

    int res = strcmp16(s_abc, s_abd);
    ASSERT_EQ(res < 0, 1, "s_abc < s_abd should return negative");

    res = strcmp16(s_abd, s_abc);
    ASSERT_EQ(res > 0, 1, "s_abd > s_abc should return positive");

    res = strcmp16(s_abc, s_ab);
    ASSERT_EQ(res > 0, 1, "s_abc > s_ab should return positive");

    res = strcmp16(s_ab, s_abc);
    ASSERT_EQ(res < 0, 1, "s_ab < s_abc should return negative");
}

void test_strncmp16() {
    printf("Running strncmp16 tests...\n");

    CHAR16 s1[] = {'a', 'b', 'c', 0};
    CHAR16 s2[] = {'a', 'b', 'd', 0};

    ASSERT_EQ(strncmp16(s1, s2, 2), 0, "strncmp16 with match should return 0");
    ASSERT_EQ(strncmp16(s1, s2, 3) < 0, 1, "strncmp16 with mismatch should return negative");
    ASSERT_EQ(strncmp16(s1, s2, 0), 0, "strncmp16 with n=0 should return 0");
}

void test_strcpy16() {
    printf("Running strcpy16 tests...\n");

    CHAR16 s_src[] = {'a', 'b', 'c', 0};
    CHAR16 s_dest[4];

    strcpy16(s_dest, s_src, 4);
    ASSERT_EQ(strcmp16(s_dest, s_src), 0, "strcpy16 should copy fully");
}

void test_strncpy16() {
    printf("Running strncpy16 tests...\n");

    CHAR16 s_src[] = {'a', 'b', 'c', 'd', 0};
    CHAR16 s_dest[5];
    CHAR16 s_dest_short[3];

    strncpy16(s_dest, s_src, 5);
    ASSERT_EQ(strcmp16(s_dest, s_src), 0, "Normal strncpy16 should copy fully");

    strncpy16(s_dest_short, s_src, 3);
    CHAR16 s_expected_short[] = {'a', 'b', 0};
    ASSERT_EQ(strcmp16(s_dest_short, s_expected_short), 0, "Truncated strncpy16 should add null terminator");

    CHAR16 s_dest_zero[5] = {1, 2, 3, 4, 5};
    strncpy16(s_dest_zero, s_src, 0);
    ASSERT_EQ(s_dest_zero[0], 1, "strncpy16 with n=0 should not modify dest");
}

void test_strlen16() {
    printf("Running strlen16 tests...\n");

    CHAR16 s_empty[] = {0};
    CHAR16 s_abc[] = {'a', 'b', 'c', 0};

    ASSERT_EQ(strlen16(s_empty), 0, "strlen16 on empty string should be 0");
    ASSERT_EQ(strlen16(s_abc), 3, "strlen16 on 'abc' should be 3");

    ASSERT_EQ(strlen16(NULL), 0, "strlen16 on NULL should be 0");
}

int main() {
    test_strcmp16();
    test_strncmp16();
    test_strcpy16();
    test_strncpy16();
    test_strlen16();

    printf("\nTest Summary: %d run, %d failed\n", tests_run, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
