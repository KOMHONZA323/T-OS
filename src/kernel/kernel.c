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

void draw_icon_wifi(int x, int y, uint32_t color) {
    // 3 ascending bars
    draw_rect_px(x, y + 8, 2, 4, color);
    draw_rect_px(x + 4, y + 5, 2, 7, color);
    draw_rect_px(x + 8, y + 2, 2, 10, color);
}

void draw_icon_battery(int x, int y, uint32_t color) {
    draw_rect_px(x, y + 4, 16, 8, color); // Body outline
    draw_rect_px(x + 2, y + 6, 12, 4, 0xFF00FF00); // Level Green
    draw_rect_px(x + 16, y + 6, 2, 4, color); // Tip
}

void draw_icon_power(int x, int y, uint32_t color) {
    draw_circle(x + 6, y + 6, 6, color, 0); // Ring
    draw_rect_px(x + 5, y, 2, 6, color); // Line at top
}

void draw_icon_code(int x, int y, int size) {
    // {} icon
    int cx = x + size/2;
    int cy = y + size/2;
    // {
    draw_line(cx - 8, cy - 6, cx - 12, cy, 0xFFFFFFFF);
    draw_line(cx - 12, cy, cx - 8, cy + 6, 0xFFFFFFFF);
    // }
    draw_line(cx + 8, cy - 6, cx + 12, cy, 0xFFFFFFFF);
    draw_line(cx + 12, cy, cx + 8, cy + 6, 0xFFFFFFFF);
    // /
    draw_line(cx + 4, cy - 8, cx - 4, cy + 8, 0xFF00E5FF);
}

void draw_icon_terminal(int x, int y, int size) {
    // >_ icon
    int cx = x + size/2;
    int cy = y + size/2;
    draw_string_px(">_", cx - 8, cy - 8, 0xFFFFFFFF);
}

void draw_icon_settings(int x, int y, int size) {
    // Gear icon (simplified circle with teeth)
    int cx = x + size/2;
    int cy = y + size/2;
    draw_circle(cx, cy, 8, 0xFFFFFFFF, 0);
    draw_circle(cx, cy, 3, 0xFFFFFFFF, 1);
    // Teeth (4 lines)
    draw_line(cx, cy - 10, cx, cy + 10, 0xFFFFFFFF);
    draw_line(cx - 10, cy, cx + 10, cy, 0xFFFFFFFF);
}

void draw_interface() {
    draw_wallpaper();

    // Window 1: C Code Editor (Left)
    int code_x = (g_width * 5) / 100;
    int code_y = (g_height * 15) / 100;
    int code_w = (g_width * 42) / 100;
    int code_h = (g_height * 60) / 100;

    draw_window_modern(code_x, code_y, code_w, code_h, "main.c - T-OS Code");
    draw_c_code_content(code_x + 10, code_y + 35);

    // Window 2: Terminal (Right)
    int term_x = (g_width * 53) / 100;
    int term_y = (g_height * 15) / 100;
    int term_w = (g_width * 42) / 100;
    int term_h = (g_height * 60) / 100;

    draw_window_modern(term_x, term_y, term_w, term_h, "Terminal");
    draw_terminal_content(term_x + 10, term_y + 35);

    draw_top_bar();
    draw_bottom_bar();
}

void draw_terminal_content(int x, int y) {
    int line_h = 16;
    int current_y = y;
    uint32_t text_col = 0xFFCCCCCC;
    uint32_t prompt_col = COLOR_NEON_BLUE;

    // Showcase Content (Simulate Compilation Output)
    draw_string_px("user@t-os:~$ tasm kernel.asm", x, current_y, text_col);
    current_y += line_h;

    draw_string_px("Assembling 'kernel.asm'...", x, current_y, 0xFF888888);
    current_y += line_h;

    draw_string_px("[SUCCESS] Output: kernel.texf (2456 bytes)", x, current_y, 0xFF00FF00); // Green
    current_y += line_h;

    draw_string_px("          0 Errors, 0 Warnings", x, current_y, 0xFF00FF00); // Green
    current_y += line_h;
    current_y += line_h; // Spacing

    // Current Prompt
    draw_string_px("user@t-os:~$", x, current_y, prompt_col);
    // draw_string_px(term_input, x + 100, current_y, text_col); // Use term_input if desired, but for showcase we leave it empty.

    if ((get_tick_count() / 500) % 2) {
        // int input_w = strlen(term_input) * 8;
        draw_rect_px(x + 100, current_y + 2, 8, 12, text_col); // Cursor at prompt
    }
}

void draw_bottom_bar() {
    int height = 48; // Fixed taller height
    int y = g_height - height;

    // Background: Semi-transparent frosted glass
    draw_rect_alpha(0, y, g_width, height, COLOR_TASKBAR_BG);

    // Start Button: Stylized T (Geometric, Neon Blue)
    int start_x = 10;
    int start_y = y + (height - 24) / 2;
    // Horizontal bar
    draw_rect_px(start_x, start_y, 24, 6, COLOR_NEON_BLUE);
    // Vertical bar
    draw_rect_px(start_x + 9, start_y + 6, 6, 18, COLOR_NEON_BLUE);
    // Accent line inside vertical bar
    draw_rect_px(start_x + 11, start_y + 6, 2, 18, 0xFFFFFFFF);

    // Centered App Icons with Glow
    int icon_size = 32;
    int spacing = 20;
    int icon_y = y + (height - icon_size) / 2;

    // Calculate starting X for centered icons
    int total_width = (3 * icon_size) + (2 * spacing);
    int current_x = (g_width - total_width) / 2;

    // 1. Terminal Icon
    draw_circle(current_x + icon_size/2, icon_y + icon_size/2, icon_size/2 + 4, COLOR_ICON_GLOW, 1); // Glow
    draw_rect_px(current_x, icon_y, icon_size, icon_size, 0xFF2D2D2D); // Icon BG
    draw_icon_terminal(current_x, icon_y, icon_size);
    // Active Indicator
    draw_rect_px(current_x + 4, g_height - 2, icon_size - 8, 2, COLOR_NEON_BLUE);
    current_x += icon_size + spacing;

    // 2. Code Icon
    draw_circle(current_x + icon_size/2, icon_y + icon_size/2, icon_size/2 + 4, COLOR_ICON_GLOW, 1);
    draw_rect_px(current_x, icon_y, icon_size, icon_size, 0xFF2D2D2D);
    draw_icon_code(current_x, icon_y, icon_size);
    // Active Indicator
    draw_rect_px(current_x + 4, g_height - 2, icon_size - 8, 2, COLOR_NEON_BLUE);
    current_x += icon_size + spacing;

    // 3. Settings Icon
    draw_circle(current_x + icon_size/2, icon_y + icon_size/2, icon_size/2 + 4, COLOR_ICON_GLOW, 1);
    draw_rect_px(current_x, icon_y, icon_size, icon_size, 0xFF2D2D2D);
    draw_icon_settings(current_x, icon_y, icon_size);

    // Show Desktop Sliver
    draw_rect_px(g_width - 6, y, 6, height, 0x40FFFFFF);
    draw_line(g_width - 7, y, g_width - 7, g_height, 0x20000000); // Separator
}

void draw_wallpaper() {
    // Deep Charcoal/Obsidian background
    draw_rect_px(0, 0, g_width, g_height, COLOR_OBSIDIAN);

    // Abstract Geometric Shapes (Subtle Lines and Alpha Rects)
    draw_line(0, g_height, g_width, 0, COLOR_CHARCOAL);
    draw_line(0, 0, g_width, g_height, COLOR_CHARCOAL);

    // Create depth with large alpha blended regions
    // Right half lighter
    draw_rect_alpha(g_width/2, 0, g_width/2, g_height, 0x05FFFFFF);
    // Bottom half darker
    draw_rect_alpha(0, g_height/2, g_width, g_height/2, 0x05000000);

    // Neon accents (Very low alpha blue box in center)
    draw_rect_alpha(g_width/4, g_height/4, g_width/2, g_height/2, 0x0800E5FF);

    // Geometric Circle Accents
    draw_circle(g_width/2, g_height/2, g_height/3, 0x201E1E1E, 0); // Dark ring
    draw_circle(g_width/2, g_height/2, g_height/3 + 2, 0x1000E5FF, 0); // Subtle blueish ring
}

void draw_top_bar() {
    int height = (g_height < 600) ? 20 : 30;
    draw_rect_alpha(0, 0, g_width, height, COLOR_TOP_BAR_BG);

    int text_y = (height - 16) / 2;
    if (text_y < 0) text_y = 0;

    // Left: Activities
    draw_string_px("Activities", 10, text_y, COLOR_TOP_BAR_TEXT);

    // Center: Clock
    char* time = "Oct 25 12:45 PM";
    int time_width = strlen(time) * 8;
    draw_string_px(time, (g_width - time_width)/2, text_y, COLOR_TOP_BAR_TEXT);

    // Right: Icons (Power, Battery, WiFi)
    int icon_base_x = g_width - 20;

    // Power
    draw_icon_power(icon_base_x - 12, text_y, COLOR_WIFI);
    icon_base_x -= 30;

    // Battery
    draw_icon_battery(icon_base_x - 16, text_y, COLOR_BATTERY);
    icon_base_x -= 30;

    // WiFi
    draw_icon_wifi(icon_base_x - 12, text_y, COLOR_WIFI);
}

void draw_window_modern(int x, int y, int w, int h, const char* title) {
    // Drop Shadow (Offset)
    draw_rect_alpha(x + 8, y + 8, w, h, COLOR_SHADOW);

    // Window Body (Rounded)
    draw_rounded_rect(x, y, w, h, 8, COLOR_CHARCOAL);

    // Header separation line (optional, subtle)
    // draw_line(x, y + 30, x + w, y + 30, 0xFF2D2D2D);

    // Title
    draw_string_px(title, x + 12, y + 8, COLOR_WHITE);

    // Window Controls (Traffic Lights)
    int radius = 5;
    int spacing = 14;
    int controls_x = x + w - 18;
    int controls_y = y + 12;

    draw_circle(controls_x, controls_y, radius, COLOR_RED, 1);
    draw_circle(controls_x - spacing, controls_y, radius, COLOR_YELLOW, 1);
    draw_circle(controls_x - spacing*2, controls_y, radius, COLOR_GREEN, 1);
}

void draw_c_code_content(int x, int y) {
    int line_h = 16;
    int current_y = y;

    // Syntax Highlighted C Code
    draw_string_px("#include", x, current_y, COLOR_MAGENTA);
    draw_string_px("<stdio.h>", x + 72, current_y, 0xFFCE9178);
    current_y += line_h;

    draw_string_px("#include", x, current_y, COLOR_MAGENTA);
    draw_string_px("<stdlib.h>", x + 72, current_y, 0xFFCE9178);
    current_y += line_h * 2;

    draw_string_px("int", x, current_y, COLOR_BLUE);
    draw_string_px("main", x + 32, current_y, COLOR_YELLOW);
    draw_string_px("(int argc, char** argv) {", x + 72, current_y, COLOR_WHITE);
    current_y += line_h;

    draw_string_px("  printf", x, current_y, COLOR_YELLOW);
    draw_string_px("(", x + 56, current_y, COLOR_WHITE);
    draw_string_px("\"Welcome to T-OS!\\n\"", x + 64, current_y, 0xFFCE9178);
    draw_string_px(");", x + 230, current_y, COLOR_WHITE);
    current_y += line_h;

    draw_string_px("  return", x, current_y, COLOR_MAGENTA);
    draw_string_px("0;", x + 56, current_y, 0xFFB5CEA8);
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
