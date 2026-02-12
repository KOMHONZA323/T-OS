#include "screen.h"
#include "ports.h"
#include "utils.h"

/* Global Screen Info */
ScreenInfo *screen_info = (ScreenInfo*)0x5000;
uint8_t *g_font = (uint8_t*)0xA000; // BIOS Font 8x16
uint32_t *g_framebuffer = 0;
/* Double Buffering: Back Buffer at 16MB mark */
static uint32_t *back_buffer = (uint32_t*)0x1000000;

int g_width = 0;
int g_height = 0;
int g_pitch = 0;
int g_bpp = 0;

/* Current cursor position */
int cursor_x = 0;
int cursor_y = 0;

/* Dynamic Screen Dimensions (Chars) */
int MAX_COLS = 80;
int MAX_ROWS = 25;

/* VGA Palette (Standard 16 colors) -> ARGB */
uint32_t vga_palette[16] = {
    0xFF000000, // 0: Black
    0xFF0000AA, // 1: Blue
    0xFF00AA00, // 2: Green
    0xFF00AAAA, // 3: Cyan
    0xFFAA0000, // 4: Red
    0xFFAA00AA, // 5: Magenta
    0xFFAA5500, // 6: Brown
    0xFFAAAAAA, // 7: Light Gray
    0xFF555555, // 8: Dark Gray
    0xFF5555FF, // 9: Light Blue
    0xFF55FF55, // 10: Light Green
    0xFF55FFFF, // 11: Light Cyan
    0xFFFF5555, // 12: Light Red
    0xFFFF55FF, // 13: Light Magenta
    0xFFFFFF55, // 14: Yellow
    0xFFFFFFFF  // 15: White
};

/* Forward Declarations */
void scroll_screen();
void draw_char(char c, int x, int y, uint32_t fg, uint32_t bg);

void init_screen() {
    g_width = screen_info->width;
    g_height = screen_info->height;
    g_bpp = screen_info->bpp;
    g_pitch = screen_info->pitch;
    g_framebuffer = (uint32_t*)screen_info->framebuffer;

    MAX_COLS = g_width / 8;
    MAX_ROWS = g_height / 16;

    // Reset cursor
    cursor_x = 0;
    cursor_y = 0;

    clear_screen();
}

void clear_screen() {
    // Fill Back Buffer with Black
    int size = g_width * g_height;
    // Assuming 32-bit aligned back buffer
    for (int i = 0; i < size; i++) {
        back_buffer[i] = 0xFF000000;
    }
    cursor_x = 0;
    cursor_y = 0;
}

void put_pixel(int x, int y, uint32_t color) {
    if (x < 0 || x >= g_width || y < 0 || y >= g_height) return;

    // Write to Back Buffer (Linear 32-bit)
    back_buffer[y * g_width + x] = color;
}

uint32_t get_pixel(int x, int y) {
    if (x < 0 || x >= g_width || y < 0 || y >= g_height) return 0;
    return back_buffer[y * g_width + x];
}

void swap_buffers() {
    // Copy Back Buffer to Front Buffer (VBE)
    // Handle VBE pitch (padding)
    int row_size_bytes = g_width * 4; // 32 bpp = 4 bytes

    if (g_pitch == row_size_bytes) {
         // Fast path
         memory_copy((char*)back_buffer, (char*)g_framebuffer, g_width * g_height * 4);
    } else {
         // Copy row by row
         for (int y = 0; y < g_height; y++) {
             char *src = (char*)back_buffer + (y * row_size_bytes);
             char *dst = (char*)g_framebuffer + (y * g_pitch);
             memory_copy(src, dst, row_size_bytes);
         }
    }
}

void draw_char(char c, int x, int y, uint32_t fg, uint32_t bg) {
    uint8_t *glyph = g_font + (unsigned char)c * 16;

    for (int row = 0; row < 16; row++) {
        uint8_t data = glyph[row];
        for (int col = 0; col < 8; col++) {
            // Check bit (MSB first)
            if ((data >> (7 - col)) & 1) {
                put_pixel(x + col, y + row, fg);
            } else {
                put_pixel(x + col, y + row, bg);
            }
        }
    }
}

void kprint_at_attr(char *message, int col, int row, char attr) {
    if (col >= 0) cursor_x = col;
    if (row >= 0) cursor_y = row;

    uint32_t fg = vga_palette[attr & 0x0F];
    uint32_t bg = vga_palette[(attr >> 4) & 0x0F];

    int i = 0;
    while (message[i] != 0) {
        char c = message[i++];

        if (c == '\n') {
            cursor_x = 0;
            cursor_y++;
        } else if (c == 0x08) { // Backspace
             if (cursor_x > 0) cursor_x--;
             draw_char(' ', cursor_x * 8, cursor_y * 16, fg, bg);
        } else {
            draw_char(c, cursor_x * 8, cursor_y * 16, fg, bg);
            cursor_x++;
        }

        // Wrap
        if (cursor_x >= MAX_COLS) {
            cursor_x = 0;
            cursor_y++;
        }

        // Scroll
        if (cursor_y >= MAX_ROWS) {
            scroll_screen();
            cursor_y = MAX_ROWS - 1;
        }
    }
}

void kprint_at(char *message, int col, int row) {
    kprint_at_attr(message, col, row, WHITE_ON_BLACK);
}

void kprint(char *message) {
    kprint_at_attr(message, -1, -1, WHITE_ON_BLACK);
}

void kprint_backspace() {
    if (cursor_x > 0) {
        cursor_x--;
        draw_char(' ', cursor_x * 8, cursor_y * 16, VGA_WHITE, VGA_BLACK);
    }
}

void scroll_screen() {
    // Scroll up 16 pixels (one row) on Back Buffer
    int bytes_per_line = g_width * 4; // Back buffer is packed
    int copy_size = (g_height - 16) * bytes_per_line;

    // Source: line 16
    // Dest: line 0
    uint32_t *src = back_buffer + (16 * g_width);
    uint32_t *dst = back_buffer;

    memory_copy((char*)src, (char*)dst, copy_size);

    // Clear last line (16 pixels high)
    uint32_t *last_line = back_buffer + ((g_height - 16) * g_width);
    memory_set((char*)last_line, 0, 16 * bytes_per_line);
}

/* TUI Functions */

void draw_rect(int col, int row, int width, int height, char attr) {
    // Clear chars in this rect with bg color
    uint32_t bg = vga_palette[(attr >> 4) & 0x0F];

    int x_start = col * 8;
    int y_start = row * 16;
    int w_px = width * 8;
    int h_px = height * 16;

    for (int y = 0; y < h_px; y++) {
        for (int x = 0; x < w_px; x++) {
            put_pixel(x_start + x, y_start + y, bg);
        }
    }
}

void draw_fill(int col, int row, int width, int height, char c, char attr) {
    uint32_t fg = vga_palette[attr & 0x0F];
    uint32_t bg = vga_palette[(attr >> 4) & 0x0F];

    for (int r = 0; r < height; r++) {
        for (int cw = 0; cw < width; cw++) {
            draw_char(c, (col + cw) * 8, (row + r) * 16, fg, bg);
        }
    }
}

void draw_box(int col, int row, int width, int height, char border_attr, char inner_attr) {
     uint32_t b_fg = vga_palette[border_attr & 0x0F];
     uint32_t b_bg = vga_palette[(border_attr >> 4) & 0x0F];

     // Fill inside
     draw_fill(col + 1, row + 1, width - 2, height - 2, ' ', inner_attr);

     // Borders
     for (int x = 0; x < width; x++) {
         draw_char(205, (col + x) * 8, row * 16, b_fg, b_bg);
         draw_char(205, (col + x) * 8, (row + height - 1) * 16, b_fg, b_bg);
     }
     for (int y = 0; y < height; y++) {
         draw_char(186, col * 8, (row + y) * 16, b_fg, b_bg);
         draw_char(186, (col + width - 1) * 8, (row + y) * 16, b_fg, b_bg);
     }
     // Corners
     draw_char(201, col * 8, row * 16, b_fg, b_bg);
     draw_char(187, (col + width - 1) * 8, row * 16, b_fg, b_bg);
     draw_char(200, col * 8, (row + height - 1) * 16, b_fg, b_bg);
     draw_char(188, (col + width - 1) * 8, (row + height - 1) * 16, b_fg, b_bg);
}

void draw_box_rounded(int col, int row, int width, int height, char border_attr, char inner_attr, char title_attr) {
    // Fill Inner
    draw_fill(col + 1, row + 1, width - 2, height - 2, ' ', inner_attr);

    uint32_t b_fg = vga_palette[border_attr & 0x0F];
    uint32_t b_bg = vga_palette[(border_attr >> 4) & 0x0F];

    // Borders (Single lines)
    for (int x = 1; x < width - 1; x++) {
         draw_char(196, (col + x) * 8, row * 16, b_fg, b_bg); // Top
         draw_char(196, (col + x) * 8, (row + height - 1) * 16, b_fg, b_bg); // Bottom
    }
    for (int y = 1; y < height - 1; y++) {
         draw_char(179, col * 8, (row + y) * 16, b_fg, b_bg); // Left
         draw_char(179, (col + width - 1) * 8, (row + y) * 16, b_fg, b_bg); // Right
    }

    // Corners (Single line rounded-ish)
    draw_char(218, col * 8, row * 16, b_fg, b_bg); // TL
    draw_char(191, (col + width - 1) * 8, row * 16, b_fg, b_bg); // TR
    draw_char(192, col * 8, (row + height - 1) * 16, b_fg, b_bg); // BL
    draw_char(217, (col + width - 1) * 8, (row + height - 1) * 16, b_fg, b_bg); // BR
}

/* Modern Graphics API */

// Helper for abs
static int abs(int v) { return v < 0 ? -v : v; }

void draw_rect_px(int x, int y, int w, int h, uint32_t color) {
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            put_pixel(x + j, y + i, color);
        }
    }
}

void draw_line(int x0, int y0, int x1, int y1, uint32_t color) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;

    for (;;) {
        put_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void draw_circle(int xc, int yc, int r, uint32_t color) {
    int x = 0, y = r;
    int d = 3 - 2 * r;
    while (y >= x) {
        put_pixel(xc + x, yc + y, color);
        put_pixel(xc - x, yc + y, color);
        put_pixel(xc + x, yc - y, color);
        put_pixel(xc - x, yc - y, color);
        put_pixel(xc + y, yc + x, color);
        put_pixel(xc - y, yc + x, color);
        put_pixel(xc + y, yc - x, color);
        put_pixel(xc - y, yc - x, color);
        x++;
        if (d > 0) {
            y--;
            d = d + 4 * (x - y) + 10;
        } else {
            d = d + 4 * x + 6;
        }
    }
}

void draw_fill_circle(int xc, int yc, int r, uint32_t color) {
    int x = 0, y = r;
    int d = 3 - 2 * r;
    while (y >= x) {
        // Draw horizontal lines
        for (int i = xc - x; i <= xc + x; i++) {
            put_pixel(i, yc + y, color);
            put_pixel(i, yc - y, color);
        }
        for (int i = xc - y; i <= xc + y; i++) {
            put_pixel(i, yc + x, color);
            put_pixel(i, yc - x, color);
        }
        x++;
        if (d > 0) {
            y--;
            d = d + 4 * (x - y) + 10;
        } else {
            d = d + 4 * x + 6;
        }
    }
}

void draw_rounded_rect(int x, int y, int w, int h, int r, uint32_t color) {
    // 1. Draw 4 Corner filled circles
    draw_fill_circle(x + r, y + r, r, color);
    draw_fill_circle(x + w - r - 1, y + r, r, color);
    draw_fill_circle(x + r, y + h - r - 1, r, color);
    draw_fill_circle(x + w - r - 1, y + h - r - 1, r, color);

    // 2. Draw Center Rects
    // Vertical Center Rect (between top and bottom circles)
    draw_rect_px(x + r, y, w - 2 * r, h, color);
    // Left Rect (between TL and BL)
    draw_rect_px(x, y + r, r, h - 2 * r, color);
    // Right Rect (between TR and BR)
    draw_rect_px(x + w - r, y + r, r, h - 2 * r, color);
}

void draw_rect_alpha(int x, int y, int w, int h, uint32_t color, uint8_t alpha) {
    uint32_t src_r = (color >> 16) & 0xFF;
    uint32_t src_g = (color >> 8) & 0xFF;
    uint32_t src_b = (color) & 0xFF;

    uint32_t a = alpha;
    uint32_t inv_a = 255 - a;

    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            int px = x + j;
            int py = y + i;
            if (px < 0 || px >= g_width || py < 0 || py >= g_height) continue;

            uint32_t dst = get_pixel(px, py);
            uint32_t dst_r = (dst >> 16) & 0xFF;
            uint32_t dst_g = (dst >> 8) & 0xFF;
            uint32_t dst_b = (dst) & 0xFF;

            // Blend
            uint32_t out_r = (src_r * a + dst_r * inv_a) / 255;
            uint32_t out_g = (src_g * a + dst_g * inv_a) / 255;
            uint32_t out_b = (src_b * a + dst_b * inv_a) / 255;

            uint32_t out_color = (0xFF << 24) | (out_r << 16) | (out_g << 8) | out_b;
            put_pixel(px, py, out_color);
        }
    }
}
