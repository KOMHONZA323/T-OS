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
    CHAR16 s_wide1[] = {0x1234, 0x5678, 0};
    CHAR16 s_wide2[] = {0x1234, 0x5678, 0};
    CHAR16 s_wide3[] = {0x1234, 0x5679, 0};

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

    ASSERT_EQ(strcmp16(s_wide1, s_wide2), 0, "Identical wide strings should return 0");

    res = strcmp16(s_wide1, s_wide3);
    ASSERT_EQ(res < 0, 1, "s_wide1 < s_wide3 should return negative");
}

int main() {
    test_strcmp16();

    printf("\nTest Summary: %d run, %d failed\n", tests_run, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
