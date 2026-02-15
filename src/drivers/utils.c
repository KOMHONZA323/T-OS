#include "utils.h"

void memory_copy(char *source, char *dest, int nbytes) {
    // Try to copy 32-bit chunks if pointers are aligned (common for framebuffers)
    if (nbytes >= 4 && ((uint32_t)source & 3) == 0 && ((uint32_t)dest & 3) == 0) {
        int n_dwords = nbytes / 4;
        uint32_t *s = (uint32_t*)source;
        uint32_t *d = (uint32_t*)dest;

        // Calculate offset before n_dwords is clobbered by rep movsl
        int offset = n_dwords * 4;

        // Optimized 32-bit copy using rep movsl
        __asm__ volatile (
            "rep movsl"
            : "+S" (s), "+D" (d), "+c" (n_dwords)
            :
            : "memory"
        );

        // Adjust for remaining bytes
        source += offset;
        dest += offset;
        nbytes -= offset;
    }

    // Copy remaining bytes (or all bytes if unaligned)
    for (int i = 0; i < nbytes; i++) {
        *(dest + i) = *(source + i);
    }
}

void memory_set(char *dest, char val, int len) {
    char *temp = (char *)dest;
    for ( ; len != 0; len--) *temp++ = val;
}

void int_to_ascii(int n, char str[]) {
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

void reverse(char s[]) {
    int c, i, j;
    for (i = 0, j = strlen(s)-1; i < j; i++, j--) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

int strlen(char s[]) {
    int i = 0;
    while (s[i] != '\0') ++i;
    return i;
}

void delay(int count) {
    // A simple busy wait loop.
    // The exact duration depends on the CPU speed.
    // Since we are running in QEMU, this is purely visual.
    volatile int i;
    // Reduced multiplier significantly as QEMU/modern CPUs are fast
    // but the previous count was causing apparent hangs.
    for (i = 0; i < count * 1000; i++) {
        __asm__("nop");
    }
}
