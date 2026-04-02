#include "font_renderer.h"
#include "font.h"

void font_draw_char(BootInfo* bi, char c, uint32_t x, uint32_t y, uint32_t color, int scale) {
    if (bi->fb_base == 0 || (uint8_t)c > 127 || scale <= 0) return;
    
    uint32_t* fb = (uint32_t*)bi->fb_base;
    uint32_t fb_width = bi->fb_width;
    uint32_t fb_height = bi->fb_height;
    uint32_t fb_pitch = bi->fb_pitch;

    // font8x8_basic is defined in font.h
    const uint8_t* glyph = (const uint8_t*)font8x8_basic[(uint8_t)c];
    
    for (int i = 0; i < 8; i++) {
        uint8_t row_data = glyph[i];
        if (!row_data) continue;

        uint32_t base_py = y + i * scale;
        if (base_py >= fb_height) break;
        uint32_t py_max = base_py + scale;
        if (py_max > fb_height) py_max = fb_height;

        for (int j = 0; j < 8; j++) {
            if ((row_data >> j) & 1) {
                uint32_t base_px = x + j * scale; // Standard 8x8 font ordering
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

void font_draw_string(BootInfo* bi, const char* s, uint32_t x, uint32_t y, uint32_t color, int scale) {
    while (*s) {
        font_draw_char(bi, *s, x, y, color, scale);
        x += 8 * scale;
        s++;
    }
}
