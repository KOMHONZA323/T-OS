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
    term_print("T-OS Terminal v1.0");
    term_print("Type 'help' for commands.");

    // Initial simulated history for showcase
    term_print("user@t-os:~/src$ tasm kernel.asm");
    term_print("Assembling kernel.asm...");
    term_print("[OK] output: kernel.o");
    term_print("user@t-os:~/src$ _");

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

        // Mouse interaction for taskbar icons could go here

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

    // C Code Editor Window (Left)
    int c_x = (g_width * 5) / 100;
    int c_y = (g_height * 10) / 100;
    int c_w = (g_width * 40) / 100;
    int c_h = (g_height * 60) / 100;
    draw_window_modern(c_x, c_y, c_w, c_h, "C Code Editor");
    draw_c_code_content(c_x + 10, c_y + 30);

    // Terminal Window (Right/Overlapping)
    int t_x = (g_width * 45) / 100;
    int t_y = (g_height * 20) / 100;
    int t_w = (g_width * 45) / 100;
    int t_h = (g_height * 50) / 100;
    draw_window_modern(t_x, t_y, t_w, t_h, "Terminal");
    draw_terminal_content(t_x + 10, t_y + 30);

    draw_top_bar();
    draw_bottom_bar();
}

void draw_terminal_content(int x, int y) {
    int line_h = 16;
    int current_y = y;
    uint32_t text_col = COLOR_WHITE;
    uint32_t prompt_col = COLOR_NEON_BLUE;
    uint32_t success_col = COLOR_SYNTAX_COMMENT; // Green

    // Custom layout for showcase
    draw_string_px("user@t-os:~/src$ ", x, current_y, prompt_col);
    draw_string_px("tasm kernel.asm", x + 136, current_y, text_col);
    current_y += line_h;

    draw_string_px("Assembling kernel.asm...", x, current_y, text_col);
    current_y += line_h;

    draw_string_px("[OK]", x, current_y, success_col);
    draw_string_px(" output: kernel.o", x + 40, current_y, text_col);
    current_y += line_h;

    draw_string_px("user@t-os:~/src$ ", x, current_y, prompt_col);
    // Blinking cursor
    if ((get_tick_count() / 500) % 2) {
        draw_rect_px(x + 136, current_y + 2, 8, 12, text_col);
    }
}

void draw_bottom_bar() {
    int height = 48;
    int y = g_height - height;

    // Frosted Glass Background
    draw_rect_alpha(0, y, g_width, height, COLOR_TASKBAR_BG);

    // Stylized 'T' Start Button
    int start_x = 10;
    int start_y = y + 10;
    // Vertical bar
    draw_rect_px(start_x + 10, start_y + 4, 8, 20, COLOR_NEON_BLUE);
    // Horizontal bar (Top)
    draw_rect_px(start_x + 2, start_y, 24, 6, COLOR_NEON_BLUE);
    // Accent
    draw_rect_px(start_x + 12, start_y + 8, 4, 12, 0xFFFFFFFF);

    // Centered Icons
    int center_x = g_width / 2;
    int icon_size = 32;
    int spacing = 16;
    int total_width = (icon_size * 3) + (spacing * 2);
    int start_icons_x = center_x - (total_width / 2);

    // Icon 1: C Code (Glow effect behind it)
    int i1_x = start_icons_x;
    draw_rect_alpha(i1_x - 4, y + 4, icon_size + 8, icon_size + 8, 0x4000E5FF); // Glow
    draw_rounded_rect(i1_x, y + 8, icon_size, icon_size, 4, COLOR_CHARCOAL); // Base
    draw_string_px("< >", i1_x + 4, y + 16, COLOR_NEON_BLUE);
    // Active indicator line
    draw_rect_px(i1_x + 8, y + height - 2, icon_size - 16, 2, COLOR_NEON_BLUE);

    // Icon 2: Terminal
    int i2_x = start_icons_x + icon_size + spacing;
    draw_rounded_rect(i2_x, y + 8, icon_size, icon_size, 4, COLOR_BLACK);
    draw_string_px(">_", i2_x + 8, y + 16, COLOR_WHITE);

    // Icon 3: Settings
    int i3_x = i2_x + icon_size + spacing;
    draw_rounded_rect(i3_x, y + 8, icon_size, icon_size, 4, COLOR_GRAY);
    draw_circle(i3_x + 16, y + 24, 8, COLOR_WHITE, 0);

    // Show Desktop Sliver
    draw_rect_alpha(g_width - 6, y, 6, height, 0x40FFFFFF);
}

void draw_wallpaper() {
    // Obsidian Background
    draw_rect_px(0, 0, g_width, g_height, COLOR_OBSIDIAN);

    // Abstract Geometric Accents (Neon Blue with Alpha)
    uint32_t accent_col = 0x2000E5FF; // Low alpha neon blue

    // Draw some large triangles/lines
    draw_line(0, g_height, g_width, 0, accent_col);
    draw_line(0, 0, g_width, g_height, accent_col);

    draw_line(g_width/3, 0, g_width, g_height/2, accent_col);
    draw_line(0, g_height/2, g_width/3, g_height, accent_col);

    // Some filled shapes (simulated with alpha rects for now as we lack fill_poly)
    draw_rect_alpha(g_width/4, g_height/4, 200, 200, 0x1000E5FF);
    draw_rect_alpha(g_width/2, g_height/2, 300, 100, 0x1000E5FF);

    // Circle accent
    draw_circle(g_width - 100, 100, 150, 0x1500E5FF, 0);
}

void draw_top_bar() {
    int height = 28;
    draw_rect_alpha(0, 0, g_width, height, COLOR_TOP_BAR_BG);

    int text_y = (height - 16) / 2;
    if (text_y < 0) text_y = 0;

    // "Activities" Button
    draw_string_px("Activities", 10, text_y, COLOR_TOP_BAR_TEXT);

    // Centered Clock
    char* time = "Oct 25 12:45 PM";
    int time_width = strlen(time) * 8;
    draw_string_px(time, (g_width - time_width)/2, text_y, COLOR_TOP_BAR_TEXT);

    // System Tray
    char* tray = "WF 100% PWR";
    int tray_width = strlen(tray) * 8;
    draw_string_px(tray, g_width - tray_width - 10, text_y, COLOR_TOP_BAR_TEXT);
}

void draw_window_modern(int x, int y, int w, int h, const char* title) {
    // Drop Shadow
    draw_rect_alpha(x + 5, y + 5, w, h, 0x60000000);

    // Window Body
    draw_rounded_rect(x, y, w, h, 8, COLOR_CHARCOAL); // Background
    // Header
    // draw_rounded_rect header area? Or just text.
    // Let's draw a subtle header line
    draw_line(x, y + 24, x + w, y + 24, 0xFF333333);

    draw_string_px(title, x + 10, y + 5, COLOR_WHITE);

    // Traffic Lights (Controls)
    int radius = 5;
    int spacing = 14;
    int controls_x = x + w - 15;
    int controls_y = y + 12;

    draw_circle(controls_x, controls_y, radius, COLOR_RED, 1); // Close
    draw_circle(controls_x - spacing, controls_y, radius, COLOR_YELLOW, 1); // Minimize
    draw_circle(controls_x - spacing*2, controls_y, radius, COLOR_GREEN, 1); // Maximize
}

void draw_c_code_content(int x, int y) {
    int line_h = 16;
    int current_y = y;
    if (current_y + 6*line_h > g_height) return;

    // Line 1: #include <stdio.h>
    draw_string_px("#include", x, current_y, COLOR_SYNTAX_PREPROC);
    draw_string_px("<stdio.h>", x + 72, current_y, COLOR_SYNTAX_STRING);
    current_y += line_h;

    // Line 2: // T-OS Kernel Entry
    draw_string_px("// T-OS Kernel Entry", x, current_y, COLOR_SYNTAX_COMMENT);
    current_y += line_h;

    // Line 3: int main() {
    draw_string_px("int", x, current_y, COLOR_SYNTAX_TYPE);
    draw_string_px("main", x + 32, current_y, COLOR_SYNTAX_FUNC);
    draw_string_px("() {", x + 72, current_y, COLOR_WHITE);
    current_y += line_h;

    // Line 4:   printf("Welcome...");
    draw_string_px("  printf", x, current_y, COLOR_SYNTAX_FUNC);
    draw_string_px("(", x + 64, current_y, COLOR_WHITE);
    draw_string_px("\"Welcome to T-OS\\n\"", x + 72, current_y, COLOR_SYNTAX_STRING);
    draw_string_px(");", x + 228, current_y, COLOR_WHITE);
    current_y += line_h;

    // Line 5:   return 0;
    draw_string_px("  return", x, current_y, COLOR_SYNTAX_KEYWORD);
    draw_string_px(" 0", x + 64, current_y, COLOR_SYNTAX_NUMBER);
    draw_string_px(";", x + 80, current_y, COLOR_WHITE);
    current_y += line_h;

    // Line 6: }
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
        delay(50);
    }
    delay(1000);
}
