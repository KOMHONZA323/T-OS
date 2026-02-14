#include "screen.h"
#include "utils.h"
#include "theme.h"
#include "keyboard.h"
#include "ata.h"
#include "ports.h"
#include "timer.h"
#include "idt.h"

#include "memory/pmm.h"
#include "memory/vmm.h"
#include "memory/heap.h"
#include "../drivers/mouse.h"
#include "task/scheduler.h"
#include "tester.h"

/* Forward Declarations */
void draw_interface();
void draw_wallpaper();
void draw_top_bar();
void draw_bottom_bar();
void draw_window_modern(int x, int y, int w, int h, const char* title);
void draw_c_code_content(int x, int y);
void draw_terminal_content(int x, int y);
void draw_loading_screen();
void open_settings();
void open_fps_settings();
void init_idt(); // Defined in isr.c

// Global state
int show_settings = 0;
int show_fps_settings = 0;
int target_fps = 30; // Default 30 FPS

void kernel_main(void) {
    // 1. Initialize Screen (VBE)
    init_screen();

    // 2. Memory Management
    init_pmm();
    init_vmm();
    init_heap();

    // 3. Initialize Interrupts & Timer
    init_idt();
    init_timer(1000); // 1000 Hz = 1ms per tick

    // 4. Input & Multitasking
    init_mouse();
    init_scheduler();

    __asm__ volatile("sti");

    // 5. Loading Screen
    draw_loading_screen();
    clear_screen();

    // 6. System Checks
    run_system_checks();

    // 7. Main Loop
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
            if (c == '1') {
                uint8_t config[512] = {0};
                config[0] = 0xAB; config[1] = 0; // 720p
                ata_write_sector(2879, config);
                port_byte_out(0x64, 0xFE); // Reboot
            } else if (c == '2') {
                uint8_t config[512] = {0};
                config[0] = 0xAB; config[1] = 1; // 1080p
                ata_write_sector(2879, config);
                port_byte_out(0x64, 0xFE);
            } else if (c == '3') {
                uint8_t config[512] = {0};
                config[0] = 0xAB; config[1] = 2; // 1440p
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

        // Draw Mouse Cursor
        int mx = get_mouse_x();
        int my = get_mouse_y();
        // Simple pointer
        draw_rect_px(mx, my, 10, 10, COLOR_WHITE);
        draw_rect_px(mx+2, my+2, 6, 6, COLOR_BLACK);

        // Swap Buffers (VSync)
        swap_buffers();

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

void draw_interface() {
    // 1. Wallpaper
    draw_wallpaper();

    // 2. Windows
    // Use relative coordinates for windows using integer arithmetic

    // Window 1: C Code Editor (Left side)
    int w1_x = (g_width * 5) / 100; // 5% from left
    int w1_y = (g_height * 10) / 100; // 10% from top
    int w1_w = (g_width * 40) / 100;  // 40% width
    int w1_h = (g_height * 50) / 100; // 50% height

    draw_window_modern(w1_x, w1_y, w1_w, w1_h, "main.c - Visual Studio Code");
    draw_c_code_content(w1_x + 10, w1_y + 30);

    // Window 2: Terminal (Right side overlapping)
    int w2_x = (g_width * 35) / 100; // Overlap
    int w2_y = (g_height * 30) / 100; // Lower
    int w2_w = (g_width * 45) / 100;
    int w2_h = (g_height * 40) / 100;

    draw_window_modern(w2_x, w2_y, w2_w, w2_h, "Terminal");
    draw_terminal_content(w2_x + 10, w2_y + 30);

    // 3. Bars
    draw_top_bar();
    draw_bottom_bar();
}

void draw_wallpaper() {
    // Deep charcoal and obsidian abstract geometric wallpaper
    draw_rect_px(0, 0, g_width, g_height, COLOR_OBSIDIAN);

    // Subtle neon blue accents (geometric lines)
    // Diagonal lines
    draw_line(0, g_height, g_width, 0, COLOR_CHARCOAL);
    draw_line(0, 0, g_width, g_height, COLOR_CHARCOAL);

    // Abstract shapes
    // Faint blue box
    draw_rect_alpha(g_width/4, g_height/4, g_width/2, g_height/2, 0x1000E5FF);
    // Faint circle
    draw_circle(g_width/2, g_height/2, g_height/3, 0x101E1E1E, 0);
}

void draw_top_bar() {
    int height = (g_height < 600) ? 20 : 30; // Smaller bar on VGA
    // Translucent black top bar
    draw_rect_alpha(0, 0, g_width, height, COLOR_TOP_BAR_BG);

    int text_y = (height - 16) / 2;
    if (text_y < 0) text_y = 0;

    // "Activities" button
    draw_string_px("Activities", 10, text_y, COLOR_TOP_BAR_TEXT);

    // Centered Clock
    char* time = "Oct 25 12:45 PM";
    int time_width = strlen(time) * 8;
    draw_string_px(time, (g_width - time_width)/2, text_y, COLOR_TOP_BAR_TEXT);

    // System Tray (Wi-Fi, Battery, Power)
    char* tray = "WF BT PWR";
    int tray_width = strlen(tray) * 8;
    draw_string_px(tray, g_width - tray_width - 10, text_y, COLOR_TOP_BAR_TEXT);
}

void draw_bottom_bar() {
    int height = (g_height < 600) ? 30 : 40;
    int y = g_height - height;

    // Semi-transparent frosted glass taskbar
    draw_rect_alpha(0, y, g_width, height, COLOR_TASKBAR_BG);

    // Start Button "T"
    int start_size = height - 10;
    draw_rect_px(5, y + 5, start_size, start_size, COLOR_NEON_BLUE);
    draw_string_px("T", 10, y + (height-16)/2, COLOR_OBSIDIAN);

    // Centered App Icons (Glowing blue-tinted)
    int center_x = g_width / 2;
    int icon_size = height - 15; // slightly smaller than bar
    int icon_spacing = icon_size + 10;

    // Icon 1 (Active)
    draw_rect_px(center_x - icon_spacing, y + 5, icon_size, icon_size, 0xFF4080FF);
    draw_rect_px(center_x - icon_spacing, y + height - 2, icon_size, 2, COLOR_NEON_BLUE); // Glow line

    // Icon 2
    draw_rect_px(center_x, y + 5, icon_size, icon_size, 0xFF4080FF);

    // Icon 3
    draw_rect_px(center_x + icon_spacing, y + 5, icon_size, icon_size, 0xFF4080FF);

    // Show Desktop Sliver
    draw_rect_px(g_width - 5, y, 5, height, 0x50FFFFFF);
}

void draw_window_modern(int x, int y, int w, int h, const char* title) {
    // Drop shadow
    draw_rect_alpha(x + 5, y + 5, w, h, 0x60000000);

    // Window Body
    draw_rounded_rect(x, y, w, h, 8, COLOR_CHARCOAL);

    // Title Bar Area (Implicit in body, just draw title)
    // Clip title if window is too small?
    // Just draw
    draw_string_px(title, x + 10, y + 5, COLOR_WHITE);

    // Window Controls (Traffic lights)
    int radius = 4;
    int spacing = 12;
    int controls_x = x + w - 15;
    int controls_y = y + 10;

    draw_circle(controls_x, controls_y, radius, COLOR_RED, 1);
    draw_circle(controls_x - spacing, controls_y, radius, COLOR_YELLOW, 1);
    draw_circle(controls_x - spacing*2, controls_y, radius, COLOR_GREEN, 1);
}

void draw_c_code_content(int x, int y) {
    int line_h = 16;
    int current_y = y;

    // Only draw if we have space
    if (current_y + 6*line_h > g_height) return;

    // Line 1: #include <stdio.h>
    draw_string_px("#include", x, current_y, COLOR_MAGENTA);
    draw_string_px("<stdio.h>", x + 70, current_y, 0xFFCE9178); // Orange-ish
    current_y += line_h;

    // Line 2:
    current_y += line_h;

    // Line 3: int main() {
    draw_string_px("int", x, current_y, COLOR_BLUE);
    draw_string_px("main", x + 30, current_y, COLOR_YELLOW);
    draw_string_px("() {", x + 65, current_y, COLOR_WHITE);
    current_y += line_h;

    // Line 4: printf("Hello T-OS!\n");
    draw_string_px("  printf", x, current_y, COLOR_YELLOW);
    draw_string_px("(", x + 70, current_y, COLOR_WHITE); // adjusted x
    draw_string_px("\"Hello!\"", x + 80, current_y, 0xFFCE9178); // shorter string
    draw_string_px(");", x + 150, current_y, COLOR_WHITE);
    current_y += line_h;

    // Line 5: return 0;
    draw_string_px("  return", x, current_y, COLOR_MAGENTA);
    draw_string_px("0;", x + 70, current_y, 0xFFB5CEA8);
    current_y += line_h;

    // Line 6: }
    draw_string_px("}", x, current_y, COLOR_WHITE);
}

void draw_terminal_content(int x, int y) {
    int line_h = 16;
    int current_y = y;
    uint32_t text_col = 0xFFCCCCCC;
    uint32_t prompt_col = COLOR_NEON_BLUE;
    uint32_t success_col = COLOR_GREEN;

    // Only draw if we have space
    if (current_y + 4*line_h > g_height) return;

    draw_string_px("user@t-os:~$", x, current_y, prompt_col);
    draw_string_px(" make build", x + 100, current_y, text_col);
    current_y += line_h;

    draw_string_px("[OK] Kernel.", x, current_y, success_col);
    current_y += line_h;

    draw_string_px("[OK] Boot.", x, current_y, success_col);
    current_y += line_h;

    draw_string_px("user@t-os:~$", x, current_y, prompt_col);
    draw_rect_px(x + 100, current_y + 2, 8, 12, text_col); // Block cursor
}

void open_settings() {
    // Legacy settings window using new primitives or old?
    // Let's use old style for simplicity if we can, or just minimal new style.
    // For now, simple text box.
    draw_rect_px(100, 100, 400, 200, COLOR_CHARCOAL);
    draw_string_px("Settings - Press 1, 2, 3 to change res", 120, 120, COLOR_WHITE);
}

void open_fps_settings() {
    draw_rect_px(100, 100, 400, 200, COLOR_CHARCOAL);
    draw_string_px("FPS Settings - Press 1, 2, 3, 4", 120, 120, COLOR_WHITE);
}

void draw_loading_screen() {
    int center_col = g_width / 2 - 80;
    int center_row = g_height / 2 - 20;

    // We might not have width/height set up correctly if init_screen not called or if text mode?
    // init_screen() is called before this.
    // draw_string_px uses pixel coords.

    draw_string_px("       T-OS       ", center_col, center_row, COLOR_NEON_BLUE);
    draw_string_px(" System Loading...", center_col, center_row + 20, COLOR_WHITE);

    int bar_width = 200;
    int bar_height = 10;
    draw_rect_px(center_col, center_row + 50, bar_width, bar_height, COLOR_CHARCOAL);

    for (int i = 0; i < bar_width; i+=5) {
        draw_rect_px(center_col, center_row + 50, i, bar_height, COLOR_NEON_BLUE);
        swap_buffers();
        delay(100);
    }
    delay(2000);
}
