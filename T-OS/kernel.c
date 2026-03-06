#include <stdint.h>
#include "bootinfo.h"

// A simple 8x8 font
static uint8_t font[128][8] = {
    // Basic characters for "SUCCESS"
    [0x41] = {0x18, 0x3c, 0x66, 0x66, 0x7e, 0x66, 0x66, 0}, // A
    [0x42] = {0x7e, 0x18, 0x18, 0x18, 0x18, 0x18, 0x7e, 0}, // B
    [0x43] = {0x3c, 0x66, 0x06, 0x06, 0x06, 0x66, 0x3c, 0}, // C
    [0x45] = {0x7e, 0x06, 0x06, 0x3e, 0x06, 0x06, 0x7e, 0}, // E
    [0x49] = {0x7e, 0x18, 0x18, 0x18, 0x18, 0x18, 0x7e, 0}, // I
    [0x4e] = {0x66, 0x76, 0x7e, 0x6e, 0x66, 0x66, 0x66, 0}, // N
    [0x4f] = {0x3c, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3c, 0}, // O
    [0x50] = {0x7e, 0x66, 0x66, 0x7e, 0x06, 0x06, 0x06, 0}, // P
    [0x53] = {0x3c, 0x66, 0x06, 0x3c, 0x60, 0x66, 0x3c, 0}, // S
    [0x54] = {0x7e, 0x7e, 0x18, 0x18, 0x18, 0x18, 0x18, 0}, // T
    [0x55] = {0x66, 0x66, 0x66, 0x66, 0x66, 0x3e, 0x3c, 0}, // U
};

void draw_char(BootInfo* bi, char c, uint32_t x, uint32_t y, uint32_t color, int scale) {
    if (c > 127 || c < 0 || scale <= 0) return;
    uint32_t* fb = (uint32_t*)bi->fb_base;
    uint32_t fb_width = bi->fb_width;
    uint32_t fb_height = bi->fb_height;
    uint32_t fb_pitch = bi->fb_pitch;

    uint8_t* glyph = font[(uint8_t)c];
    for (int i = 0; i < 8; i++) {
        uint8_t row_data = glyph[i];
        if (!row_data) continue;

        uint32_t base_py = y + i * scale;
        if (base_py >= fb_height) break;
        uint32_t py_max = base_py + scale;
        if (py_max > fb_height) py_max = fb_height;

        for (int j = 0; j < 8; j++) {
            if ((row_data >> j) & 1) {
                uint32_t base_px = x + (7 - j) * scale;
                if (base_px >= fb_width) continue;
                uint32_t px_max = base_px + scale;
                if (px_max > fb_width) px_max = fb_width;

                for (uint32_t py = base_py; py < py_max; py++) {
                    uint32_t* row_ptr = &fb[py * fb_pitch];
                    for (uint32_t px = base_px; px < px_max; px++) {
                        row_ptr[px] = color;
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

void delay(uint64_t milliseconds) {
    // This is a very crude delay and depends on CPU speed.
    // A more accurate delay would require a timer interrupt.
    volatile uint64_t i;
    for (i = 0; i < milliseconds * 1000000; i++) {
        __asm__ __volatile__("nop");
    }
}

void kpanic(BootInfo* bi) {
    uint32_t* fb = (uint32_t*)bi->fb_base;

    // Clear screen to red for panic
    for (uint32_t i = 0; i < bi->fb_size / 4; i++) {
        fb[i] = 0x00FF0000; // Red color
    }

    if (bi->fb_base != 0 && bi->fb_width > 0 && bi->fb_height > 0) {
        uint32_t screen_center_x = bi->fb_width / 2;
        uint32_t screen_center_y = bi->fb_height / 2;
        
        uint32_t panic_len = 5 * 8 * 5; // 5 chars, 8 pixels wide, 5x scale
        draw_string(bi, "PANIC", screen_center_x - panic_len / 2, screen_center_y, 0xFFFFFFFF, 5); // White text
    }

    while(1) { __asm__ __volatile__("hlt"); }
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
    
    // Delay for 10 seconds (approximate)
    delay(10000); // 10000 milliseconds = 10 seconds

    // Trigger panic
    kpanic(bi);

    // This part should not be reached after kpanic
    while(1) { __asm__ __volatile__("hlt"); }
}
