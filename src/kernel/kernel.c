#include "screen.h"
#include "utils.h"
#include "theme.h"
#include "keyboard.h"
#include "ata.h"
#include "ports.h"
#include "../drivers/timer.h"
#include "idt.h"

void draw_interface();
void draw_wallpaper();
void draw_desktop_icons();
void draw_top_bar();
void draw_bottom_bar();
void draw_window(int x, int y, int w, int h, const char* title, const char* content);
void draw_loading_screen();
void open_settings();
void open_fps_settings();
void init_idt(); // Defined in isr.c, linked later

// Global state
int show_settings = 0;
int show_fps_settings = 0;
int target_fps = 30; // Default 30 FPS

void kernel_main(void) {
    // 1. Initialize Screen (VBE)
    init_screen();

    // 2. Initialize Interrupts & Timer
    init_idt();
    init_timer(1000); // 1000 Hz = 1ms per tick
    __asm__ volatile("sti");

    // 3. Loading Screen
    draw_loading_screen();
    clear_screen();

    // 4. Main Loop
    while (1) {
        uint32_t start_tick = get_tick_count();

        // Handle Input
        char c = get_char();
        if (c == 's') {
            show_settings = !show_settings;
            show_fps_settings = 0;
            clear_screen(); // Clear to redraw background
        } else if (c == 'f') {
            show_fps_settings = !show_fps_settings;
            show_settings = 0;
            clear_screen();
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

        // Logic for FPS Settings
        if (show_fps_settings) {
            if (c == '1') target_fps = 30;
            else if (c == '2') target_fps = 45;
            else if (c == '3') target_fps = 60;
            else if (c == '4') target_fps = 120;
        }

        // Draw
        draw_interface();

        if (show_settings) {
            open_settings();
        }

        if (show_fps_settings) {
            open_fps_settings();
        }

        // FPS Limiter (using PIT)
        uint32_t elapsed = get_tick_count() - start_tick;
        uint32_t frame_time = 1000 / target_fps;
        if (elapsed < frame_time) {
            uint32_t wait_ticks = frame_time - elapsed;
            uint32_t end_wait = get_tick_count() + wait_ticks;
            while (get_tick_count() < end_wait) {
                // Busy wait or halt
                __asm__ volatile("hlt");
            }
        }
    }
}

void open_fps_settings() {
    draw_window(10, 5, 40, 15, " FPS Settings ",
        "Select FPS Limit:\n\n"
        "Press '1': 30 FPS\n"
        "Press '2': 45 FPS\n"
        "Press '3': 60 FPS\n"
        "Press '4': 120 FPS\n\n"
        "Current: Variable"); // Ideally we show the current value

    // Simple way to show current selection
    if (target_fps == 30) kprint_at_attr("Current: 30 ", 12, 16, WIN_ACCENT_ATTR);
    else if (target_fps == 45) kprint_at_attr("Current: 45 ", 12, 16, WIN_ACCENT_ATTR);
    else if (target_fps == 60) kprint_at_attr("Current: 60 ", 12, 16, WIN_ACCENT_ATTR);
    else if (target_fps == 120) kprint_at_attr("Current: 120", 12, 16, WIN_ACCENT_ATTR);
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
        // Only show these if settings not open, to keep it clean?
        // Or show them behind.

        // C Code Editor
        draw_window(3, 4, 38, 14, " kernel.c ",
            "void main() {\n"
            "  // T-OS Kernel\n"
            "  init_video();\n"
            "  kprint(\"Hello\");\n"
            "  while(1);\n"
            "}");

        // Terminal
        draw_window(44, 7, 32, 11, " Terminal ",
            "$ make\n"
            "[OK] Kernel built.\n"
            "[OK] Bootloader.\n"
            "$ ./run\n"
            "Starting T-OS..."
        );
    }
}

void draw_wallpaper() {
    // Fill with 'Deep Charcoal' pattern
    draw_fill(0, 0, MAX_COLS, MAX_ROWS, WALLPAPER_CHAR, WALLPAPER_ATTR);
}

void draw_desktop_icons() {
    // Draw Icons (Layer 1)

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
    kprint_at_attr(" Oct 25 12:00 ", 35, 0, WIN_ACCENT_ATTR);

    // Right: Status Icons
    kprint_at_attr(" WF BT PW ", 70, 0, TOP_BAR_TEXT_ATTR);
}

void draw_bottom_bar() {
    // Windows Hybrid: Thicker (3 Rows)
    int y_start = MAX_ROWS - 3;
    if (y_start < 0) y_start = 0; // Safety

    draw_fill(0, y_start, MAX_COLS, 3, TASKBAR_CHAR, TASKBAR_ATTR);

    // Start Button
    kprint_at_attr(" [T] ", 1, y_start + 1, WIN_ACCENT_ATTR);

    // Centered Apps
    int center = 30;
    kprint_at_attr(" [Code] ", center, y_start + 1, WIN_ACCENT_ATTR);

    // Active line
    char active_line[] = { '_', '_', '_', '_', '_', '_', '_', '_', 0 };
    kprint_at_attr(active_line, center, y_start + 2, WIN_ACCENT_ATTR);

    kprint_at_attr(" [Term] ", center + 10, y_start + 1, WIN_CONTENT_ATTR);

    // System Tray
    kprint_at_attr(" ^  ENG  12:00 ", 65, y_start + 1, WIN_CONTENT_ATTR);
}

void draw_window(int x, int y, int w, int h, const char* title, const char* content) {
    // Check bounds
    if (x + w > MAX_COLS) w = MAX_COLS - x;
    if (y + h > MAX_ROWS) h = MAX_ROWS - y;

    draw_box_rounded(x, y, w, h, WIN_BORDER_ATTR, WIN_CONTENT_ATTR, WIN_TITLE_ATTR);

    // Title
    kprint_at_attr(" X ", x + w - 4, y, WIN_ACCENT_ATTR);
    kprint_at_attr( (char*)title, x + 2, y, WIN_TITLE_ATTR);

    // Content
    int cx = x + 2;
    int cy = y + 2;

    const char* p = content;
    while(*p) {
        if(*p == '\n') {
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
    draw_window(10, 5, 60, 15, " Settings ",
        "Resolution Selection:\n\n"
        "Press '1': 1280x720 (HD)\n"
        "Press '2': 1920x1080 (FHD)\n"
        "Press '3': 2560x1440 (QHD)\n\n"
        "System will reboot automatically.");
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
