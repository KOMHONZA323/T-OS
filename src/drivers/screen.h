#ifndef SCREEN_H
#define SCREEN_H

#include <stdint.h>

// Screen Info Struct (Must match Stage 2 layout at 0x5000)
typedef struct {
    uint16_t width;
    uint16_t height;
    uint8_t bpp;
    uint16_t pitch;
    uint32_t framebuffer;
    uint8_t requested_res;
} __attribute__((packed)) ScreenInfo;

// Colors (ARGB 8888)
#define COLOR_BLACK 0xFF000000
#define COLOR_WHITE 0xFFFFFFFF
#define COLOR_RED   0xFFFF0000
#define COLOR_GREEN 0xFF00FF00
#define COLOR_BLUE  0xFF0000FF
#define COLOR_CYAN  0xFF00FFFF
#define COLOR_MAGENTA 0xFFFF00FF
#define COLOR_YELLOW 0xFFFFFF00
#define COLOR_GRAY  0xFF808080
#define COLOR_LIGHT_GRAY 0xFFC0C0C0

// Legacy VGA Attribute Mapping
// We will map 4-bit VGA colors to 32-bit ARGB
#define VGA_BLACK 0
#define VGA_BLUE 1
#define VGA_GREEN 2
#define VGA_CYAN 3
#define VGA_RED 4
#define VGA_MAGENTA 5
#define VGA_BROWN 6
#define VGA_LIGHT_GRAY 7
#define VGA_DARK_GRAY 8
#define VGA_LIGHT_BLUE 9
#define VGA_LIGHT_GREEN 10
#define VGA_LIGHT_CYAN 11
#define VGA_LIGHT_RED 12
#define VGA_LIGHT_MAGENTA 13
#define VGA_YELLOW 14
#define VGA_WHITE 15

// Attributes (FG | BG << 4)
#define WHITE_ON_BLACK (VGA_WHITE | (VGA_BLACK << 4))
#define RED_ON_WHITE (VGA_RED | (VGA_WHITE << 4))

// Screen Size Constants (Dynamic now)
// Will be set in init_screen()
extern int MAX_COLS;
extern int MAX_ROWS;

// Global screen properties
extern int g_width;
extern int g_height;
extern int g_pitch;
extern int g_bpp;
extern uint32_t *g_framebuffer;
extern uint32_t *g_back_buffer;

/* Public kernel API */
void init_screen();
void swap_buffers();
void clear_screen();
void kprint_at(char *message, int col, int row);
void kprint_at_attr(char *message, int col, int row, char attr);
void kprint(char *message);
void kprint_backspace();

/* TUI Extension functions (Legacy, use new primitives for modern UI) */
void draw_rect(int col, int row, int width, int height, char attr);
void draw_fill(int col, int row, int width, int height, char c, char attr);
void draw_box(int col, int row, int width, int height, char border_attr, char inner_attr);
void draw_box_rounded(int col, int row, int width, int height, char border_attr, char inner_attr, char title_attr);

/* Low-level Graphics API */
void put_pixel(int x, int y, uint32_t color);
uint32_t get_pixel(int x, int y);
uint32_t vga_to_rgb(uint8_t attr);
void swap_buffers();

/* Modern Graphics API (Pixel based) */
void draw_rect_px(int x, int y, int w, int h, uint32_t color);
void draw_rect_alpha(int x, int y, int w, int h, uint32_t color, uint8_t alpha);
void draw_line(int x0, int y0, int x1, int y1, uint32_t color);
void draw_circle(int xc, int yc, int r, uint32_t color);
void draw_rounded_rect(int x, int y, int w, int h, int r, uint32_t color);
void draw_fill_circle(int xc, int yc, int r, uint32_t color); // Helper for rounded rect fill

#endif
