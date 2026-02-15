#include "std.glh"

// Minimal libc stubs
void* memcpy(void* dest, const void* src, int n) {
    char* d = (char*)dest;
    const char* s = (const char*)src;
    while (n--) *d++ = *s++;
    return dest;
}

void* memset(void* s, int c, int n) {
    char* p = (char*)s;
    while (n--) *p++ = c;
    return s;
}

// Any other symbols gcc might emit
// int strlen(const char* s) { ... } // tasm.c has its own implementation.
