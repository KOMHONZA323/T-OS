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
int tab_count = 0;

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

const char* COMMANDS[] = { "help", "cls", "shutdown", "reboot", "ls", "tasm", 0 };

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

// In fat16.c
void fat16_list_matches(char* partial);

void autocomplete_list() {
    int space_idx = -1;
    for (int i=term_idx-1; i>=0; i--) {
        if (term_input[i] == ' ') {
            space_idx = i;
            break;
        }
    }

    if (space_idx != -1) {
        // List matching files
        char* partial = term_input + space_idx + 1;
        fat16_list_matches(partial);
    } else {
        // List matching commands
        for (int i = 0; COMMANDS[i] != 0; i++) {
            term_print(COMMANDS[i]);
        }
    }
}

void autocomplete() {
    if (term_idx == 0) return;

    int space_idx = -1;
    for (int i=term_idx-1; i>=0; i--) {
        if (term_input[i] == ' ') {
            space_idx = i;
            break;
        }
    }

    if (space_idx != -1) {
        char* partial = term_input + space_idx + 1;
        // if (strlen(partial) == 0) return; // Allow empty

        char found_name[32];
        if (fat16_find_file(partial, found_name)) {
            int found_len = strlen(found_name);
            int base_len = space_idx + 1;

            if (base_len + found_len < TERM_BUF_SIZE) {
                memory_copy(found_name, term_input + base_len, found_len);
                term_input[base_len + found_len] = 0;
                term_idx = base_len + found_len;
            }
        }
        return;
    }

    for (int i = 0; COMMANDS[i] != 0; i++) {
        int match = 1;
        for (int j = 0; j < term_idx; j++) {
            if (COMMANDS[i][j] != term_input[j]) {
                match = 0;
                break;
            }
        }

        if (match) {
            int len = strlen((char*)COMMANDS[i]);
            if (len >= TERM_BUF_SIZE) len = TERM_BUF_SIZE - 1;
            memory_copy((char*)COMMANDS[i], term_input, len);
            term_input[len] = 0;
            term_idx = len;
            return;
        }
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
    else if (cmd[0] == 't' && cmd[1] == 'g' && cmd[2] == 'c') {
        term_print("Running TGC...");
        swap_buffers();
        spawn_process("TGC.TEXF");
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

    // Pre-populate history for showcase
    term_print("T-OS Terminal v1.0");
    term_print("Type 'help' for commands.");
    term_print("");
    term_print("user@t-os:~/src$ tasm kernel.asm");
    term_print("Assembler v1.0 ... Starts");
    term_print("Pass 1... OK");
    term_print("Pass 2... OK");
    term_print("Success: kernel.texf generated.");

    while (1) {
        uint32_t start_tick = get_tick_count();

        if (!user_process_active) {
            char c;
            while ((c = get_char())) {
                if (c == '\t') { // TAB
                    tab_count++;
                    if (tab_count >= 3) {
                        autocomplete_list();
                        tab_count = 0; // Reset?
                    } else {
                        autocomplete();
                    }
                } else {
                    tab_count = 0; // Reset count on any other key

                    if (c == '\b') {
                        if (term_idx > 0) {
                            term_input[--term_idx] = 0;
                        }
                    } else if (c == '\n') {
                        process_command(term_input);
                        term_idx = 0;
                        memory_set(term_input, 0, TERM_BUF_SIZE);
                    } else {
                        if (term_idx < TERM_BUF_SIZE - 1 && c >= 32 && c <= 126) {
                            term_input[term_idx++] = c;
                            term_input[term_idx] = 0;
                        }
                    }
                }
            }
        }

        draw_interface();

        int mx = get_mouse_x();
        int my = get_mouse_y();
        uint8_t mb = get_mouse_buttons();

        if (mb & 1) {
            // Power Button Logic (Top Right)
            if (mx > g_width - 40 && my < 30) {
                shutdown();
            }

            // Start Button Logic (Bottom Left)
            if (mx < 50 && my > g_height - 50) {
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

    // Draw C Code Editor Window (Top Left-ish)
    int win1_x = 80;
    int win1_y = 60;
    int win1_w = 500;
    int win1_h = 350;

    draw_window_modern(win1_x, win1_y, win1_w, win1_h, "editor.c - Code");
    draw_c_code_content(win1_x + 10, win1_y + 40);

    // Draw Terminal Window (Bottom Right-ish, overlapping)
    int win2_x = 300;
    int win2_y = 200;
    int win2_w = 500;
    int win2_h = 350;

    draw_window_modern(win2_x, win2_y, win2_w, win2_h, "Terminal");
    draw_terminal_content(win2_x + 10, win2_y + 40);

    draw_top_bar();
    draw_bottom_bar();
}

void draw_wallpaper() {
    // Obsidian Background
    draw_rect_px(0, 0, g_width, g_height, COLOR_OBSIDIAN);

    // Abstract Geometric Patterns
    // Draw a "mesh" or grid faintly
    for (int i=0; i<g_width; i+=100) {
        draw_line(i, 0, i+200, g_height, 0x10303030);
    }

    // Some glowing rects
    draw_rect_alpha(g_width/4, g_height/3, 200, 5, 0x4000E5FF);
    draw_rect_alpha(g_width/2, g_height/2, 5, 200, 0x4000E5FF);

    // Diagonal line
    draw_line(0, g_height, g_width, 0, COLOR_CHARCOAL);
}

void draw_top_bar() {
    int height = 28;
    draw_rect_alpha(0, 0, g_width, height, COLOR_TOP_BAR_BG);

    int text_y = (height - 16) / 2;
    if (text_y < 0) text_y = 0; // centered vertically

    // Activities
    draw_string_px("Activities", 10, text_y, COLOR_WHITE);

    // Center Clock
    char* time = "Oct 25 12:45 PM";
    int time_w = strlen(time) * 8;
    draw_string_px(time, (g_width - time_w)/2, text_y, COLOR_WHITE);

    // Right Icons (Wi-Fi, Battery, Power)
    int icon_x = g_width - 20;

    // Power (Circle)
    draw_circle(icon_x, height/2, 6, COLOR_WHITE, 0); // Open circle
    draw_line(icon_x, height/2 - 4, icon_x, height/2, COLOR_WHITE); // Line

    icon_x -= 30;
    // Battery (Rect)
    draw_rect_px(icon_x, height/2 - 5, 16, 10, COLOR_WHITE);
    draw_rect_px(icon_x + 16, height/2 - 2, 2, 4, COLOR_WHITE); // Tip
    draw_rect_px(icon_x + 2, height/2 - 3, 10, 6, COLOR_GREEN); // Fill

    icon_x -= 30;
    // Wi-Fi (Ascending bars)
    draw_rect_px(icon_x, height/2 + 2, 2, 2, COLOR_WHITE);
    draw_rect_px(icon_x + 4, height/2 - 1, 2, 5, COLOR_WHITE);
    draw_rect_px(icon_x + 8, height/2 - 4, 2, 8, COLOR_WHITE);
    draw_rect_px(icon_x + 12, height/2 - 7, 2, 11, COLOR_WHITE);
}

void draw_bottom_bar() {
    int height = 48;
    int y = g_height - height;

    // Frosted Glass Taskbar
    draw_rect_alpha(0, y, g_width, height, COLOR_TASKBAR_BG);

    // "T" Start Button
    int start_x = 10;
    int start_y = y + 8;
    // Stylized T
    draw_rect_px(start_x, start_y, 32, 8, COLOR_NEON_BLUE);
    draw_rect_px(start_x + 12, start_y, 8, 32, COLOR_NEON_BLUE);

    // App Icons (Center)
    int center_x = g_width / 2;
    int icon_size = 32;
    int spacing = 48;
    int icon_y = y + 8;

    // Icon 1
    draw_rounded_rect(center_x - spacing, icon_y, icon_size, icon_size, 4, 0xFF4080FF);
    // Icon 2
    draw_rounded_rect(center_x, icon_y, icon_size, icon_size, 4, 0xFF4080FF);
    // Icon 3
    draw_rounded_rect(center_x + spacing, icon_y, icon_size, icon_size, 4, 0xFF4080FF);

    // Glow under middle icon
    draw_rect_px(center_x + 4, y + height - 2, icon_size - 8, 2, COLOR_NEON_BLUE);

    // Show Desktop Sliver
    draw_rect_px(g_width - 5, y, 5, height, 0x50FFFFFF);
}

void draw_window_modern(int x, int y, int w, int h, const char* title) {
    // Drop Shadow
    draw_rounded_rect(x + 8, y + 8, w, h, 8, 0x60000000);

    // Window Body
    draw_rounded_rect(x, y, w, h, 8, COLOR_CHARCOAL);

    // Title
    draw_string_px(title, x + 16, y + 10, COLOR_WHITE);

    // Controls (Right aligned)
    int cx = x + w - 20;
    int cy = y + 16;
    draw_circle(cx, cy, 6, COLOR_RED, 1);
    draw_circle(cx - 20, cy, 6, COLOR_YELLOW, 1);
    draw_circle(cx - 40, cy, 6, COLOR_GREEN, 1);
}

void draw_c_code_content(int x, int y) {
    int lh = 20; // Line height
    int cy = y;
    if (cy + 7*lh > g_height) return;

    // Line numbers + Code
    draw_string_px("1  #include <stdio.h>", x, cy, 0xFFC586C0); // Magenta-ish
    cy += lh;
    draw_string_px("2  ", x, cy, 0xFF858585);
    cy += lh;
    draw_string_px("3  int main() {", x, cy, 0xFF569CD6); // Blue-ish
    cy += lh;
    draw_string_px("4      // T-OS Kernel Entry", x, cy, 0xFF6A9955); // Green comment
    cy += lh;
    draw_string_px("5      printf(\"Welcome to T-OS\");", x, cy, 0xFFCE9178); // String color
    cy += lh;
    draw_string_px("6      return 0;", x, cy, 0xFFC586C0);
    cy += lh;
    draw_string_px("7  }", x, cy, 0xFFD4D4D4);
}

void draw_terminal_content(int x, int y) {
    int line_h = 16;
    int current_y = y;
    uint32_t text_col = 0xFFCCCCCC;
    uint32_t prompt_col = COLOR_NEON_BLUE;

    for (int i=0; i<term_hist_count; i++) {
        uint32_t col = text_col;
        // Check for specific messages to colorize
        if (term_history[i][0] == 'S' && term_history[i][1] == 'u' && term_history[i][2] == 'c') {
            col = COLOR_GREEN;
        }
        draw_string_px(term_history[i], x, current_y, col);
        current_y += line_h;
    }

    draw_string_px("user@t-os:~$", x, current_y, prompt_col);
    draw_string_px(term_input, x + 100, current_y, text_col);

    // Blinking cursor
    if ((get_tick_count() / 500) % 2) {
        int input_w = strlen(term_input) * 8;
        draw_rect_px(x + 100 + input_w, current_y + 2, 8, 12, text_col);
    }
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
