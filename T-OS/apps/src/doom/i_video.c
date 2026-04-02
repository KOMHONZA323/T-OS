#include <stdint.h>

// DOOM internal types
typedef uint8_t byte;

// T-OS provided info
typedef struct {
    uint64_t fb_base;
    uint32_t fb_width;
    uint32_t fb_height;
    uint32_t fb_pitch;
} BootInfo_Internal;

extern BootInfo_Internal global_boot_info;
uint32_t doom_palette[256];
extern byte* screens[5];

void I_InitGraphics(void) {
    // Already handled by T-OS kernel/bootloader
}

void I_ShutdownGraphics(void) {
    // Nothing to do
}

void I_SetPalette(byte* palette) {
    // Convert 8-bit RGB palette from WAD to 32-bit T-OS colors
    for (int i = 0; i < 256; i++) {
        uint8_t r = palette[i * 3];
        uint8_t g = palette[i * 3 + 1];
        uint8_t b = palette[i * 3 + 2];
        doom_palette[i] = (r << 16) | (g << 8) | b;
    }
}

void I_FinishUpdate(void) {
    uint32_t* fb = (uint32_t*)global_boot_info.fb_base;
    uint32_t pitch = global_boot_info.fb_pitch;

    // Simple 1:1 blit to the top-left of the screen
    for (int y = 0; y < 200; y++) {
        for (int x = 0; x < 320; x++) {
            byte color_idx = screens[0][y * 320 + x];
            fb[y * pitch + x] = doom_palette[color_idx];
        }
    }
}

void I_ReadScreen(byte* scr) {
    // Unused in simple port
}

void I_UpdateNoBlit(void) {
    // Unused
}

void I_BeginUpdate(void) {
    // Unused
}
