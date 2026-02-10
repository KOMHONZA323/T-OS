/*
 * T-OS Minimal Kernel
 * Freestanding C Environment - TUI Implementation
 */

#include "screen.h"
#include "utils.h"

/* Attribute Byte: (Background << 4) | Foreground
 * 0: Black, 1: Blue, 2: Green, 3: Cyan, 4: Red, 5: Magenta, 6: Brown, 7: Light Grey
 * 8: Dark Grey, 9: Light Blue, A: Light Green, B: Light Cyan, C: Light Red, D: Light Magenta, E: Yellow, F: White
 */
#define COLOR_BG        0x0F // White on Black
#define COLOR_BAR       0x1F // White on Blue
#define COLOR_WIN_BG    0x0F // White on Black
#define COLOR_WIN_BORDER 0x07 // Light Grey on Black
#define COLOR_SHADOW    0x08 // Dark Grey on Black (Text) - used with block char, or use 0x70 for background shadow

void draw_desktop();
void draw_top_bar();
void draw_bottom_bar();
void draw_window(int x, int y, int w, int h, const char* title, const char* content);

void kernel_main(void) {
    clear_screen();
    draw_desktop();

    // Position cursor for input (simulated)
    // kprint_at("> ", 2, 22);

    while (1)
        ;
}

void draw_desktop() {
    // 2. Top Bar (Fedora Style)
    draw_top_bar();

    // 3. Bottom Taskbar (Windows Hybrid)
    draw_bottom_bar();

    // 4. Windows
    // Window 1: C Code Editor
    draw_window(3, 3, 38, 14, " kernel.c ",
        "void main() {\n"
        "  // T-OS Kernel\n"
        "  init_video();\n"
        "  kprint(\"Hello\");\n"
        "  while(1);\n"
        "}");

    // Window 2: Terminal
    draw_window(44, 6, 32, 11, " Terminal ",
        "$ make\n"
        "[OK] Kernel built.\n"
        "[OK] Bootloader.\n"
        "$ ./run\n"
        "Starting T-OS..."
    );
}

void draw_top_bar() {
    // Background
    draw_rect(0, 0, 80, 1, COLOR_BAR);

    // Text
    kprint_at_attr(" Activities ", 0, 0, COLOR_BAR); // Left

    // Center Clock (Mock)
    kprint_at_attr(" 12:00 ", 37, 0, COLOR_BAR);

    // Right Icons
    kprint_at_attr(" WF BT PW ", 70, 0, COLOR_BAR);
}

void draw_bottom_bar() {
    // Background
    draw_rect(0, 24, 80, 1, COLOR_BAR);

    // Start Button (Stylized T)
    kprint_at_attr(" [T] ", 0, 24, COLOR_BAR);

    // Center Icons (Mock)
    kprint_at_attr(" [Code] [Term] [File] ", 30, 24, COLOR_BAR);

    // Show Desktop sliver
    kprint_at_attr("||", 78, 24, COLOR_BAR);
}

void draw_window(int x, int y, int w, int h, const char* title, const char* content) {
    // Shadow (Right and Bottom)
    // Using 0x07 (Light Grey) char on Black for simplicity, or 0x08 (Dark Grey)
    // Actually, screen.c draw_rect prints spaces with attribute.
    // If we want a shadow effect, we can use attribute 0x08 (Dark Grey foreground, Black background)
    // But spaces are empty.
    // Let's use attribute 0x70 (Black foreground, Light Grey background) -> Solid Light Grey block
    // Or 0x80 (Black on Dark Grey) -> but bit 7 is blink.
    // Let's stick to 0x08 (Dark Grey on Black) and assume we can't do block shadow easily without changing draw_rect to accept char.
    // I'll skip shadow for now to keep it clean.

    // Window Box
    draw_box(x, y, w, h, COLOR_WIN_BORDER, COLOR_WIN_BG);

    // Title
    kprint_at((char*)title, x + 2, y);

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
            // Ensure we don't overflow window
            if (cx < x + w - 1 && cy < y + h - 1) {
                kprint_at(str, cx, cy);
            }
            cx++;
        }
        p++;
    }
}
