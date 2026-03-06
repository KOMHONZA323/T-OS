#ifndef _BOOTINFO_H_
#define _BOOTINFO_H_

#include <stdint.h>

typedef struct {
    uint64_t fb_base;
    uint64_t fb_size;
    uint32_t fb_width;
    uint32_t fb_height;
    uint32_t fb_pitch; // This is PixelsPerScanLine
} BootInfo;

#endif