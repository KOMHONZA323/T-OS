#include "screen.h"
#include "utils.h"
#include "theme.h"

void draw_interface();
void draw_wallpaper();
void draw_desktop_icons();
void draw_top_bar();
void draw_bottom_bar();
void draw_window(int x, int y, int w, int h, const char* title, const char* content);
void draw_loading_screen(); // Keep from previous step

void kernel_main(void) {
    clear_screen();

    // Simulate Loading Screen
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

    // 2. Layer 1: Desktop Icons
    draw_desktop_icons();

    // 3. Layer 2: Taskbars (Always on top of wallpaper)
    draw_top_bar();
    draw_bottom_bar();

    // 4. Layer 3: Windows (On top of taskbars)
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

void draw_desktop_icons() {
    // Draw Icons (Layer 1)
    // Simple mock icons
    // "My PC"
    kprint_at_attr(" [PC] ", 2, 2, WIN_CONTENT_ATTR);
    kprint_at_attr("My PC ", 2, 3, WIN_CONTENT_ATTR);

    // "Trash"
    kprint_at_attr(" [TR] ", 2, 5, WIN_CONTENT_ATTR);
    kprint_at_attr("Trash ", 2, 6, WIN_CONTENT_ATTR);

    // "Home"
    kprint_at_attr(" [HM] ", 2, 8, WIN_CONTENT_ATTR);
    kprint_at_attr("Home  ", 2, 9, WIN_CONTENT_ATTR);
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
    // Using Light Blue on Black for the Icon area (overwriting block char)
    // Row 1 of Taskbar
    kprint_at_attr(" [T] ", 1, y_start + 1, WIN_ACCENT_ATTR);

    // Centered Apps (Windows 11 style)
    // We need to 'clear' the block chars where the icons are to draw text.
    int center = 30;
    // App 1: [Code] (Active)
    kprint_at_attr(" [Code] ", center, y_start + 1, WIN_ACCENT_ATTR);
    // Active Indicator (Blue Line below)
    // Use 0xDC (Lower Half Block) or '_'
    // Since we want a thin line, use '_' or 0xC4 (Single horizontal line)
    // But '_' is usually low. 0xC4 is centered.
    // Let's try to draw just a blue line using spaces with BLUE background?
    // No, background colors limited to 0-7 (no Blue background, only Blue FG).
    // So we must use a char with Blue FG.
    // 0xDC (Lower half block) with Blue FG -> Bottom half of cell is blue.
    // Or 0xDF (Upper half block) with Blue FG -> Top half of cell is blue.
    // Or 0xC4 (Horizontal Line) with Blue FG -> Middle line.
    // '_' (Underscore) with Blue FG -> Bottom line.
    // Let's use '_' for a subtle glow.
    // We draw it at y_start + 2 (bottom row of taskbar)
    char active_line[] = { '_', '_', '_', '_', '_', '_', '_', '_', 0 };
    kprint_at_attr(active_line, center, y_start + 2, WIN_ACCENT_ATTR);

    // App 2: [Term]
    kprint_at_attr(" [Term] ", center + 10, y_start + 1, WIN_CONTENT_ATTR);

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
