#include "screen.h"
#include "ports.h"
#include "utils.h"

/* Global Screen Info */
ScreenInfo *screen_info = (ScreenInfo*)0x5000;
uint8_t *g_font = (uint8_t*)0xA000; // BIOS Font 8x16
uint32_t *g_framebuffer = 0;
uint32_t *g_back_buffer = (uint32_t*)0x1000000; // 16MB Mark
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
    // Fill with Black
    // Handle pitch correctly (stride might be larger than width * 4)
    uint8_t *row_ptr = (uint8_t*)g_back_buffer;

    for (int y = 0; y < g_height; y++) {
        uint32_t *pixel = (uint32_t*)row_ptr;
        for (int x = 0; x < g_width; x++) {
            pixel[x] = 0xFF000000;
        }
        row_ptr += g_pitch;
    }
    cursor_x = 0;
    cursor_y = 0;
}

void swap_buffers() {
    // Copy back buffer to front buffer
    // Optimize: 32-bit copy
    int size_dwords = (g_height * g_pitch) / 4;

    // We use a simple loop as we don't have memcpy optimized for 32-bit
    // Also, g_framebuffer is in VRAM, writing might be slower, but that's expected.
    for (int i = 0; i < size_dwords; i++) {
        g_framebuffer[i] = g_back_buffer[i];
    }
}

void put_pixel(int x, int y, uint32_t color) {
    if (x < 0 || x >= g_width || y < 0 || y >= g_height) return;
    // Calculate offset: y * pitch + x * (bpp/8)
    // Assuming 32 bpp
    // g_pitch is in bytes. g_framebuffer is uint32_t*.
    // We need byte offset then cast.

    // Optimization: Since g_framebuffer is uint32_t*, we can index by pixels ONLY IF pitch == width * 4.
    // VBE pitch might include padding. So always use pitch.

    uint8_t *pixel_addr = (uint8_t*)g_back_buffer + (y * g_pitch) + (x * 4);
    *(uint32_t*)pixel_addr = color;
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
    // Scroll up 16 pixels (one row)
    // Copy (Height - 16) lines from y=16 to y=0
    int bytes_per_line = g_pitch;
    int copy_size = (g_height - 16) * bytes_per_line;

    // Source: line 16 (offset 16 * pitch)
    // Dest: line 0
    uint8_t *src = (uint8_t*)g_back_buffer + (16 * bytes_per_line);
    uint8_t *dst = (uint8_t*)g_back_buffer;

    memory_copy((char*)src, (char*)dst, copy_size);

    // Clear last line (16 pixels high)
    // Offset: (Height - 16) * pitch
    uint8_t *last_line = (uint8_t*)g_back_buffer + ((g_height - 16) * bytes_per_line);
    // Fill with black
    // Using memory_set, but for 32-bit color 0 is transparent/black?
    // ARGB 0x00000000 is transparent black?
    // ARGB 0xFF000000 is opaque black.
    // If I use 0, it might be fine.
    // For TUI, let's assume 0 is black.
    memory_set((char*)last_line, 0, 16 * bytes_per_line);
}

/* TUI Functions */

void draw_rect(int col, int row, int width, int height, char attr) {
    // Clear chars in this rect with bg color
    uint32_t bg = vga_palette[(attr >> 4) & 0x0F];
    // We only fill background?
    // Actually draw_rect usually clears the area.

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
     // Naive implementation using chars
     // Top/Bottom
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

    // Title bar area?
    // Already handled by caller filling title text.
}

uint32_t vga_to_rgb(uint8_t attr) {
    return vga_palette[attr & 0x0F];
}
