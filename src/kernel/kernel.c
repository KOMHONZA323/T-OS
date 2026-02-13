#include "ata.h"
#include "keyboard.h"
#include "ports.h"
#include "screen.h"
#include "theme.h"
#include "utils.h"

void draw_interface();
void draw_wallpaper();
void draw_desktop_icons();
void draw_top_bar();
void draw_bottom_bar();
void draw_window(int x, int y, int w, int h, const char *title,
                 const char *content);
void draw_loading_screen();
void open_settings();
void draw_string_px(int x, int y, const char* str, uint32_t fg);

// External from screen.c
extern void draw_char(char c, int x, int y, uint32_t fg, uint32_t bg);
extern int g_width, g_height;
extern uint8_t *g_font;

// Global state
int show_settings = 0;

void kernel_main(void) {
  // 1. Initialize Screen (VBE)
  init_screen();

  // 2. Loading Screen
  draw_loading_screen();
  clear_screen();

  // 3. Main Loop
  while (1) {
    // Handle Input
    char c = get_char();
    if (c == 's') {
      show_settings = !show_settings;
      clear_screen(); // Clear to redraw background
    }

    // Logic for Settings
    if (show_settings) {
      // Check for selection
      if (c >= '0' && c <= '5') {
        uint8_t config[512] = {0};
        config[0] = 0xAB; // Magic

        // 0=Auto, 1=720p, 2=1080p, 3=1440p, 4=1920p, 5=4K
        config[1] = c - '0';

        ata_write_sector(2879, config);
        // Reboot via Keyboard Controller
        port_byte_out(0x64, 0xFE);
      }
    }

    // Draw
    draw_interface();

    if (show_settings) {
      open_settings();
    }

    swap_buffers();

    // Delay to prevent CPU hogging
    delay(1000);
  }
}

void draw_interface() {

  // 1. Layer 0: Wallpaper
  draw_wallpaper();

  // 2. Layer 1: Desktop Icons
  draw_desktop_icons();

  // 3. Layer 2: Taskbars (Always on top of wallpaper)
  draw_top_bar();
  draw_bottom_bar();

  // 4. Layer 3: Windows (On top of taskbars)
  if (!show_settings) {
    // C Code Editor (Left)
    int win1_x = 50;
    int win1_y = 50;
    int win1_w = 400;
    int win1_h = 300;

    draw_window(win1_x, win1_y, win1_w, win1_h, "kernel.c - Visual Studio Code", "");

    // Draw Syntax Highlighted Content manually
    int sx = win1_x + 12;
    int sy = win1_y + 32;
    int lh = 20; // Line height

    draw_string_px(sx, sy, "#include", COLOR_MAGENTA);
    draw_string_px(sx + 72, sy, "<kernel.h>", COLOR_GREEN);

    draw_string_px(sx, sy + lh, "void", COLOR_NEON_BLUE);
    draw_string_px(sx + 40, sy + lh, "kernel_main", COLOR_YELLOW);
    draw_string_px(sx + 136, sy + lh, "(void) {", COLOR_WHITE);

    draw_string_px(sx + 16, sy + 2*lh, "// Initialize Video", 0xFF808080);
    draw_string_px(sx + 16, sy + 3*lh, "init_video();", COLOR_WHITE);
    draw_string_px(sx + 16, sy + 3*lh, "init_video", COLOR_YELLOW); // Highlight

    draw_string_px(sx + 16, sy + 5*lh, "while", COLOR_MAGENTA);
    draw_string_px(sx + 60, sy + 5*lh, "(1) {", COLOR_WHITE);
    draw_string_px(sx + 32, sy + 6*lh, "draw_interface();", COLOR_WHITE);
    draw_string_px(sx + 32, sy + 6*lh, "draw_interface", COLOR_YELLOW);
    draw_string_px(sx + 16, sy + 7*lh, "}", COLOR_WHITE);
    draw_string_px(sx, sy + 8*lh, "}", COLOR_WHITE);

    // Terminal (Right, slightly overlapping)
    int win2_x = 350;
    int win2_y = 200;
    int win2_w = 400;
    int win2_h = 250;

    draw_window(win2_x, win2_y, win2_w, win2_h, "Terminal", "");

    sx = win2_x + 12;
    sy = win2_y + 32;

    draw_string_px(sx, sy, "user@T-OS:~/src $", COLOR_NEON_BLUE);
    draw_string_px(sx + 144, sy, "make", COLOR_WHITE);

    draw_string_px(sx, sy + lh, "[ASM] Compiling boot.asm...", COLOR_WHITE);
    draw_string_px(sx, sy + 2*lh, "[CC]  Compiling kernel.c...", COLOR_WHITE);
    draw_string_px(sx, sy + 3*lh, "[LD]  Linking kernel.bin...", COLOR_WHITE);
    draw_string_px(sx, sy + 4*lh, "[OK]  Build Successful.", COLOR_GREEN);

    draw_string_px(sx, sy + 6*lh, "user@T-OS:~/src $", COLOR_NEON_BLUE);
    draw_string_px(sx + 144, sy + 6*lh, "_", COLOR_WHITE);
  }
}

void draw_wallpaper() {
  // 1. Solid Obsidian Background
  draw_rect_px(0, 0, g_width, g_height, COLOR_OBSIDIAN);

  // 2. Abstract Geometric Lines (Charcoal)
  for (int i = 0; i < g_width; i += 120) {
      draw_line(i, 0, i + 400, g_height, COLOR_CHARCOAL);
  }

  // Cross pattern
  for (int i = 0; i < g_width; i += 180) {
      draw_line(i, g_height, i + 400, 0, COLOR_CHARCOAL);
  }

  // 3. Neon Blue Accents (Subtle)
  draw_line(0, 0, g_width, g_height, COLOR_NEON_BLUE);
  draw_line(g_width, 0, 0, g_height, COLOR_NEON_BLUE);

  // 4. Depth Elements (Translucent shapes)
  draw_rect_alpha(g_width / 4, g_height / 4, 300, 300, COLOR_CHARCOAL, 40);
  draw_rect_alpha(g_width - 400, g_height - 400, 200, 200, COLOR_NEON_BLUE, 20);
}

void draw_desktop_icons() {
  // "My PC"
  kprint_at_attr(" [PC] ", 2, 2, WIN_CONTENT_ATTR);
  kprint_at_attr("My PC ", 2, 3, WIN_CONTENT_ATTR);

  // "Trash"
  kprint_at_attr(" [TR] ", 2, 5, WIN_CONTENT_ATTR);
  kprint_at_attr("Trash ", 2, 6, WIN_CONTENT_ATTR);

  // "Settings" Shortcut
  kprint_at_attr(" [ST] ", 2, 8, WIN_CONTENT_ATTR);
  kprint_at_attr("Press 's'", 2, 9, WIN_CONTENT_ATTR);
}

void draw_top_bar() {
    // 1. Fedora Style: Slim, Translucent Black (Alpha 220)
    draw_rect_alpha(0, 0, g_width, 32, COLOR_BLACK, 220);

    // 2. Left: "Activities" Button
    draw_string_px(20, 8, "Activities", COLOR_WHITE);

    // 3. Center: Clock
    // "Oct 25 12:00" centered (approx 12 chars * 8 = 96px)
    int clock_x = (g_width / 2) - 48;
    draw_string_px(clock_x, 8, "Oct 25 12:00", COLOR_WHITE);

    // 4. Right: Status Icons
    // "WI-FI  BAT  PWR"
    draw_string_px(g_width - 150, 8, "WI-FI  100%  PWR", COLOR_WHITE);
}

void draw_bottom_bar() {
    int bar_height = 40;
    int y = g_height - bar_height;

    // 1. Frosted Glass Background (Dark Translucent)
    draw_rect_alpha(0, y, g_width, bar_height, 0xFF101010, 200); // Almost black, high alpha
    // Top border highlight for glass effect
    draw_line(0, y, g_width, y, 0xFF404040);

    // 2. Start Button "T"
    // Stylized T logo button
    draw_rounded_rect(10, y + 4, 32, 32, 6, 0xFF202020);
    draw_string_px(22, y + 12, "T", COLOR_NEON_BLUE);

    // 3. Centered App Icons
    int center_x = g_width / 2;
    int icon_w = 40;

    // Code App (Active)
    int code_x = center_x - icon_w - 4;
    draw_rounded_rect(code_x, y + 4, 32, 32, 6, 0xFF303030); // Lighter bg for active
    draw_string_px(code_x + 6, y + 12, "{C}", COLOR_NEON_BLUE);
    // Glow indicator
    draw_line(code_x + 8, y + 38, code_x + 24, y + 38, COLOR_NEON_BLUE);

    // Terminal App
    int term_x = center_x + 4;
    draw_rounded_rect(term_x, y + 4, 32, 32, 6, 0xFF202020);
    draw_string_px(term_x + 6, y + 12, ">_", COLOR_GREEN);

    // 4. Show Desktop Sliver (Far Right)
    draw_rect_px(g_width - 6, y, 6, bar_height, 0xFF404040);

    // 5. System Tray (Time, Lang)
    draw_string_px(g_width - 100, y + 12, "ENG  12:00", COLOR_WHITE);
}

void draw_window(int x, int y, int w, int h, const char *title,
                 const char *content) {
  // Drop Shadow (Semi-transparent black)
  draw_rounded_rect(x + 8, y + 8, w, h, 8, 0x80000000);

  // Window Body (Charcoal)
  draw_rounded_rect(x, y, w, h, 8, COLOR_CHARCOAL);

  // Title Bar Separator
  draw_line(x, y + 28, x + w, y + 28, 0xFF404040);

  // Title Text
  draw_string_px(x + 12, y + 8, title, COLOR_WHITE);

  // Window Controls (Mac-style traffic lights)
  draw_fill_circle(x + w - 16, y + 14, 5, 0xFFFF5F56); // Red
  draw_fill_circle(x + w - 32, y + 14, 5, 0xFFFFBD2E); // Yellow
  draw_fill_circle(x + w - 48, y + 14, 5, 0xFF27C93F); // Green

  // Draw content if provided (Legacy support)
  if (content && *content) {
      draw_string_px(x + 12, y + 36, content, COLOR_WHITE);
  }
}

void open_settings() {
  draw_window(10, 5, 60, 16, " Settings ",
              "Resolution Selection:\n\n"
              "Press '0': Auto (Best Available)\n"
              "Press '1': 1280x720 (HD)\n"
              "Press '2': 1920x1080 (FHD)\n"
              "Press '3': 2560x1440 (QHD)\n"
              "Press '4': 2560x1920 (1920p)\n"
              "Press '5': 3840x2160 (4K)\n\n"
              "System will reboot automatically.");
}

void draw_loading_screen() {
  int center_col = MAX_COLS / 2 - 10;
  int center_row = MAX_ROWS / 2 - 2;

  kprint_at_attr("       T-OS       ", center_col, center_row, WIN_ACCENT_ATTR);
  kprint_at_attr(" System Loading...", center_col, center_row + 2,
                 WIN_CONTENT_ATTR);

  int bar_width = 20;
  int bar_col = center_col;
  int bar_row = center_row + 4;

  kprint_at_attr("[", bar_col - 1, bar_row, WIN_CONTENT_ATTR);
  kprint_at_attr("]", bar_col + bar_width, bar_row, WIN_CONTENT_ATTR);

  for (int i = 0; i < bar_width; i++) {
    char progress[2] = {'=', 0};
    kprint_at_attr(progress, bar_col + i, bar_row, WIN_ACCENT_ATTR);
    swap_buffers();
    delay(3000);
  }
  delay(5000);
}
