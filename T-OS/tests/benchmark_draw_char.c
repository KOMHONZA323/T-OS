#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

// Mock BootInfo
typedef struct {
    uint64_t fb_base;
    uint32_t fb_size;
    uint32_t fb_width;
    uint32_t fb_height;
    uint32_t fb_pitch;
} BootInfo;

static uint8_t font[128][8] = {
    [0x41] = {0x18, 0x3c, 0x66, 0x66, 0x7e, 0x66, 0x66, 0}, // A
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

                uint32_t* row_ptr = &fb[base_py * fb_pitch];
                for (uint32_t py = base_py; py < py_max; py++) {
                    for (uint32_t px = base_px; px < px_max; px++) {
                        row_ptr[px] = color;
                    }
                    row_ptr += fb_pitch;
                }
            }
        }
    }
}

int main() {
    uint32_t fb_width = 800;
    uint32_t fb_height = 600;
    uint32_t fb_pitch = 800;
    uint32_t* fb = malloc(fb_height * fb_pitch * sizeof(uint32_t));

    BootInfo bi;
    bi.fb_base = (uint64_t)fb;
    bi.fb_size = fb_height * fb_pitch * sizeof(uint32_t);
    bi.fb_width = fb_width;
    bi.fb_height = fb_height;
    bi.fb_pitch = fb_pitch;

    clock_t start = clock();

    for (int i = 0; i < 1000000; i++) {
        draw_char(&bi, 0x41, 10, 10, 0xFFFFFFFF, 5);
    }

    clock_t end = clock();
    double cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;

    printf("Baseline Time taken: %f seconds\n", cpu_time_used);

    free(fb);
    return 0;
}
