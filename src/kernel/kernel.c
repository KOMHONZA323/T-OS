#include "screen.h"
#include "utils.h"
#include "theme.h"
#include "keyboard.h"
#include "ata.h"
#include "ports.h"

void draw_interface();
void draw_wallpaper();
void draw_desktop_icons();
void draw_top_bar();
void draw_bottom_bar();
void draw_window_frame(int x, int y, int w, int h, const char* title);
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
            // Check for 1, 2, 3 selection
            if (c == '1') {
                // 720p
                uint8_t config[512] = {0};
                config[0] = 0xAB; // Magic
                config[1] = 0;    // 720p
                ata_write_sector(2879, config);
                // Reboot
                port_byte_out(0x64, 0xFE);
            } else if (c == '2') {
                // 1080p
                uint8_t config[512] = {0};
                config[0] = 0xAB; // Magic
                config[1] = 1;    // 1080p
                ata_write_sector(2879, config);
                port_byte_out(0x64, 0xFE);
            } else if (c == '3') {
                // 1440p
                uint8_t config[512] = {0};
                config[0] = 0xAB; // Magic
                config[1] = 2;    // 1440p
                ata_write_sector(2879, config);
                port_byte_out(0x64, 0xFE);
            }
        }

        // Draw
        draw_interface();

        // Swap Buffers (Double Buffering)
        swap_buffers();

        if (show_settings) {
            open_settings();
        }
    }
}

void draw_interface() {
    // 1. Layer 0: Wallpaper
    draw_wallpaper();

    // 2. Layer 2: Taskbars (Always on top of wallpaper)
    draw_top_bar();
    draw_bottom_bar();

    // 3. Layer 3: Windows (On top of taskbars)
    if (!show_settings) {
        // --- Window 1: C Code Editor ---
        int w1_x = 80, w1_y = 100, w1_w = 600, w1_h = 400;
        // Adjust for resolution safety
        if (w1_x + w1_w > g_width) w1_w = g_width - w1_x - 10;
        if (w1_y + w1_h > g_height) w1_h = g_height - w1_y - 60;

        draw_window_frame(w1_x, w1_y, w1_w, w1_h, "src/kernel/main.c");

        // Content Area
        int cx = w1_x + 15;
        int cy = w1_y + 40;
        int lh = 20; // Line height

        // Line 1
        draw_string_px(cx, cy, "#include", COLOR_NEON_BLUE);
        draw_string_px(cx + 70, cy, "\"kernel.h\"", 0xFF98C379); // Greenish
        cy += lh;

        // Line 2
        draw_string_px(cx, cy, "void", COLOR_NEON_BLUE);
        draw_string_px(cx + 40, cy, "kernel_main", 0xFFE5C07B); // Yellow/Orange
        draw_string_px(cx + 130, cy, "() {", COLOR_TEXT_MAIN);
        cy += lh;

        // Line 3
        draw_string_px(cx + 20, cy, "// Initialize VBE", COLOR_TEXT_DIM);
        cy += lh;

        // Line 4
        draw_string_px(cx + 20, cy, "init_screen", 0xFF61AFEF); // Blue
        draw_string_px(cx + 110, cy, "();", COLOR_TEXT_MAIN);
        cy += lh;

        // Line 5
        draw_string_px(cx + 20, cy, "draw_desktop", 0xFF61AFEF);
        draw_string_px(cx + 120, cy, "();", COLOR_TEXT_MAIN);
        cy += lh;

        // Line 6
        draw_string_px(cx, cy, "}", COLOR_TEXT_MAIN);

        // --- Window 2: Terminal ---
        int w2_x = 600, w2_y = 200, w2_w = 500, w2_h = 300;
        // Safety check
        if (w2_x > g_width - 100) w2_x = g_width - 500;
        if (w2_x < 0) w2_x = 50;

        draw_window_frame(w2_x, w2_y, w2_w, w2_h, "Terminal");

        cx = w2_x + 15;
        cy = w2_y + 40;

        draw_string_px(cx, cy, "user@t-os:~$", COLOR_NEON_BLUE);
        draw_string_px(cx + 100, cy, "make all", COLOR_TEXT_MAIN);
        cy += lh;

        draw_string_px(cx, cy, "[ASM] boot_sect.bin", COLOR_TEXT_MAIN);
        draw_string_px(cx + 200, cy, "OK", COLOR_CYAN_ACCENT);
        cy += lh;

        draw_string_px(cx, cy, "[CC]  kernel.o", COLOR_TEXT_MAIN);
        draw_string_px(cx + 200, cy, "OK", COLOR_CYAN_ACCENT);
        cy += lh;

        draw_string_px(cx, cy, "[LD]  kernel.bin", COLOR_TEXT_MAIN);
        draw_string_px(cx + 200, cy, "OK", COLOR_CYAN_ACCENT);
        cy += lh;

        draw_string_px(cx, cy, "user@t-os:~$", COLOR_NEON_BLUE);
        draw_string_px(cx + 100, cy, "_", COLOR_TEXT_MAIN); // Cursor
    }
}

void draw_string_px(int x, int y, const char* str, uint32_t fg) {
    int cx = x;
    while (*str) {
        char c = *str;
        uint8_t *glyph = g_font + (unsigned char)c * 16;
        for (int row = 0; row < 16; row++) {
             uint8_t data = glyph[row];
             for (int col = 0; col < 8; col++) {
                 if ((data >> (7 - col)) & 1) {
                     put_pixel(cx + col, y + row, fg);
                 }
             }
        }
        cx += 8;
        str++;
    }
}

void draw_wallpaper() {
    // 1. Fill Background
    draw_rect_px(0, 0, g_width, g_height, COLOR_OBSIDIAN);

    // 2. Geometric Pattern (Diagonal Lines)
    int spacing = 60;
    for (int i = 0; i < g_width + g_height; i += spacing) {
        int x1, y1, x2, y2;

        // Start point (Left or Bottom edge)
        if (i < g_height) {
            x1 = 0; y1 = i;
        } else {
            x1 = i - g_height; y1 = g_height;
        }

        // End point (Top or Right edge)
        if (i < g_width) {
            x2 = i; y2 = 0;
        } else {
            x2 = g_width; y2 = i - g_width;
        }

        draw_line(x1, y1, x2, y2, COLOR_CHARCOAL);
    }

    // 3. Neon Accents (Subtle)
    draw_line(100, g_height - 100, 300, g_height - 300, COLOR_NEON_BLUE);
    draw_line(g_width - 300, 100, g_width - 100, 300, COLOR_NEON_BLUE);
}

void draw_desktop_icons() {
    // Placeholder - not used in this concept
}

void draw_top_bar() {
    // Height: 24px
    int bar_h = 24;
    draw_rect_alpha(0, 0, g_width, bar_h, COLOR_TOP_BAR_BG, 200); // ~80% opacity

    // Left: "Activities"
    draw_string_px(10, 4, "Activities", COLOR_TOP_BAR_TEXT);

    // Center: Clock
    char *clock_str = "Oct 25 12:00";
    int clock_w = 12 * 8;
    draw_string_px((g_width - clock_w)/2, 4, clock_str, COLOR_TOP_BAR_TEXT);

    // Right: Status Icons
    char *status = "WF  BT  PWR";
    int status_w = 11 * 8;
    draw_string_px(g_width - status_w - 10, 4, status, COLOR_TOP_BAR_TEXT);
}

void draw_bottom_bar() {
    // Height: 48px
    int bar_h = 48;
    int y_start = g_height - bar_h;

    draw_rect_alpha(0, y_start, g_width, bar_h, COLOR_TASKBAR_BG, 170); // Semi-transparent

    // Start Button (Left)
    int start_x = 10;
    draw_string_px(start_x, y_start + 16, " T ", COLOR_NEON_BLUE);

    // Center Apps
    int icon_size = 32;
    int spacing = 10;
    int num_apps = 3;
    int total_w = num_apps * icon_size + (num_apps - 1) * spacing;
    int center_x = (g_width - total_w) / 2;

    // App 1: Code (Active)
    int app1_x = center_x;
    draw_rect_px(app1_x, y_start + 8, icon_size, icon_size, COLOR_CHARCOAL); // Icon BG
    draw_string_px(app1_x + 12, y_start + 16, "C", COLOR_NEON_BLUE);
    // Glow/Underline
    draw_rect_px(app1_x + 4, y_start + bar_h - 4, icon_size - 8, 2, COLOR_NEON_BLUE);

    // App 2: Terminal
    int app2_x = center_x + icon_size + spacing;
    draw_rect_px(app2_x, y_start + 8, icon_size, icon_size, COLOR_CHARCOAL);
    draw_string_px(app2_x + 8, y_start + 16, ">_", COLOR_TEXT_DIM);

    // App 3: Browser
    int app3_x = center_x + 2*(icon_size + spacing);
    draw_rect_px(app3_x, y_start + 8, icon_size, icon_size, COLOR_CHARCOAL);
    draw_string_px(app3_x + 12, y_start + 16, "W", COLOR_TEXT_DIM);

    // Right: Show Desktop Sliver
    draw_rect_px(g_width - 5, y_start, 5, bar_h, 0x80FFFFFF);
}

void draw_window_frame(int x, int y, int w, int h, const char* title) {
    // 1. Drop Shadow
    int shadow_offset = 8;
    draw_rounded_rect(x + shadow_offset, y + shadow_offset, w, h, 10, COLOR_WIN_SHADOW);

    // 2. Window Body
    draw_rounded_rect(x, y, w, h, 10, COLOR_WIN_BG);

    // 3. Title Bar
    // Separator line
    int title_h = 30;
    draw_rect_px(x, y + title_h, w, 1, COLOR_WIN_BORDER);

    // Text
    draw_string_px(x + 15, y + 8, title, COLOR_WIN_TITLE);

    // Close Button "X"
    draw_string_px(x + w - 20, y + 8, "X", COLOR_TEXT_DIM);
}

void open_settings() {
    int w = 500;
    int h = 300;
    int x = (g_width - w) / 2;
    int y = (g_height - h) / 2;

    draw_window_frame(x, y, w, h, "Settings");

    int cx = x + 20;
    int cy = y + 40;

    draw_string_px(cx, cy, "Resolution Selection:", COLOR_TEXT_MAIN);
    cy += 30;
    draw_string_px(cx, cy, "Press '1': 1280x720 (HD)", COLOR_TEXT_MAIN);
    cy += 20;
    draw_string_px(cx, cy, "Press '2': 1920x1080 (FHD)", COLOR_TEXT_MAIN);
    cy += 20;
    draw_string_px(cx, cy, "Press '3': 2560x1440 (QHD)", COLOR_TEXT_MAIN);
    cy += 40;
    draw_string_px(cx, cy, "System will reboot automatically.", COLOR_TEXT_DIM);
}

void draw_loading_screen() {
    int center_col = MAX_COLS / 2 - 10;
    int center_row = MAX_ROWS / 2 - 2;

    kprint_at_attr("       T-OS       ", center_col, center_row, WIN_ACCENT_ATTR);
    kprint_at_attr(" System Loading...", center_col, center_row + 2, WIN_CONTENT_ATTR);

    int bar_width = 20;
    int bar_col = center_col;
    int bar_row = center_row + 4;

    kprint_at_attr("[", bar_col - 1, bar_row, WIN_CONTENT_ATTR);
    kprint_at_attr("]", bar_col + bar_width, bar_row, WIN_CONTENT_ATTR);

    for (int i = 0; i < bar_width; i++) {
        char progress[2] = { '=', 0 };
        kprint_at_attr(progress, bar_col + i, bar_row, WIN_ACCENT_ATTR);
        delay(3000);
    }

    delay(5000);
}
