#include "ports.h"
#include "screen.h"
#include "utils.h"

/* Global Screen Info */
ScreenInfo *screen_info = (ScreenInfo *)0x5000;
uint8_t *g_font = (uint8_t *)0xA000; // BIOS Font 8x16
uint32_t *g_framebuffer = 0;
uint32_t *g_back_buffer = 0;
int g_width = 0;
int g_height = 0;
int g_pitch = 0;
int g_bpp = 0;
int g_logical_pitch = 0;

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

  // Use the hardware pitch provided by Stage 2 (which reads VBE mode info correctly)
  g_pitch = screen_info->pitch;

  g_framebuffer = (uint32_t *)screen_info->framebuffer;

  // Use direct rendering into VRAM to avoid corruption from assuming a fixed
  // back-buffer address/size at high resolutions (e.g. 4K).
  g_back_buffer = g_framebuffer;
  g_logical_pitch = g_pitch;
  // Logical Pitch (Back Buffer is always packed 32-bit pixels: width * 4)
  g_logical_pitch = g_width * 4;

  // Allocate back buffer at 16MB mark
  g_back_buffer = (uint32_t *)0x1000000;

  MAX_COLS = g_width / 8;
  MAX_ROWS = g_height / 16;

  // Reset cursor
  cursor_x = 0;
  cursor_y = 0;

  clear_screen();
}

void clear_screen() {
  // Fill with Black
  // Use logical pitch for back buffer
  int size_bytes = g_height * g_logical_pitch;
  memory_set((char *)g_back_buffer, 0, size_bytes);

  cursor_x = 0;
  cursor_y = 0;
}

void put_pixel(int x, int y, uint32_t color) {
  if (x < 0 || x >= g_width || y < 0 || y >= g_height)
    return;

  // Draw to Back Buffer using LOGICAL pitch (Packed)
  uint8_t *pixel_addr =
      (uint8_t *)g_back_buffer + (y * g_logical_pitch) + (x * 4);
  *(uint32_t *)pixel_addr = color;
}

void wait_vsync() {
  while (port_byte_in(0x3DA) & 8)
    ;
  while (!(port_byte_in(0x3DA) & 8))
    ;
}

void swap_buffers() {
  // No-op when drawing directly into VRAM.
  if (g_back_buffer == g_framebuffer)
    return;

    // Copy back buffer to front buffer row by row
    // handling potential pitch mismatch (padding)

    for (int y = 0; y < g_height; y++) {
        // Source: Back buffer (Packed)
        // Dest: Front buffer (Hardware Pitch)
  wait_vsync();

  // Copy back buffer to front buffer row by row
  for (int y = 0; y < g_height; y++) {
    // Source: Back buffer (Packed using g_logical_pitch)
    char *src = (char *)g_back_buffer + (y * g_logical_pitch);
    // Dest: Front buffer (Hardware Pitch)
    char *dst = (char *)g_framebuffer + (y * g_pitch);

    memory_copy(src, dst, g_width * 4);
  }
}

void draw_char(char c, int x, int y, uint32_t fg, uint32_t bg) {
  uint8_t *glyph = g_font + (unsigned char)c * 16;

  for (int row = 0; row < 16; row++) {
    uint8_t data = glyph[row];
    for (int col = 0; col < 8; col++) {
      if ((data >> (7 - col)) & 1) {
        put_pixel(x + col, y + row, fg);
      } else {
        put_pixel(x + col, y + row, bg);
      }
    }
  }
}

void kprint_at_attr(char *message, int col, int row, char attr) {
  if (col >= 0)
    cursor_x = col;
  if (row >= 0)
    cursor_y = row;

  uint32_t fg = vga_palette[attr & 0x0F];
  uint32_t bg = vga_palette[(attr >> 4) & 0x0F];

  int i = 0;
  while (message[i] != 0) {
    char c = message[i++];

    if (c == '\n') {
      cursor_x = 0;
      cursor_y++;
    } else if (c == 0x08) { // Backspace
      if (cursor_x > 0)
        cursor_x--;
      draw_char(' ', cursor_x * 8, cursor_y * 16, fg, bg);
    } else {
      draw_char(c, cursor_x * 8, cursor_y * 16, fg, bg);
      cursor_x++;
    }

    if (cursor_x >= MAX_COLS) {
      cursor_x = 0;
      cursor_y++;
    }

    if (cursor_y >= MAX_ROWS) {
      scroll_screen();
      cursor_y = MAX_ROWS - 1;
    }
  }
}

void kprint_at(char *message, int col, int row) {
  kprint_at_attr(message, col, row, WHITE_ON_BLACK);
}

void kprint(char *message) { kprint_at_attr(message, -1, -1, WHITE_ON_BLACK); }

void kprint_backspace() {
  if (cursor_x > 0) {
    cursor_x--;
    draw_char(' ', cursor_x * 8, cursor_y * 16, VGA_WHITE, VGA_BLACK);
  }
}

void scroll_screen() {
  int bytes_per_line = g_logical_pitch;
  int copy_rows = g_height - 16;
  int copy_size = copy_rows * bytes_per_line;

  uint8_t *src = (uint8_t *)g_back_buffer + (16 * bytes_per_line);
  uint8_t *dst = (uint8_t *)g_back_buffer;

  memory_copy((char *)src, (char *)dst, copy_size);

  uint8_t *last_line = (uint8_t *)g_back_buffer + (copy_rows * bytes_per_line);
  memory_set((char *)last_line, 0, 16 * bytes_per_line);
}

/* TUI Functions */

void draw_rect(int col, int row, int width, int height, char attr) {
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

void draw_box(int col, int row, int width, int height, char border_attr,
              char inner_attr) {
  uint32_t b_fg = vga_palette[border_attr & 0x0F];
  uint32_t b_bg = vga_palette[(border_attr >> 4) & 0x0F];

  draw_fill(col + 1, row + 1, width - 2, height - 2, ' ', inner_attr);

  for (int x = 0; x < width; x++) {
    draw_char(205, (col + x) * 8, row * 16, b_fg, b_bg);
    draw_char(205, (col + x) * 8, (row + height - 1) * 16, b_fg, b_bg);
  }
  for (int y = 0; y < height; y++) {
    draw_char(186, col * 8, (row + y) * 16, b_fg, b_bg);
    draw_char(186, (col + width - 1) * 8, (row + y) * 16, b_fg, b_bg);
  }
  draw_char(201, col * 8, row * 16, b_fg, b_bg);
  draw_char(187, (col + width - 1) * 8, row * 16, b_fg, b_bg);
  draw_char(200, col * 8, (row + height - 1) * 16, b_fg, b_bg);
  draw_char(188, (col + width - 1) * 8, (row + height - 1) * 16, b_fg, b_bg);
}

void draw_box_rounded(int col, int row, int width, int height, char border_attr,
                      char inner_attr, char title_attr) {
  draw_fill(col + 1, row + 1, width - 2, height - 2, ' ', inner_attr);

  uint32_t b_fg = vga_palette[border_attr & 0x0F];
  uint32_t b_bg = vga_palette[(border_attr >> 4) & 0x0F];

  for (int x = 1; x < width - 1; x++) {
    draw_char(196, (col + x) * 8, row * 16, b_fg, b_bg);
    draw_char(196, (col + x) * 8, (row + height - 1) * 16, b_fg, b_bg);
  }
  for (int y = 1; y < height - 1; y++) {
    draw_char(179, col * 8, (row + y) * 16, b_fg, b_bg);
    draw_char(179, (col + width - 1) * 8, (row + y) * 16, b_fg, b_bg);
  }

  draw_char(218, col * 8, row * 16, b_fg, b_bg);
  draw_char(191, (col + width - 1) * 8, row * 16, b_fg, b_bg);
  draw_char(192, col * 8, (row + height - 1) * 16, b_fg, b_bg);
  draw_char(217, (col + width - 1) * 8, (row + height - 1) * 16, b_fg, b_bg);
}

uint32_t vga_to_rgb(uint8_t attr) { return vga_palette[attr & 0x0F]; }
