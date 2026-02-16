#include "screen.h"
#include "utils.h"
#include "theme.h"
#include "keyboard.h"
#include "ata.h"
#include "ports.h"
#include "timer.h"
#include "idt.h"
#include "power.h"

#include "memory/pmm.h"
#include "memory/vmm.h"
#include "memory/heap.h"
#include "../drivers/mouse.h"
#include "task/scheduler.h"
#include "tester.h"
#include "../drivers/filesystem/fat16.h"

int spawn_process(char* filename);

// Global flag to indicate if a user process is active and should receive input
int user_process_active = 0;

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
void init_idt();

int show_settings = 0;
int show_fps_settings = 0;
int target_fps = 60;

#define TERM_BUF_SIZE 256
char term_input[TERM_BUF_SIZE];
int term_idx = 0;
char term_history[10][64];
int term_hist_count = 0;

void term_print(const char* msg) {
    if (term_hist_count < 10) {
        int len = strlen((char*)msg);
        if (len > 63) len = 63;
        memory_copy((char*)msg, term_history[term_hist_count], len);
        term_history[term_hist_count][len] = 0;
        term_hist_count++;
    } else {
        for (int i=0; i<9; i++) {
            memory_copy(term_history[i+1], term_history[i], 64);
        }
        int len = strlen((char*)msg);
        if (len > 63) len = 63;
        memory_copy((char*)msg, term_history[9], len);
        term_history[9][len] = 0;
    }
}

void process_command(char* cmd) {
    term_print(cmd);

    if (strlen(cmd) == 0) return;

    if (cmd[0] == 'h' && cmd[1] == 'e' && cmd[2] == 'l' && cmd[3] == 'p') {
        term_print("Commands: help, cls, shutdown, reboot, ls, tasm");
        term_print("  tasm: Run assembler");
    }
    else if (cmd[0] == 'c' && cmd[1] == 'l' && cmd[2] == 's') {
        term_hist_count = 0;
    }
    else if (cmd[0] == 's' && cmd[1] == 'h' && cmd[2] == 'u' && cmd[3] == 't') {
        term_print("Shutting down...");
        swap_buffers();
        delay(500);
        shutdown();
    }
    else if (cmd[0] == 'r' && cmd[1] == 'e' && cmd[2] == 'b' && cmd[3] == 'o') {
        term_print("Rebooting...");
        swap_buffers();
        delay(500);
        reboot();
    }
    else if (cmd[0] == 'l' && cmd[1] == 's') {
        term_print("Files (FAT16):");
        fat16_list_directory();
        term_print("Check debug log.");
    }
    else if (cmd[0] == 't' && cmd[1] == 'a' && cmd[2] == 's' && cmd[3] == 'm') {
        term_print("Running TASM...");
        swap_buffers();
        spawn_process("TASM.TEXF");
    }
    else {
        term_print("Unknown command.");
    }
}

void kernel_main(void) {
    init_screen();
    init_pmm();
    init_vmm();
    init_heap();
    init_idt();
    init_timer(1000);
    init_mouse();
    init_scheduler();
    init_fat16();

    __asm__ volatile("sti");

    draw_loading_screen();
    clear_screen();
    run_system_checks();

    memory_set(term_input, 0, TERM_BUF_SIZE);
    // Showcase: Pre-populate terminal with success message
    term_print("user@t-os:~$ nasm -f bin kernel.asm -o kernel.bin");
    term_print("user@t-os:~$ gcc -c kernel.c -o kernel.o");
    term_print("user@t-os:~$ ld -o kernel.bin kernel.o");
    term_print("[SUCCESS] ASM Compilation Finished.");
    term_print("user@t-os:~$ ./kernel.bin");

    while (1) {
        uint32_t start_tick = get_tick_count();

        if (!user_process_active) {
            char c;
            while ((c = get_char())) {
                if (c == '\b') {
                    if (term_idx > 0) {
                        term_input[--term_idx] = 0;
                    }
                } else if (c == '\n') {
                    process_command(term_input);
                    term_idx = 0;
                    memory_set(term_input, 0, TERM_BUF_SIZE);
                } else {
                    if (term_idx < TERM_BUF_SIZE - 1) {
                        term_input[term_idx++] = c;
                        term_input[term_idx] = 0;
                    }
                }
            }
        }

        draw_interface();

        int mx = get_mouse_x();
        int my = get_mouse_y();
        uint8_t mb = get_mouse_buttons();

        if (mb & 1) {
            if (mx > g_width - 30 && mx < g_width - 10 && my > g_height - 30 && my < g_height - 10) {
                shutdown();
            }
            if (mx > g_width - 60 && mx < g_width - 40 && my > g_height - 30 && my < g_height - 10) {
                reboot();
            }
        }

        draw_rect_px(mx, my, 10, 10, COLOR_WHITE);
        draw_rect_px(mx+2, my+2, 6, 6, COLOR_BLACK);

        swap_buffers();

        uint32_t elapsed = get_tick_count() - start_tick;
        uint32_t frame_time = 1000 / target_fps;
        if (elapsed < frame_time) {
            uint32_t wait_ticks = frame_time - elapsed;
            uint32_t end_wait = get_tick_count() + wait_ticks;
            while (get_tick_count() < end_wait) {
                __asm__ volatile("hlt");
            }
        }
    }
}

void draw_interface() {
    draw_wallpaper();

    // C Code Editor Window
    int c_x = 100;
    int c_y = 100;
    int c_w = 400;
    int c_h = 300;
    if (g_width > 1200) { c_x = 200; c_y = 200; c_w = 600; c_h = 400; }

    draw_window_modern(c_x, c_y, c_w, c_h, "code.c - C Editor");
    draw_c_code_content(c_x + 10, c_y + 30);

    // Terminal Window
    int t_x = c_x + c_w + 50;
    int t_y = c_y + 50;
    int t_w = c_w;
    int t_h = c_h;

    // Ensure it fits
    if (t_x + t_w > g_width) {
        // Stack or overlap if screen too small
        t_x = c_x + 50;
        t_y = c_y + 50;
    }

    draw_window_modern(t_x, t_y, t_w, t_h, "Terminal");
    draw_terminal_content(t_x + 10, t_y + 30);

    draw_top_bar();
    draw_bottom_bar();
}

void draw_terminal_content(int x, int y) {
    int line_h = 16;
    int current_y = y;
    uint32_t text_col = 0xFFCCCCCC;
    uint32_t prompt_col = COLOR_NEON_BLUE;

    for (int i=0; i<term_hist_count; i++) {
        draw_string_px(term_history[i], x, current_y, text_col);
        current_y += line_h;
    }

    draw_string_px("user@t-os:~$", x, current_y, prompt_col);
    draw_string_px(term_input, x + 100, current_y, text_col);

    if ((get_tick_count() / 500) % 2) {
        int input_w = strlen(term_input) * 8;
        draw_rect_px(x + 100 + input_w, current_y + 2, 8, 12, text_col);
    }
}

void draw_bottom_bar() {
    int height = (g_height < 600) ? 30 : 40;
    int y = g_height - height;

    draw_rect_alpha(0, y, g_width, height, COLOR_TASKBAR_BG);

    int start_size = height - 10;
    // Stylized T Logo
    int t_x = 5;
    int t_y = y + 5;
    draw_rect_px(t_x, t_y, start_size, 6, COLOR_NEON_BLUE); // Top bar
    draw_rect_px(t_x + (start_size/2) - 3, t_y, 6, start_size, COLOR_NEON_BLUE); // Vertical bar

    int center_x = g_width / 2;
    int icon_size = height - 15;
    int icon_spacing = icon_size + 10;
    // Glowing Blue App Icons
    draw_rect_px(center_x - icon_spacing, y + 5, icon_size, icon_size, COLOR_NEON_BLUE);
    draw_rect_px(center_x, y + 5, icon_size, icon_size, COLOR_NEON_BLUE);
    draw_rect_px(center_x + icon_spacing, y + 5, icon_size, icon_size, COLOR_NEON_BLUE);

    // Show Desktop Sliver
    draw_rect_px(g_width - 5, y, 5, height, 0x80FFFFFF);
}

void draw_wallpaper() {
    // Deep Charcoal & Obsidian Base
    draw_rect_px(0, 0, g_width, g_height, COLOR_OBSIDIAN);

    // Abstract Geometric Shapes (Charcoal)
    draw_line(0, g_height, g_width, 0, COLOR_CHARCOAL);
    draw_line(0, 0, g_width, g_height, COLOR_CHARCOAL);
    draw_rect_alpha(100, 100, 300, 800, 0x201E1E1E); // Vertical strip
    draw_rect_alpha(g_width - 400, 0, 200, g_height, 0x201E1E1E);

    // Subtle Neon Blue Accents (Alpha blended)
    draw_line(0, g_height/2, g_width, g_height/2, 0x4000E5FF); // Horizon line
    draw_line(g_width/3, 0, g_width/3, g_height, 0x2000E5FF);
    draw_line(g_width*2/3, 0, g_width*2/3, g_height, 0x2000E5FF);

    draw_rect_alpha(g_width/2 - 100, g_height/2 - 100, 200, 200, 0x1000E5FF); // Central glow
}

void draw_top_bar() {
    int height = (g_height < 600) ? 20 : 30;
    draw_rect_alpha(0, 0, g_width, height, COLOR_TOP_BAR_BG);

    int text_y = (height - 16) / 2;
    if (text_y < 0) text_y = 0;

    draw_string_px("Activities", 10, text_y, COLOR_TOP_BAR_TEXT);

    char* time = "Oct 25 12:45 PM";
    int time_width = strlen(time) * 8;
    draw_string_px(time, (g_width - time_width)/2, text_y, COLOR_TOP_BAR_TEXT);

    // System Tray Icons
    int icon_y = (height - 12) / 2;
    int tray_x = g_width - 100;

    // WiFi (Signal Bars)
    draw_rect_px(tray_x, icon_y + 9, 3, 3, COLOR_WHITE);
    draw_rect_px(tray_x + 5, icon_y + 6, 3, 6, COLOR_WHITE);
    draw_rect_px(tray_x + 10, icon_y + 3, 3, 9, COLOR_WHITE);
    draw_rect_px(tray_x + 15, icon_y, 3, 12, COLOR_WHITE);

    // Battery
    tray_x += 30;
    draw_rect_px(tray_x, icon_y + 2, 20, 10, COLOR_WHITE); // Body
    draw_rect_px(tray_x + 1, icon_y + 3, 12, 8, COLOR_GREEN); // Level
    draw_rect_px(tray_x + 20, icon_y + 5, 2, 4, COLOR_WHITE); // Tip

    // Power
    tray_x += 35;
    draw_circle(tray_x + 6, icon_y + 6, 5, COLOR_WHITE, 0); // Circle outline
    draw_rect_px(tray_x + 5, icon_y, 2, 6, COLOR_TOP_BAR_BG); // Cut top (Darkens)
    draw_rect_px(tray_x + 5, icon_y, 2, 4, COLOR_WHITE); // Line
}

void draw_window_modern(int x, int y, int w, int h, const char* title) {
    draw_rect_alpha(x + 5, y + 5, w, h, 0x60000000);
    draw_rounded_rect(x, y, w, h, 8, COLOR_CHARCOAL);
    draw_string_px(title, x + 10, y + 5, COLOR_WHITE);

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
    if (current_y + 6*line_h > g_height) return;
    draw_string_px("#include", x, current_y, COLOR_MAGENTA);
    draw_string_px("<stdio.h>", x + 70, current_y, 0xFFCE9178);
    current_y += line_h;
    current_y += line_h;
    draw_string_px("int", x, current_y, COLOR_BLUE);
    draw_string_px("main", x + 30, current_y, COLOR_YELLOW);
    draw_string_px("() {", x + 65, current_y, COLOR_WHITE);
    current_y += line_h;
    draw_string_px("  printf", x, current_y, COLOR_YELLOW);
    draw_string_px("(", x + 70, current_y, COLOR_WHITE);
    draw_string_px("\"Hello!\"", x + 80, current_y, 0xFFCE9178);
    draw_string_px(");", x + 150, current_y, COLOR_WHITE);
    current_y += line_h;
    draw_string_px("  return", x, current_y, COLOR_MAGENTA);
    draw_string_px("0;", x + 70, current_y, 0xFFB5CEA8);
    current_y += line_h;
    draw_string_px("}", x, current_y, COLOR_WHITE);
}

void open_settings() {
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
