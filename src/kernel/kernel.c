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
    // Only show these if settings not open

    // C Code Editor
    draw_window(10, 4, 34, 14, " kernel.c ",
                "void main() {\n"
                "  // T-OS Kernel\n"
                "  init_video();\n"
                "  kprint(\"Hello\");\n"
                "  while(1);\n"
                "}");

    // Terminal
    draw_window(46, 7, 32, 11, " Terminal ",
                "$ make\n"
                "[OK] Kernel built.\n"
                "[OK] Bootloader.\n"
                "$ ./run\n"
                "Starting T-OS...");
  }
}

void draw_wallpaper() {
  // Use a solid fill so ultra-high resolutions do not introduce visible
  // font-pattern banding artifacts across the desktop background.
  draw_rect(0, 0, MAX_COLS, MAX_ROWS, WALLPAPER_ATTR);
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

    // Fedora Style: Slim (1 Row)
    draw_fill(0, 0, MAX_COLS, 1, ' ', TOP_BAR_BG);

    // Left: "Activities"
    kprint_at_attr(" Activities ", 1, 0, TOP_BAR_TEXT_ATTR);

    // Center: Clock
    int clock_x = (MAX_COLS / 2) - 7;
    kprint_at_attr(" Oct 25 12:00 ", clock_x, 0, TOP_BAR_TEXT_ATTR);

    // Right: Status Icons
    kprint_at_attr(" WF  BT  PW ", MAX_COLS - 14, 0, TOP_BAR_TEXT_ATTR);

}

void draw_bottom_bar() {
  int y_start = MAX_ROWS - 3;
  if (y_start < 0)
    y_start = 0;

  draw_fill(0, y_start, MAX_COLS, 3, TASKBAR_CHAR, TASKBAR_ATTR);
  kprint_at_attr(" [T] ", 1, y_start + 1, WIN_ACCENT_ATTR);

    // Centered Apps
    int center = (MAX_COLS / 2) - 8;
    kprint_at_attr(" [Code] ", center, y_start + 1, WIN_ACCENT_ATTR);

  char active_line[] = {'_', '_', '_', '_', '_', '_', '_', '_', 0};
  kprint_at_attr(active_line, center, y_start + 2, WIN_ACCENT_ATTR);

    kprint_at_attr(" [Term] ", center + 10, y_start + 1, WIN_CONTENT_ATTR);

    // System Tray
    kprint_at_attr(" ^  ENG  12:00 ", MAX_COLS - 16, y_start + 1, WIN_CONTENT_ATTR);
}

void draw_window(int x, int y, int w, int h, const char *title,
                 const char *content) {
  if (x + w > MAX_COLS)
    w = MAX_COLS - x;
  if (y + h > MAX_ROWS)
    h = MAX_ROWS - y;

  draw_box_rounded(x, y, w, h, WIN_BORDER_ATTR, WIN_CONTENT_ATTR,
                   WIN_TITLE_ATTR);

  kprint_at_attr(" X ", x + w - 4, y, WIN_ACCENT_ATTR);
  kprint_at_attr((char *)title, x + 2, y, WIN_TITLE_ATTR);

  int cx = x + 2;
  int cy = y + 2;

  const char *p = content;
  while (*p) {
    if (*p == '\n') {
      cy++;
      cx = x + 2;
    } else {
      char str[2] = {*p, 0};
      if (cx < x + w - 1 && cy < y + h - 1) {
        kprint_at_attr(str, cx, cy, WIN_CONTENT_ATTR);
      }
      cx++;
    }
    p++;
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
