#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <string.h>

#define FB_WIDTH 1920
#define FB_HEIGHT 1080
uint32_t fb[FB_WIDTH * FB_HEIGHT];
uint32_t fb_pitch = FB_WIDTH;

static uint8_t font[128][8] = {
    [0x41] = {0x18, 0x3c, 0x66, 0x66, 0x7e, 0x66, 0x66, 0}, // A
};

void draw_char_old(char c, uint32_t x, uint32_t y, uint32_t color, int scale) {
    uint8_t* glyph = font[(uint8_t)c];
    for (int i = 0; i < 8; i++) {
        uint8_t row_data = glyph[i];
        if (!row_data) continue;

        uint32_t base_py = y + i * scale;
        if (base_py >= FB_HEIGHT) break;
        uint32_t py_max = base_py + scale;
        if (py_max > FB_HEIGHT) py_max = FB_HEIGHT;

        for (int j = 0; j < 8; j++) {
            if ((row_data >> j) & 1) {
                uint32_t base_px = x + (7 - j) * scale;
                if (base_px >= FB_WIDTH) continue;
                uint32_t px_max = base_px + scale;
                if (px_max > FB_WIDTH) px_max = FB_WIDTH;

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

void draw_char_new(char c, uint32_t x, uint32_t y, uint32_t color, int scale) {
    uint8_t* glyph = font[(uint8_t)c];
    for (int i = 0; i < 8; i++) {
        uint8_t row_data = glyph[i];
        if (!row_data) continue;

        uint32_t base_py = y + i * scale;
        if (base_py >= FB_HEIGHT) break;
        uint32_t py_max = base_py + scale;
        if (py_max > FB_HEIGHT) py_max = FB_HEIGHT;

        for (int j = 0; j < 8; j++) {
            if ((row_data >> j) & 1) {
                uint32_t base_px = x + (7 - j) * scale;
                if (base_px >= FB_WIDTH) continue;
                uint32_t px_max = base_px + scale;
                if (px_max > FB_WIDTH) px_max = FB_WIDTH;

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
    int iterations = 100000;
    int scale = 10;

    clock_t start, end;
    double time_old, time_new;

    start = clock();
    for (int i = 0; i < iterations; i++) {
        for(int x = 0; x < 100; x += 10) {
            draw_char_old('A', x, x, 0xFFFFFF, scale);
        }
    }
    end = clock();
    time_old = ((double) (end - start)) / CLOCKS_PER_SEC;

    memset(fb, 0, sizeof(fb));

    start = clock();
    for (int i = 0; i < iterations; i++) {
        for(int x = 0; x < 100; x += 10) {
            draw_char_new('A', x, x, 0xFFFFFF, scale);
        }
    }
    end = clock();
    time_new = ((double) (end - start)) / CLOCKS_PER_SEC;

    printf("Old time: %f s\n", time_old);
    printf("New time: %f s\n", time_new);
    printf("Improvement: %.2f%%\n", (time_old - time_new) / time_old * 100.0);

    return 0;
}
