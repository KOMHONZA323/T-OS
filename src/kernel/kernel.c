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

    // Pre-populate terminal with success message for showcase
    term_hist_count = 0;
    term_print("user@t-os:~/projects/kernel$ tasm kernel.asm");
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

    // Editor Window (Left, overlapping top)
    int w1_x = (g_width * 10) / 100;
    int w1_y = (g_height * 15) / 100;
    int w1_w = (g_width * 45) / 100;
    int w1_h = (g_height * 50) / 100;
    draw_window_modern(w1_x, w1_y, w1_w, w1_h, "editor.c - Code");
    draw_c_code_content(w1_x + 10, w1_y + 30);

    // Terminal Window (Right, overlapping bottom)
    int w2_x = (g_width * 40) / 100;
    int w2_y = (g_height * 35) / 100;
    int w2_w = (g_width * 50) / 100;
    int w2_h = (g_height * 45) / 100;
    draw_window_modern(w2_x, w2_y, w2_w, w2_h, "Terminal - ASM");
    draw_terminal_content(w2_x + 10, w2_y + 30);

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
    int height = (g_height < 600) ? 36 : 48; // Thicker semi-transparent frosted glass taskbar
    int y = g_height - height;

    // Frosted Glass Background
    draw_rect_alpha(0, y, g_width, height, COLOR_TASKBAR_BG);

    // Start Button (Stylized T) - Left
    int start_size = height - 10;
    int start_x = 10;
    int start_y = y + 5;

    // Start Button Glow
    draw_rect_alpha(start_x - 2, start_y - 2, start_size + 4, start_size + 4, 0x4000E5FF);
    draw_rect_px(start_x, start_y, start_size, start_size, COLOR_OBSIDIAN); // Button bg

    // Stylized T Logo
    int t_w = start_size / 2;
    int t_h = start_size - 8;
    int t_x = start_x + (start_size - t_w)/2;
    int t_y = start_y + 4;
    draw_rect_px(t_x, t_y, t_w, 4, COLOR_NEON_BLUE); // Top bar
    draw_rect_px(t_x + t_w/2 - 2, t_y, 4, t_h, COLOR_NEON_BLUE); // Vertical bar

    // Centered App Icons (Windows 11 style)
    int icon_size = height - 14;
    int center_x = g_width / 2;
    int spacing = icon_size + 16;
    int num_icons = 4;
    int total_w = num_icons * spacing;
    int start_icons_x = center_x - total_w / 2;

    // Icon 1: Terminal (Active)
    int ix = start_icons_x;
    int iy = y + 7;
    // Active indicator (line at bottom)
    draw_rect_px(ix + icon_size/2 - 10, y + height - 2, 20, 2, COLOR_ICON_ACTIVE);
    // Glow
    draw_rect_alpha(ix - 5, iy - 5, icon_size + 10, icon_size + 10, 0x2000E5FF);
    // Icon Body (Black terminal)
    draw_rect_px(ix, iy, icon_size, icon_size, COLOR_CHARCOAL);
    draw_string_px(">", ix + 6, iy + (icon_size-16)/2, COLOR_GREEN);

    // Icon 2: Editor
    ix += spacing;
    draw_rect_px(ix, iy, icon_size, icon_size, 0xFF2D2D2D); // Dark grey
    draw_string_px("{}", ix + 4, iy + (icon_size-16)/2, COLOR_BLUE);

    // Icon 3: File Manager
    ix += spacing;
    draw_rect_px(ix, iy, icon_size, icon_size, 0xFF2D2D2D);
    draw_rect_px(ix + 4, iy + 8, icon_size - 8, icon_size - 12, COLOR_YELLOW); // Folder part

    // Icon 4: Settings
    ix += spacing;
    draw_rect_px(ix, iy, icon_size, icon_size, 0xFF2D2D2D);
    draw_circle(ix + icon_size/2, iy + icon_size/2, icon_size/3, LIGHT_GREY, 0); // Gear circle simulation

    // Show Desktop Sliver (Far right)
    draw_rect_alpha(g_width - 6, y, 6, height, 0x40FFFFFF);
    draw_line(g_width - 7, y, g_width - 7, g_height, 0x20FFFFFF); // Separator
}

void draw_wallpaper() {
    // Deep charcoal and obsidian abstract geometric wallpaper
    draw_rect_px(0, 0, g_width, g_height, COLOR_OBSIDIAN);

    // Abstract lines
    draw_line(0, 0, g_width, g_height, COLOR_CHARCOAL);
    draw_line(0, g_height, g_width, 0, COLOR_CHARCOAL);

    // Geometric accents (Neon Blue)
    draw_rect_alpha(g_width/3, g_height/4, g_width/3, g_height/2, 0x1000E5FF); // Subtle blue center
    draw_line(g_width/2, 0, g_width/2, g_height, 0x2000E5FF); // Vertical faint line
    draw_line(0, g_height/2, g_width, g_height/2, 0x2000E5FF); // Horizontal faint line

    // Corner accents
    draw_rect_px(g_width - 100, 50, 50, 2, COLOR_NEON_BLUE);
    draw_rect_px(50, g_height - 50, 50, 2, COLOR_NEON_BLUE);

    draw_circle(g_width/2, g_height/2, g_height/3, 0x101E1E1E, 0); // Faint ring
}

void draw_top_bar() {
    int height = (g_height < 600) ? 24 : 28; // Slim translucent black top bar
    draw_rect_alpha(0, 0, g_width, height, COLOR_TOP_BAR_BG);

    int text_y = (height - 16) / 2;
    if (text_y < 0) text_y = 0;

    // "Activities" Button (Left)
    draw_string_px("Activities", 10, text_y, COLOR_TOP_BAR_TEXT);

    // Centered Digital Clock
    char* time = "Oct 25 12:45 PM";
    int time_width = strlen(time) * 8;
    draw_string_px(time, (g_width - time_width)/2, text_y, COLOR_TOP_BAR_TEXT);

    // System Tray Icons (Wi-Fi, Battery, Power) on the far right
    int icon_x = g_width - 25;
    int icon_y = text_y + 2;

    // Power Icon (Circle with line)
    draw_circle(icon_x, icon_y + 6, 6, COLOR_WHITE, 0); // Outline
    draw_line(icon_x, icon_y + 2, icon_x, icon_y + 6, COLOR_WHITE);

    icon_x -= 35;
    // Battery Icon
    draw_rect_px(icon_x, icon_y + 3, 20, 10, COLOR_WHITE); // Body (Outline simulated by filling white then black inner? No draw_rect_px is solid)
    // To simulate outline with draw_rect_px, we need draw_rect (outline).
    // Assuming draw_rect_px is fill. Let's draw fill white then fill black inner.
    draw_rect_px(icon_x + 1, icon_y + 4, 18, 8, COLOR_BLACK); // Inner
    draw_rect_px(icon_x + 2, icon_y + 4, 12, 8, COLOR_WHITE); // Charge level
    draw_rect_px(icon_x + 20, icon_y + 5, 2, 6, COLOR_WHITE); // Nub

    icon_x -= 35;
    // Wi-Fi Icon (Ascending bars)
    draw_rect_px(icon_x, icon_y + 8, 3, 4, COLOR_WHITE);
    draw_rect_px(icon_x + 5, icon_y + 5, 3, 7, COLOR_WHITE);
    draw_rect_px(icon_x + 10, icon_y + 2, 3, 10, COLOR_WHITE);
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

    // Line 1
    draw_string_px("1", x-15, current_y, 0xFF808080);
    draw_string_px("#include", x, current_y, COLOR_MAGENTA);
    draw_string_px("<stdio.h>", x + 70, current_y, 0xFFCE9178);
    current_y += line_h;

    // Line 2
    draw_string_px("2", x-15, current_y, 0xFF808080);
    current_y += line_h;

    // Line 3
    draw_string_px("3", x-15, current_y, 0xFF808080);
    draw_string_px("int", x, current_y, COLOR_BLUE);
    draw_string_px("main", x + 30, current_y, COLOR_YELLOW);
    draw_string_px("() {", x + 65, current_y, COLOR_WHITE);
    current_y += line_h;

    // Line 4
    draw_string_px("4", x-15, current_y, 0xFF808080);
    draw_string_px("  // Kernel Entry Point", x, current_y, 0xFF6A9955); // Green comment
    current_y += line_h;

    // Line 5
    draw_string_px("5", x-15, current_y, 0xFF808080);
    draw_string_px("  printf", x, current_y, COLOR_YELLOW);
    draw_string_px("(", x + 70, current_y, COLOR_WHITE);
    draw_string_px("\"Hello T-OS!\"", x + 80, current_y, 0xFFCE9178);
    draw_string_px(");", x + 200, current_y, COLOR_WHITE);
    current_y += line_h;

    // Line 6
    draw_string_px("6", x-15, current_y, 0xFF808080);
    draw_string_px("  return", x, current_y, COLOR_MAGENTA);
    draw_string_px("0;", x + 70, current_y, 0xFFB5CEA8);
    current_y += line_h;

    // Line 7
    draw_string_px("7", x-15, current_y, 0xFF808080);
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
