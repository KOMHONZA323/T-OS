#ifndef COMPOSITOR_H
#define COMPOSITOR_H

#include <stdint.h>
#include <stddef.h>

// Framebuffer Basics: UEFI GOP data struct
typedef struct {
    uint64_t fb_base;
    uint64_t fb_size;
    uint32_t fb_width;
    uint32_t fb_height;
    uint32_t fb_pitch;
} GOP_Info;

// Theme Modes
typedef enum {
    THEME_FEDORA_DARK,
    THEME_WINDOWS_AERO
} ThemeMode;

// Global UI State
typedef struct {
    ThemeMode current_theme;
    uint32_t term_x;
    uint32_t term_y;
    uint32_t term_w;
    uint32_t term_h;
    uint32_t cur_x;
    uint32_t cur_y;
} UIState;

void compositor_init(GOP_Info* info);
void compositor_put_pixel(uint32_t x, uint32_t y, uint32_t color);
void compositor_draw_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);
void compositor_draw_string(const char* str, uint32_t x, uint32_t y, uint32_t color);
void compositor_swap_buffers(void);
void compositor_scroll_terminal(void);
void compositor_print(const char* str, uint32_t color);
void compositor_draw_desktop(void);
void compositor_toggle_theme(void);
void compositor_handle_interrupt(uint8_t scancode);

#endif // COMPOSITOR_H
