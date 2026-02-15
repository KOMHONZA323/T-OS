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

  g_framebuffer = (uint32_t *)screen_info->framebuffer;
  g_pitch = screen_info->pitch;

  if (g_bpp == 0) {
      // Text Mode (Fallback)
      g_back_buffer = g_framebuffer;
      g_logical_pitch = 160;
      MAX_COLS = 80;
      MAX_ROWS = 25;
  } else if (g_bpp == 4) {
      // VGA 640x480 Planar
      g_width = 640;
      g_height = 480;
      // Use RAM back buffer for double buffering
      // We will store it as 1 byte per pixel (Linear 8-bit) in RAM
      g_logical_pitch = 640;
      g_back_buffer = (uint32_t *)0x1000000; // Reuse the 16MB buffer location
      // g_framebuffer remains 0xA0000 (VRAM)
      g_framebuffer = (uint32_t*)0xA0000;

      MAX_COLS = 80;
      MAX_ROWS = 30; // 480 / 16
  } else {
      // VBE Graphics Mode
      g_back_buffer = g_framebuffer;
      g_logical_pitch = g_width * 4;
      g_back_buffer = (uint32_t *)0x1000000;
      MAX_COLS = g_width / 8;
      MAX_ROWS = g_height / 16;
  }

  // Reset cursor
  cursor_x = 0;
  cursor_y = 0;

  clear_screen();
}

void clear_screen() {
  if (g_bpp == 0) {
      // Text Mode Clear
      char* vidmem = (char*)g_framebuffer;
      for (int i = 0; i < MAX_COLS * MAX_ROWS * 2; i+=2) {
          vidmem[i] = ' ';
          vidmem[i+1] = 0x07;
      }
  } else if (g_bpp == 4) {
      // VGA Double Buffered Clear
      // Just clear the RAM buffer
      memory_set((char*)g_back_buffer, 0, 640 * 480);
  } else {
      // VBE Clear
      int size_bytes = g_height * g_logical_pitch;
      memory_set((char *)g_back_buffer, 0, size_bytes);
  }

  cursor_x = 0;
  cursor_y = 0;
}

// Convert ARGB to closest VGA index (0-15)
uint8_t rgb_to_vga(uint32_t color) {
    // 1. Explicit Matches for T-OS Theme
    if ((color & 0xFFFFFF) == 0x121212) return 0; // Black
    if ((color & 0xFFFFFF) == 0x1E1E1E) return 8; // Dark Gray
    if ((color & 0xFFFFFF) == 0x00E5FF) return 11; // Cyan

    // 2. Check exact matches
    for (int i = 0; i < 16; i++) {
        if (vga_palette[i] == color) return i;
    }

    // Check alpha
    if (((color >> 24) & 0xFF) == 0) return 0;

    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;

    // 3. Threshold Mapping
    if (r < 32 && g < 32 && b < 32) return 0;

    if (r > 128 && g > 128 && b > 128) return 15; // White
    if (r > 128 && g > 128) return 14; // Yellow
    if (r > 128 && b > 128) return 13; // Magenta
    if (g > 128 && b > 128) return 11; // Cyan
    if (r > 128) return 12; // Light Red
    if (g > 128) return 10; // Light Green
    if (b > 128) return 9; // Light Blue

    // Mid-tones
    if (r > 64 && g > 64 && b > 64) return 7; // Light Gray
    return 8; // Dark Gray
}

void put_pixel(int x, int y, uint32_t color) {
  if (g_bpp == 0) return;

  if (x < 0 || x >= g_width || y < 0 || y >= g_height)
    return;

  if (g_bpp == 4) {
      // VGA: Write index to RAM back buffer (1 byte per pixel)
      uint8_t idx = rgb_to_vga(color);
      uint8_t* buf = (uint8_t*)g_back_buffer;
      buf[y * 640 + x] = idx;
      return;
  }

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
  // If text mode, nothing to swap
  if (g_bpp == 0) return;

  // If VBE single buffering (direct), nothing to swap
  if (g_back_buffer == g_framebuffer && g_bpp != 4) return;

  // wait_vsync(); // Removed to fix QEMU lag. rely on PIT limiter.

  if (g_bpp == 4) {
      // VGA Chunky-to-Planar Conversion
      // g_back_buffer holds 640x480 bytes (indices 0-15)
      // g_framebuffer is 0xA0000 (VRAM)
      // We must write 4 planes.

      // Ensure Write Mode 0, Bit Mask 0xFF
      port_byte_out(0x3CE, 0x05); port_byte_out(0x3CF, 0x00);
      port_byte_out(0x3CE, 0x08); port_byte_out(0x3CF, 0xFF);

      uint8_t* ram_buf = (uint8_t*)g_back_buffer;
      volatile uint8_t* vram = (volatile uint8_t*)0xA0000;

      // For each plane
      for (int plane = 0; plane < 4; plane++) {
          // Select Plane via Map Mask (Sequencer Index 2) -> wait, Map Mask is 0x3C4 index 2.
          port_byte_out(0x3C4, 0x02);
          port_byte_out(0x3C5, 1 << plane);

          // Loop through all bytes in the plane (38400 bytes)
          for (int i = 0; i < 38400; i++) {
              uint8_t planar_byte = 0;
              // Construct 1 byte from 8 chunky pixels
              int pixel_base = i * 8;

              // Unroll loop for speed
              // Bit 7 (Pixel 0)
              if (ram_buf[pixel_base + 0] & (1 << plane)) planar_byte |= 0x80;
              // Bit 6 (Pixel 1)
              if (ram_buf[pixel_base + 1] & (1 << plane)) planar_byte |= 0x40;
              // Bit 5 (Pixel 2)
              if (ram_buf[pixel_base + 2] & (1 << plane)) planar_byte |= 0x20;
              // Bit 4 (Pixel 3)
              if (ram_buf[pixel_base + 3] & (1 << plane)) planar_byte |= 0x10;
              // Bit 3 (Pixel 4)
              if (ram_buf[pixel_base + 4] & (1 << plane)) planar_byte |= 0x08;
              // Bit 2 (Pixel 5)
              if (ram_buf[pixel_base + 5] & (1 << plane)) planar_byte |= 0x04;
              // Bit 1 (Pixel 6)
              if (ram_buf[pixel_base + 6] & (1 << plane)) planar_byte |= 0x02;
              // Bit 0 (Pixel 7)
              if (ram_buf[pixel_base + 7] & (1 << plane)) planar_byte |= 0x01;

              vram[i] = planar_byte;
          }
      }
      return;
  }

  // VBE Swap
  for (int y = 0; y < g_height; y++) {
    char *src = (char *)g_back_buffer + (y * g_logical_pitch);
    char *dst = (char *)g_framebuffer + (y * g_pitch);
    memory_copy(src, dst, g_width * 4);
  }
}

void draw_char(char c, int x, int y, uint32_t fg, uint32_t bg) {
  if (g_bpp == 0) {
      int col = x / 8;
      int row = y / 16;
      if (col >= MAX_COLS || row >= MAX_ROWS) return;
      char* vidmem = (char*)g_framebuffer + (row * MAX_COLS + col) * 2;
      vidmem[0] = c;
      vidmem[1] = 0x0F;
      return;
  }

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

      if (g_bpp == 0) {
          char* vidmem = (char*)g_framebuffer + (cursor_y * MAX_COLS + cursor_x) * 2;
          vidmem[0] = ' ';
          vidmem[1] = attr;
      } else {
          draw_char(' ', cursor_x * 8, cursor_y * 16, fg, bg);
      }
    } else {
      if (g_bpp == 0) {
          char* vidmem = (char*)g_framebuffer + (cursor_y * MAX_COLS + cursor_x) * 2;
          vidmem[0] = c;
          vidmem[1] = attr;
      } else {
          draw_char(c, cursor_x * 8, cursor_y * 16, fg, bg);
      }
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
  kprint_at_attr(message, col, row, 0x0F); // White on Black
}

void kprint(char *message) { kprint_at_attr(message, -1, -1, 0x0F); }

void kprint_backspace() {
  if (cursor_x > 0) {
    cursor_x--;
    if (g_bpp == 0) {
         char* vidmem = (char*)g_framebuffer + (cursor_y * MAX_COLS + cursor_x) * 2;
         vidmem[0] = ' ';
         vidmem[1] = 0x0F;
    } else {
        draw_char(' ', cursor_x * 8, cursor_y * 16, VGA_WHITE, VGA_BLACK);
    }
  }
}

void scroll_screen() {
  if (g_bpp == 0) {
      char* vidmem = (char*)g_framebuffer;
      for (int i = 0; i < (MAX_ROWS - 1) * MAX_COLS * 2; i++) {
          vidmem[i] = vidmem[i + MAX_COLS * 2];
      }
      for (int i = (MAX_ROWS - 1) * MAX_COLS * 2; i < MAX_ROWS * MAX_COLS * 2; i+=2) {
          vidmem[i] = ' ';
          vidmem[i+1] = 0x0F;
      }
      return;
  }

  if (g_bpp == 4) {
      // VGA Back Buffer Scroll (Easy now!)
      // Just copy RAM buffer
      uint8_t* buf = (uint8_t*)g_back_buffer;
      int line_width = 640;
      int copy_bytes = line_width * (480 - 16);

      memory_copy((char*)(buf + 16 * 640), (char*)buf, copy_bytes);
      memory_set((char*)(buf + copy_bytes), 0, 16 * 640);
      return;
  }

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
  if (g_bpp == 0) {
      for (int r=0; r<height; r++) {
          for (int c=0; c<width; c++) {
              int idx = ((row + r) * MAX_COLS + (col + c)) * 2;
              char* vidmem = (char*)g_framebuffer;
              vidmem[idx+1] = attr;
          }
      }
      return;
  }

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
  if (g_bpp == 0) {
      for (int r=0; r<height; r++) {
          for (int cw=0; cw<width; cw++) {
               int idx = ((row + r) * MAX_COLS + (col + cw)) * 2;
               char* vidmem = (char*)g_framebuffer;
               vidmem[idx] = c;
               vidmem[idx+1] = attr;
          }
      }
      return;
  }

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
  uint32_t b_fg = vga_palette[border_attr & 0x0F];
  uint32_t b_bg = vga_palette[(border_attr >> 4) & 0x0F];

  draw_fill(col + 1, row + 1, width - 2, height - 2, ' ', inner_attr);

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

/* Advanced Graphics Primitives */

static inline int abs(int x) { return x < 0 ? -x : x; }

void put_pixel_alpha(int x, int y, uint32_t color) {
    if (g_bpp == 0) {
        // Fallback for Text Mode mouse drawing
        // We can't really alpha blend, but we can set the attribute
        // to inverted or something?
        // Or just map to closest char cell?
        // kernel_main calls draw_rect_px for mouse.
        // Let's implement a hacky rect fill for text mode here if needed,
        // but wait, draw_rect_px calls this.
        // For text mode, we should really use hardware cursor or character replacement.
        // But since this function is per-pixel, it's hard.
        // We'll leave it as return for now, assuming user is in VBE mode.
        return;
    }

    // In VGA mode, we don't read back easily for alpha blending
    // Fallback: just draw opaque pixel for now or use simplified logic
    if (g_bpp == 4) {
        if (((color >> 24) & 0xFF) > 128) { // 50% threshold
            put_pixel(x, y, color);
        }
        return;
    }

    if (x < 0 || x >= g_width || y < 0 || y >= g_height) return;

    uint8_t a = (color >> 24) & 0xFF;
    if (a == 0) return;
    if (a == 255) {
        put_pixel(x, y, color);
        return;
    }

    uint8_t *pixel_addr = (uint8_t *)g_back_buffer + (y * g_logical_pitch) + (x * 4);
    uint32_t bg = *(uint32_t *)pixel_addr;

    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;

    uint8_t bg_r = (bg >> 16) & 0xFF;
    uint8_t bg_g = (bg >> 8) & 0xFF;
    uint8_t bg_b = bg & 0xFF;

    uint8_t out_r = (r * a + bg_r * (255 - a)) / 255;
    uint8_t out_g = (g * a + bg_g * (255 - a)) / 255;
    uint8_t out_b = (b * a + bg_b * (255 - a)) / 255;

    *(uint32_t *)pixel_addr = (0xFF << 24) | (out_r << 16) | (out_g << 8) | out_b;
}

void draw_rect_px(int x, int y, int width, int height, uint32_t color) {
    if (g_bpp == 0) {
        // Basic support for Text Mode Mouse Cursor
        int col_start = x / 8;
        int row_start = y / 16;
        int col_end = (x + width) / 8;
        int row_end = (y + height) / 16;

        for (int r = row_start; r <= row_end; r++) {
            for (int c = col_start; c <= col_end; c++) {
                if (c >= MAX_COLS || r >= MAX_ROWS) continue;
                char* vidmem = (char*)g_framebuffer + (r * MAX_COLS + c) * 2;
                vidmem[1] = (vidmem[1] ^ 0x77);
            }
        }
        return;
    }

    if (x < 0) { width += x; x = 0; }
    if (y < 0) { height += y; y = 0; }
    if (x + width > g_width) width = g_width - x;
    if (y + height > g_height) height = g_height - y;
    if (width <= 0 || height <= 0) return;

    // Check for alpha in color
    uint8_t a = (color >> 24) & 0xFF;
    if (a < 255) {
        for (int r = 0; r < height; r++) {
            for (int c = 0; c < width; c++) {
                put_pixel_alpha(x + c, y + r, color);
            }
        }
        return;
    }

    if (g_bpp == 4) {
        uint8_t idx = rgb_to_vga(color);
        uint8_t* buf = (uint8_t*)g_back_buffer;
        for (int r = 0; r < height; r++) {
            memory_set((char*)(buf + (y + r) * 640 + x), idx, width);
        }
        return;
    }

    // Optimization: Direct Memory Fill for Opaque Colors (VBE)
    // Uses rep stosd implicitly via loop or optimized compiler logic
    // Removing the inner loop function call is key.
    // 32-bit color fill
    for (int r = 0; r < height; r++) {
        uint32_t* line = (uint32_t*)((uint8_t*)g_back_buffer + (y + r) * g_logical_pitch);
        uint32_t* start = line + x;
        // Manual unroll or let compiler optimize
        for (int c = 0; c < width; c++) {
            start[c] = color;
        }
    }
}

void draw_rect_alpha(int x, int y, int width, int height, uint32_t color) {
    draw_rect_px(x, y, width, height, color);
}

void draw_line(int x0, int y0, int x1, int y1, uint32_t color) {
    if (g_bpp == 0) return; // Not supported in text mode

    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;

    while (1) {
        put_pixel_alpha(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

// Helper for rounded rect corners (filled quadrant)
static void draw_circle_quadrant_fill(int x0, int y0, int radius, int quadrant, uint32_t color) {
    int x = radius;
    int y = 0;
    int err = 0;

    while (x >= y) {
        if (quadrant == 0) {
            draw_line(x0, y0 + y, x0 + x, y0 + y, color);
            draw_line(x0, y0 + x, x0 + y, y0 + x, color);
        } else if (quadrant == 1) {
            draw_line(x0 - x, y0 + y, x0, y0 + y, color);
            draw_line(x0 - y, y0 + x, x0, y0 + x, color);
        } else if (quadrant == 2) {
            draw_line(x0 - x, y0 - y, x0, y0 - y, color);
            draw_line(x0 - y, y0 - x, x0, y0 - x, color);
        } else if (quadrant == 3) {
            draw_line(x0, y0 - y, x0 + x, y0 - y, color);
            draw_line(x0, y0 - x, x0 + y, y0 - x, color);
        }

        if (err <= 0) {
            y += 1;
            err += 2 * y + 1;
        }
        if (err > 0) {
            x -= 1;
            err -= 2 * x + 1;
        }
    }
}

void draw_circle(int x0, int y0, int radius, uint32_t color, uint8_t fill) {
    if (g_bpp == 0) return; // Not supported in text mode

    int x = radius;
    int y = 0;
    int err = 0;

    while (x >= y) {
        if (fill) {
            draw_line(x0 - x, y0 + y, x0 + x, y0 + y, color);
            draw_line(x0 - x, y0 - y, x0 + x, y0 - y, color);
            draw_line(x0 - y, y0 + x, x0 + y, y0 + x, color);
            draw_line(x0 - y, y0 - x, x0 + y, y0 - x, color);
        } else {
            put_pixel_alpha(x0 + x, y0 + y, color);
            put_pixel_alpha(x0 + y, y0 + x, color);
            put_pixel_alpha(x0 - y, y0 + x, color);
            put_pixel_alpha(x0 - x, y0 + y, color);
            put_pixel_alpha(x0 - x, y0 - y, color);
            put_pixel_alpha(x0 - y, y0 - x, color);
            put_pixel_alpha(x0 + y, y0 - x, color);
            put_pixel_alpha(x0 + x, y0 - y, color);
        }

        if (err <= 0) {
            y += 1;
            err += 2 * y + 1;
        }
        if (err > 0) {
            x -= 1;
            err -= 2 * x + 1;
        }
    }
}

void draw_rounded_rect(int x, int y, int width, int height, int radius, uint32_t color) {
    if (g_bpp == 0) return; // Not supported in text mode

    // Center rects
    draw_rect_px(x + radius, y, width - 2 * radius, height, color);
    draw_rect_px(x, y + radius, radius, height - 2 * radius, color);
    draw_rect_px(x + width - radius, y + radius, radius, height - 2 * radius, color);

    // Corners
    draw_circle_quadrant_fill(x + width - radius - 1, y + height - radius - 1, radius, 0, color);
    draw_circle_quadrant_fill(x + radius, y + height - radius - 1, radius, 1, color);
    draw_circle_quadrant_fill(x + radius, y + radius, radius, 2, color);
    draw_circle_quadrant_fill(x + width - radius - 1, y + radius, radius, 3, color);
}

void draw_string_px(const char* str, int x, int y, uint32_t color) {
    if (g_bpp == 0) {
        int col = x / 8;
        int row = y / 16;
        kprint_at((char*)str, col, row);
        return;
    }

    int cur_x = x;
    int cur_y = y;
    while (*str) {
        if (*str == '\n') {
            cur_x = x;
            cur_y += 16;
        } else {
            uint8_t *glyph = g_font + (unsigned char)*str * 16;
            for (int row = 0; row < 16; row++) {
                uint8_t data = glyph[row];
                for (int col = 0; col < 8; col++) {
                    if ((data >> (7 - col)) & 1) {
                        put_pixel_alpha(cur_x + col, cur_y + row, color);
                    }
                }
            }
            cur_x += 8;
        }
        str++;
    }
}
