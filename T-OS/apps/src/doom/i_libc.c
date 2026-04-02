#include <stddef.h>
#include <stdint.h>

// Global WAD pointer from bootloader/initrd
extern uint8_t* doom_wad_data;
extern uint32_t doom_wad_size;

static uint32_t current_wad_pos = 0;

void* malloc(size_t size) {
    extern void* kmalloc(uint64_t size);
    return kmalloc(size);
}

void free(void* ptr) {
    // T-OS uses a bump allocator, free is currently a no-op
}

int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++; s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

void* memcpy(void* dest, const void* src, size_t n) {
    char* d = dest;
    const char* s = src;
    while (n--) *d++ = *s++;
    return dest;
}

void* memset(void* s, int c, size_t n) {
    unsigned char* p = s;
    while (n--) *p++ = (unsigned char)c;
    return s;
}

// File I/O (RAM Disk based)
typedef struct {
    uint32_t offset;
    uint32_t size;
} FILE;

FILE* fopen(const char* filename, const char* mode) {
    // Only support DOOM1.WAD
    if (strcmp(filename, "doom1.wad") == 0) {
        static FILE wad;
        wad.offset = 0;
        wad.size = doom_wad_size;
        return &wad;
    }
    return NULL;
}

int fread(void* ptr, int size, int nmemb, FILE* stream) {
    int bytes = size * nmemb;
    if (current_wad_pos + bytes > doom_wad_size) {
        bytes = doom_wad_size - current_wad_pos;
    }
    memcpy(ptr, doom_wad_data + current_wad_pos, bytes);
    current_wad_pos += bytes;
    return bytes / size;
}

int fseek(FILE* stream, long offset, int whence) {
    if (whence == 0) current_wad_pos = offset; // SEEK_SET
    else if (whence == 1) current_wad_pos += offset; // SEEK_CUR
    return 0;
}

long ftell(FILE* stream) {
    return current_wad_pos;
}

void fclose(FILE* stream) {
    // No-op
}
