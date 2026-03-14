#ifndef _TSL_H_
#define _TSL_H_

#include "uefi.h"
#include "bootinfo.h"

// T-OS Screen Library (TSL)
// Provides drawing primitives for GUI and Apps

static inline void tsl_draw_pixel(BootInfo *bi, UINT32 *buffer, UINT32 x, UINT32 y, UINT32 color) {
    if (x >= bi->fb_width || y >= bi->fb_height) return;
    buffer[(uint64_t)y * (uint64_t)bi->fb_width + (uint64_t)x] = color;
}

static inline void tsl_fill_rect(BootInfo *bi, UINT32 *buffer, UINT32 x, UINT32 y, UINT32 w, UINT32 h, UINT32 color) {
    if (x >= bi->fb_width || y >= bi->fb_height) return;
    if (x + w > bi->fb_width) w = bi->fb_width - x;
    if (y + h > bi->fb_height) h = bi->fb_height - y;

    for (UINT32 j = 0; j < h; j++) {
        UINT32 *line = &buffer[(uint64_t)(y + j) * (uint64_t)bi->fb_width + x];
        for (UINT32 i = 0; i < w; i++) line[i] = color;
    }
}

// Custom Window Making
typedef struct {
    UINT32 x, y, w, h;
    UINT32 bg_color;
    UINT32 border_color;
    UINT32 titlebar_color;
    UINT32 text_color;
    const char *title;
} TslWindow;

static inline void tsl_draw_window(BootInfo *bi, UINT32 *buffer, TslWindow *win) {
    if (!bi || !buffer || !win) return;

    // Window Background
    tsl_fill_rect(bi, buffer, win->x, win->y, win->w, win->h, win->bg_color);
    
    // Top Border
    tsl_fill_rect(bi, buffer, win->x, win->y, win->w, 1, win->border_color);
    // Bottom Border
    tsl_fill_rect(bi, buffer, win->x, win->y + win->h - 1, win->w, 1, win->border_color);
    // Left Border
    tsl_fill_rect(bi, buffer, win->x, win->y, 1, win->h, win->border_color);
    // Right Border
    tsl_fill_rect(bi, buffer, win->x + win->w - 1, win->y, 1, win->h, win->border_color);

    // Titlebar Background
    tsl_fill_rect(bi, buffer, win->x + 1, win->y + 1, win->w - 2, 20, win->titlebar_color);
    
    // (Title text drawing would require font rendering integration here)
}

#endif
