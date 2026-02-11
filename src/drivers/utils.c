#include "utils.h"

void memory_copy(char *source, char *dest, int n_bytes) {
    int i;
    for (i = 0; i < n_bytes; i++) {
        *(dest + i) = *(source + i);
    }
}

void memory_set(char *dest, char val, int len) {
    char *temp = (char *)dest;
    for ( ; len != 0; len--) *temp++ = val;
}

int strlen(const char *s) {
    int i = 0;
    while (s[i] != '\0') ++i;
    return i;
}

void reverse(char s[]) {
    int i, j;
    char c;
    for (i = 0, j = strlen(s)-1; i < j; i++, j--) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

void itoa(int n, char str[]) {
    int i, sign;
    if ((sign = n) < 0) n = -n;
    i = 0;
    do {
        str[i++] = n % 10 + '0';
    } while ((n /= 10) > 0);
    if (sign < 0) str[i++] = '-';
    str[i] = '\0';
    reverse(str);
}

void delay(int count) {
    while(count > 0) count--;
}
