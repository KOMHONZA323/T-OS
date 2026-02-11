#include "../drivers/screen.h"
#include "../drivers/utils.h"
#include "theme.h"

void draw_wallpaper() {
    fill_rect(0, 0, MAX_COLS, MAX_ROWS, WALLPAPER_ATTR, WALLPAPER_CHAR);
}

void draw_top_bar() {
    // Background
    fill_rect(0, 0, MAX_COLS, TOP_BAR_HEIGHT, TOP_BAR_ATTR, ' ');
    // Activities
    print_at("Activities", 0, 1, TOP_BAR_ATTR);
    // Clock
    print_centered("12:00 PM", 0, TOP_BAR_CLOCK_ATTR);
    // Tray
    print_right_aligned("[W] [B] [P] ", 0, TOP_BAR_ATTR);
}

void draw_taskbar() {
    int row = TASKBAR_ROW;
    // Background
    fill_rect(row, 0, MAX_COLS, TASKBAR_HEIGHT, TASKBAR_ATTR, ' ');

    // Start Button " [ T ] "
    // Using Cyan BG for the button to make it pop
    int start_btn_width = 7;
    fill_rect(row, 0, start_btn_width, 1, START_BUTTON_ATTR, ' ');
    print_at("[ T ]", row, 1, START_BUTTON_ATTR);

    // App Icons (Centered)
    const char* apps = "[ Code ]  [ Term ]  [ File ]";
    // We want to highlight the "Term" app as active maybe? Or just show them.
    // Let's just print them centered for now.
    int app_len = strlen(apps);
    int start_col = (MAX_COLS - app_len) / 2;
    print_at(apps, row, start_col, ACTIVE_APP_ATTR);

    // Show Desktop sliver
    print_at("|", row, MAX_COLS - 1, TASKBAR_ATTR);
}

void draw_window(const char* title, int x, int y, int w, int h) {
    // Shadow (simple chars to right and bottom)
    // Draw shadow offset by 1 char right and 1 char down
    // Use Dark Grey block 178 '▓'
    fill_rect(y + 1, x + 1, w, h, (COLOR_DARK_GREY | (COLOR_BLACK << 4)), 178);

    // Window Body
    fill_rect(y, x, w, h, WINDOW_ATTR, ' ');

    // Border
    draw_box(y, x, w, h, WINDOW_BORDER_ATTR);

    // Title Bar
    // Draw a bar at the top inside the border
    fill_rect(y, x, w, 1, WINDOW_TITLE_ATTR, ' '); // Overwrite top border line
    // We need to redraw the top corners though if we want them rounded,
    // or just accept the square title bar.
    // Let's redraw the top border line with the title attribute but use box chars
    // Top Left Corner
    put_char(0xC9, y, x, WINDOW_TITLE_ATTR);
    // Top Right Corner
    put_char(0xBB, y, x + w - 1, WINDOW_TITLE_ATTR);
    // Top Line
    for (int i = 1; i < w - 1; i++) {
        put_char(0xCD, y, x + i, WINDOW_TITLE_ATTR);
    }

    // Title Text
    int title_len = strlen(title);
    int title_col = x + (w - title_len) / 2;
    print_at(title, y, title_col, WINDOW_TITLE_ATTR);

    // Window Controls [X]
    print_at("[X]", y, x + w - 4, WINDOW_TITLE_ATTR);
}

void kernel_main() {
    clear_screen();

    // Draw Desktop Environment
    draw_wallpaper();
    draw_top_bar();
    draw_taskbar();

    // Window 1: C Code Editor
    // Position: Col 4, Row 3, Width 40, Height 14
    draw_window("main.c - T-Edit", 4, 3, 40, 14);
    // Content
    // Offset inside window: y+2 (below title/border), x+2 (padding)
    int w1_y = 3 + 2;
    int w1_x = 4 + 2;
    // Text colors: Keywords White, content Grey
    print_at("#include <stdio.h>", w1_y, w1_x, WINDOW_ATTR);
    print_at("", w1_y+1, w1_x, WINDOW_ATTR);
    print_at("int main(int argc, char** argv) {", w1_y+2, w1_x, WINDOW_ATTR);
    print_at("  // Welcome to T-OS", w1_y+3, w1_x + 2, (COLOR_GREEN | (COLOR_BLACK << 4)));
    print_at("  printf(\"Hello World!\\n\");", w1_y+4, w1_x + 2, WINDOW_ATTR);
    print_at("  return 0;", w1_y+5, w1_x + 2, WINDOW_ATTR);
    print_at("}", w1_y+6, w1_x, WINDOW_ATTR);

    // Window 2: Terminal
    // Position: Col 30, Row 10, Width 45, Height 10
    // Overlaps the first window slightly for depth
    draw_window("Terminal", 30, 10, 45, 10);
    int w2_y = 10 + 2;
    int w2_x = 30 + 2;
    print_at("root@t-os:~$ nasm -f bin boot.asm -o boot.bin", w2_y, w2_x, WINDOW_ATTR);
    print_at("Compiling...", w2_y+1, w2_x, WINDOW_ATTR);
    print_at("Success: 512 bytes written.", w2_y+2, w2_x, (COLOR_LIGHT_GREEN | (COLOR_BLACK << 4)));
    print_at("root@t-os:~$ _", w2_y+3, w2_x, WINDOW_ATTR);

    while(1);
}
