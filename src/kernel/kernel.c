#include "screen.h"
#include "utils.h"
#include "theme.h"

void draw_interface();
void draw_wallpaper();
void draw_top_bar();
void draw_bottom_bar();
void draw_window(int x, int y, int w, int h, const char* title, const char* content);
void draw_loading_screen(); // Keep from previous step

void kernel_main(void) {
    clear_screen();

    // Simulate Loading Screen (as per previous step)
    draw_loading_screen();
    clear_screen();

    // Main GUI Loop (Mock)
    while (1) {
        draw_interface();

        // Just halt here to save CPU
        __asm__("hlt");
    }
}

void draw_interface() {
    // 1. Layer 0: Wallpaper
    draw_wallpaper();

    // 2. Layer 2: Taskbars (Always on top of wallpaper)
    draw_top_bar();
    draw_bottom_bar();

    // 3. Layer 3: Windows (On top of taskbars)
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

void draw_wallpaper() {
    // Fill with 'Deep Charcoal' pattern (Light Shade 0xB0 with Dark Grey)
    draw_fill(0, 0, MAX_COLS, MAX_ROWS, WALLPAPER_CHAR, WALLPAPER_ATTR);
}

void draw_top_bar() {
    // Fedora Style: Slim (1 Row)
    // Translucent Black -> Solid Black Background
    draw_fill(0, 0, MAX_COLS, 1, ' ', TOP_BAR_BG);

    // Left: "Activities" (White)
    kprint_at_attr(" Activities ", 1, 0, TOP_BAR_TEXT_ATTR);

    // Center: Clock (Blue Highlight)
    kprint_at_attr(" Oct 25 12:00 ", 35, 0, WIN_ACCENT_ATTR);

    // Right: Status Icons (Mock)
    kprint_at_attr(" WF BT PW ", 70, 0, TOP_BAR_TEXT_ATTR);
}

void draw_bottom_bar() {
    // Windows Hybrid: Thicker (3 Rows = ~48px)
    int y_start = MAX_ROWS - 3;

    // Background: 'Frosted Glass' (Simulated with Dark Grey block pattern or just Black)
    // Let's use Block Character 0xDB with Dark Grey FG to simulate a solid Dark Grey bar
    draw_fill(0, y_start, MAX_COLS, 3, TASKBAR_CHAR, TASKBAR_ATTR);

    // Start Button: Stylized T (Row 23)
    // Using Light Blue on Dark Grey
    // Since we are using Block Chars for BG, drawing text over it requires changing the char.
    // We replace the block char with the 'T' char, but we are stuck with Black BG.
    // So: Black 'T' on Light Blue BG? Or Light Blue 'T' on Black BG?
    // Let's use Light Blue on Black for the Icon area.
    kprint_at_attr(" [T] ", 1, y_start + 1, WIN_ACCENT_ATTR);

    // Centered Apps (Windows 11 style)
    // We need to 'clear' the block chars where the icons are to draw text.
    int center = 30;
    kprint_at_attr(" [Code] ", center, y_start + 1, WIN_CONTENT_ATTR);
    kprint_at_attr(" [Term] ", center + 8, y_start + 1, WIN_CONTENT_ATTR);

    // Active App Indicator (Glowing Blue Line under Code)
    // Underline? VGA attr doesn't support underline easily without monochrome.
    // We can use a different char or color.
    // Let's use a cyan 'period' or 'underscore' below it? No space.
    // Let's make the active app text Blue.
    kprint_at_attr(" [Code] ", center, y_start + 1, WIN_ACCENT_ATTR);

    // System Tray (Right)
    kprint_at_attr(" ^  ENG  12:00 ", 65, y_start + 1, WIN_CONTENT_ATTR);
}

void draw_window(int x, int y, int w, int h, const char* title, const char* content) {
    // Draw Frame (Rounded Corners, Deep Charcoal Border)
    draw_box_rounded(x, y, w, h, WIN_BORDER_ATTR, WIN_CONTENT_ATTR, WIN_TITLE_ATTR);

    // Draw Title (Centered or Left)
    // Mock "Traffic Lights" or Buttons: [X] [-]
    kprint_at_attr(" X ", x + w - 4, y, WIN_ACCENT_ATTR); // Close button
    kprint_at_attr( (char*)title, x + 2, y, WIN_TITLE_ATTR);

    // Draw Content
    int cx = x + 2;
    int cy = y + 2; // Start below title/border

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

// Re-implement loading screen to use new constants if needed, or keep as is.
// Using existing implementation from previous step but ensuring it compiles.
// Note: previous implementation used constants like COLOR_BAR which are removed.
// We must update it.
void draw_loading_screen() {
    int center_col = 30;
    int center_row = 10;

    // Draw Logo
    kprint_at_attr("       T-OS       ", center_col, center_row, WIN_ACCENT_ATTR);
    kprint_at_attr(" System Loading...", center_col, center_row + 2, WIN_CONTENT_ATTR);

    // Draw Progress Bar Frame
    int bar_width = 20;
    int bar_col = center_col;
    int bar_row = center_row + 4;

    kprint_at_attr("[", bar_col - 1, bar_row, WIN_CONTENT_ATTR);
    kprint_at_attr("]", bar_col + bar_width, bar_row, WIN_CONTENT_ATTR);

    // Animate
    for (int i = 0; i < bar_width; i++) {
        char progress[2] = { '=', 0 };
        kprint_at_attr(progress, bar_col + i, bar_row, WIN_ACCENT_ATTR);
        delay(3000);
    }

    delay(5000);
}
