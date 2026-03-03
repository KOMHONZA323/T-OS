#include <stdint.h>

typedef struct {
    uint64_t fb_base;
    uint64_t fb_size;
    uint32_t fb_width;
    uint32_t fb_height;
    uint32_t fb_pitch;
} BootInfo;

void kmain(BootInfo* bi) {
    uint32_t* fb = (uint32_t*)bi->fb_base;
    uint32_t pitch = bi->fb_pitch;

    // Fill Blue
    for (uint32_t y = 0; y < bi->fb_height; y++) {
        for (uint32_t x = 0; x < bi->fb_width; x++) {
            fb[y * pitch + x] = 0xFF0000FF;
        }
    }

    // White Square in center
    uint32_t cx = bi->fb_width / 2;
    uint32_t cy = bi->fb_height / 2;
    for (int y = -50; y < 50; y++) {
        for (int x = -50; x < 50; x++) {
            fb[(cy + y) * pitch + (cx + x)] = 0xFFFFFFFF;
        }
    }

    while(1) { __asm__ __volatile__("hlt"); }
}
