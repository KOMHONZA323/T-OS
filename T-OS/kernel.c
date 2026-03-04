#include <stdint.h>

// Match the struct from main.c
typedef struct {
    uint64_t fb_base;
    uint64_t fb_size;
    uint32_t fb_width;
    uint32_t fb_height;
    uint32_t fb_pitch; // This is PixelsPerScanLine
} BootInfo;

// A simple 8x8 font
static uint8_t font[128][8] = {
    // Basic characters for "SUCCESS"
    [0x42] = {0x7e, 0x18, 0x18, 0x18, 0x18, 0x18, 0x7e, 0}, // B
    [0x43] = {0x3c, 0x66, 0x06, 0x06, 0x06, 0x66, 0x3c, 0}, // C
    [0x45] = {0x7e, 0x06, 0x06, 0x3e, 0x06, 0x06, 0x7e, 0}, // E
    [0x4f] = {0x3c, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3c, 0}, // O
    [0x53] = {0x3c, 0x66, 0x06, 0x3c, 0x60, 0x66, 0x3c, 0}, // S
    [0x54] = {0x7e, 0x7e, 0x18, 0x18, 0x18, 0x18, 0x18, 0}, // T
    [0x55] = {0x66, 0x66, 0x66, 0x66, 0x66, 0x3e, 0x3c, 0}, // U
};

void draw_char(BootInfo* bi, char c, uint32_t x, uint32_t y, uint32_t color, int scale) {
    if (c > 127) return;
    uint32_t* fb = (uint32_t*)bi->fb_base;

    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            if ((font[(uint8_t)c][i] >> j) & 1) {
                for (int k = 0; k < scale; k++) {
                    for (int l = 0; l < scale; l++) {
                        uint32_t px = x + (8 - j - 1) * scale + k;
                        uint32_t py = y + i * scale + l;
                        if (px < bi->fb_width && py < bi->fb_height) {
                            fb[py * bi->fb_pitch + px] = color;
                        }
                    }
                }
            }
        }
    }
}

void draw_string(BootInfo* bi, const char* s, uint32_t x, uint32_t y, uint32_t color, int scale) {
    while (*s) {
        draw_char(bi, *s, x, y, color, scale);
        x += 8 * scale;
        s++;
    }
}

void kmain(BootInfo* bi) {
    uint32_t* fb = (uint32_t*)bi->fb_base;

    // Clear screen to black
    for (uint32_t i = 0; i < bi->fb_size / 4; i++) {
        fb[i] = 0x00000000;
    }

    // If we have a valid framebuffer, draw the success message
    if (bi->fb_base != 0 && bi->fb_width > 0 && bi->fb_height > 0) {
        uint32_t screen_center_x = bi->fb_width / 2;
        uint32_t screen_center_y = bi->fb_height / 2;
        
        // "BOOT"
        uint32_t boot_len = 4 * 8 * 5; // 4 chars, 8 pixels wide, 5x scale
        draw_string(bi, "BOOT", screen_center_x - boot_len / 2, screen_center_y - 8*5, 0xFF00FF00, 5);
        
        // "SUCCESS"
        uint32_t success_len = 7 * 8 * 5; // 7 chars
        draw_string(bi, "SUCCESS", screen_center_x - success_len / 2, screen_center_y, 0xFF00FF00, 5);
    }

    // Halt the system
    while(1) { __asm__ __volatile__("hlt"); }
}
