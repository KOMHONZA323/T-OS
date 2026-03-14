#ifndef _STRING16_H_
#define _STRING16_H_

#include "uefi.h"

static inline int strcmp16(CHAR16 *s1, CHAR16 *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *s1 - *s2;
}

static inline int strncmp16(CHAR16 *s1, CHAR16 *s2, UINTN n) {
    while (n && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) return 0;
    return *s1 - *s2;
}

static inline void strcpy16(CHAR16 *dest, CHAR16 *src, UINTN n) {
    if (n == 0) return;
    UINTN i;
    for (i = 0; i < n - 1 && src[i]; i++) {
        dest[i] = src[i];
    }
    dest[i] = 0;
}

static inline void strncpy16(CHAR16 *dest, CHAR16 *src, UINTN n) {
    if (n == 0) return;
    UINTN i;
    for (i = 0; i < n - 1 && src[i]; i++) {
        dest[i] = src[i];
    }
    dest[i] = 0;
}

static inline UINTN strlen16(CHAR16 *s) {
    if (!s) return 0;
    UINTN len = 0;
    while (*s++) {
        len++;
    }
    return len;
}

#endif
