#include "compositor.h"
#include "gui/font_renderer.h"
#include "gui/mouse_event.h"

// Basic assumptions requested by prompt
extern void* kmalloc(uint64_t size);

static GOP_Info g_info;
static uint32_t* double_buffer = NULL;
static UIState ui_state;

// Phase 4: Window Manager Integration
void compositor_draw_cursor(void) {
    uint32_t x = ui_state.mouse_x;
    uint32_t y = ui_state.mouse_y;
    uint32_t color = 0xFFFFFF; // White cursor

    // Simple arrow cursor
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j <= i; j++) {
            compositor_put_pixel(x + j, y + i, color);
        }
    }
}

void compositor_update_mouse(void) {
    MousePacket mp;
    while (mouse_queue_pop(&mp)) {
        ui_state.mouse_x += mp.dx;
        ui_state.mouse_y -= mp.dy; // PS/2 Y is inverted

        // Phase 4, Req 3: Clamp to screen resolution
        if (ui_state.mouse_x < 0) ui_state.mouse_x = 0;
        if (ui_state.mouse_y < 0) ui_state.mouse_y = 0;
        if (ui_state.mouse_x >= (int32_t)g_info.fb_width) ui_state.mouse_x = g_info.fb_width - 1;
        if (ui_state.mouse_y >= (int32_t)g_info.fb_height) ui_state.mouse_y = g_info.fb_height - 1;

        ui_state.mouse_buttons = (mp.left_button ? 1 : 0) | (mp.right_button ? 2 : 0) | (mp.middle_button ? 4 : 0);

        // Phase 4, Req 4: Trigger Click Event on transition 0 -> 1
        if ((ui_state.mouse_buttons & 1) && !(ui_state.last_buttons & 1)) {
            // Left click event
            compositor_toggle_theme();
        }
        ui_state.last_buttons = ui_state.mouse_buttons;
    }
}

// Phase 1 Functions
void compositor_init(GOP_Info* info) {
    g_info = *info;
    
    // Allocate double buffer
    uint64_t buffer_size = (uint64_t)info->fb_pitch * info->fb_height * 4;
    double_buffer = (uint32_t*)kmalloc(buffer_size);

    ui_state.current_theme = THEME_FEDORA_DARK;
    ui_state.term_w = info->fb_width / 2;
    ui_state.term_h = info->fb_height / 2;
    ui_state.term_x = (info->fb_width - ui_state.term_w) / 2;
    ui_state.term_y = (info->fb_height - ui_state.term_h) / 2;
    ui_state.cur_x = 0;
    ui_state.cur_y = 0;
    ui_state.mouse_x = info->fb_width / 2;
    ui_state.mouse_y = info->fb_height / 2;
    ui_state.mouse_buttons = 0;
    ui_state.last_buttons = 0;

    compositor_draw_desktop();
}

void compositor_put_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (x >= g_info.fb_width || y >= g_info.fb_height || !double_buffer) return;
    double_buffer[y * g_info.fb_pitch + x] = color;
}

void compositor_draw_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color) {
    for (uint32_t dy = 0; dy < h; dy++) {
        for (uint32_t dx = 0; dx < w; dx++) {
            compositor_put_pixel(x + dx, y + dy, color);
        }
    }
}

static void draw_char_local(char c, uint32_t x, uint32_t y, uint32_t color) {
    // We use font_draw_char but it expects a BootInfo. 
    // We can simulate it or just use the logic directly.
    // Since compositor has its own double buffer, we need a modified version or just keep it simple.
    // For now, let's keep it but use the centralized font.
    BootInfo bi;
    bi.fb_base = (uint64_t)double_buffer;
    bi.fb_width = g_info.fb_width;
    bi.fb_height = g_info.fb_height;
    bi.fb_pitch = g_info.fb_pitch;
    font_draw_char(&bi, c, x, y, color, 1);
}

void compositor_draw_string(const char* str, uint32_t x, uint32_t y, uint32_t color) {
    while (*str) {
        draw_char_local(*str, x, y, color);
        x += 8;
        str++;
    }
}


// Inline ASM optimizations for swapping
void compositor_swap_buffers(void) {
    if (!double_buffer || !g_info.fb_base) return;
    
    uint32_t* fb = (uint32_t*)g_info.fb_base;
    uint64_t dwords = (uint64_t)g_info.fb_pitch * g_info.fb_height;

    // Using REP MOVSQ for high-speed block transfer.
    // The source and destination operands must be locals: rep movsq advances
    // RSI/RDI, and a "+S" constraint on double_buffer itself would write the
    // post-copy pointer back into the global, leaving it dangling one frame
    // past the buffer from the second swap onwards.
    const uint32_t* src = double_buffer;
    uint32_t* dst = fb;
    uint64_t qwords = dwords / 2;
    uint32_t rem = dwords % 2;

    __asm__ volatile (
        "cld\n\t"
        "rep movsq\n\t"
        : "+S"(src), "+D"(dst), "+c"(qwords)
        :
        : "memory"
    );

    if (rem) {
        fb[dwords - 1] = double_buffer[dwords - 1];
    }
}

void compositor_scroll_terminal(void) {
    uint32_t term_bg = (ui_state.current_theme == THEME_FEDORA_DARK) ? 0x1E1E1E : 0x000000;
    uint32_t start_y = ui_state.term_y + 32; 
    uint32_t end_y = ui_state.term_y + ui_state.term_h;
    uint32_t scroll_lines = 8;
    
    // Shift pixels up
    for (uint32_t y = start_y; y < end_y - scroll_lines; y++) {
        for (uint32_t x = ui_state.term_x; x < ui_state.term_x + ui_state.term_w; x++) {
            double_buffer[y * g_info.fb_pitch + x] = double_buffer[(y + scroll_lines) * g_info.fb_pitch + x];
        }
    }
    // Clear bottom line
    compositor_draw_rect(ui_state.term_x, end_y - scroll_lines, ui_state.term_w, scroll_lines, term_bg);
}

void compositor_print(const char* str, uint32_t color) {
    while (*str) {
        if (*str == '\n') {
            ui_state.cur_x = 0;
            ui_state.cur_y += 8;
        } else if (*str == '\r') {
            ui_state.cur_x = 0;
        } else {
            draw_char_local(*str, ui_state.term_x + 5 + ui_state.cur_x, ui_state.term_y + 32 + 5 + ui_state.cur_y, color);
            ui_state.cur_x += 8;
        }
        
        if (ui_state.cur_x >= ui_state.term_w - 10) {
            ui_state.cur_x = 0;
            ui_state.cur_y += 8;
        }
        
        if (ui_state.cur_y >= ui_state.term_h - 40) {
            compositor_scroll_terminal();
            ui_state.cur_y -= 8;
        }
        str++;
    }
    compositor_swap_buffers();
}

static void draw_fedora_theme() {
    compositor_draw_rect(0, 0, g_info.fb_width, g_info.fb_height, 0x242424);
    compositor_draw_rect(0, 0, g_info.fb_width, 32, 0x000000);
    compositor_draw_string("Activities", 20, 12, 0xFFFFFF);
    compositor_draw_string("12:00 PM", g_info.fb_width / 2 - 32, 12, 0xFFFFFF);
    
    compositor_draw_rect(ui_state.term_x, ui_state.term_y, ui_state.term_w, 32, 0x303030);
    compositor_draw_string("Terminal", ui_state.term_x + ui_state.term_w / 2 - 32, ui_state.term_y + 12, 0xFFFFFF);
    compositor_draw_rect(ui_state.term_x, ui_state.term_y + 32, ui_state.term_w, ui_state.term_h - 32, 0x1E1E1E);
}

// Math Helpers for Gradient/Alpha
static uint32_t blend_colors(uint32_t fg, uint32_t bg, uint8_t alpha) {
    uint8_t inv_alpha = 255 - alpha;

    uint32_t r_blend = ((fg >> 16) & 0xFF) * alpha + ((bg >> 16) & 0xFF) * inv_alpha;
    uint8_t r = (uint8_t)((r_blend + 1 + (r_blend >> 8)) >> 8);

    uint32_t g_blend = ((fg >> 8) & 0xFF) * alpha + ((bg >> 8) & 0xFF) * inv_alpha;
    uint8_t g = (uint8_t)((g_blend + 1 + (g_blend >> 8)) >> 8);

    uint32_t b_blend = (fg & 0xFF) * alpha + (bg & 0xFF) * inv_alpha;
    uint8_t b = (uint8_t)((b_blend + 1 + (b_blend >> 8)) >> 8);

    return (r << 16) | (g << 8) | b;
}

static void draw_icon(uint32_t x, uint32_t y, uint32_t color, const char* label) {
    compositor_draw_rect(x + 8, y, 16, 16, color);
    compositor_draw_rect(x + 4, y + 16, 24, 4, 0xAAAAAA); // Base
    compositor_draw_string(label, x, y + 24, 0xFFFFFF);
}

static void draw_aero_theme() {
    // Radial-like background (dark blue)
    compositor_draw_rect(0, 0, g_info.fb_width, g_info.fb_height, 0x003b6f); 
    
    // Desktop Icons
    draw_icon(40, 40, 0x00A4FF, "Computer");
    draw_icon(40, 100, 0xFFA500, "Documents");
    draw_icon(40, 160, 0x00FF00, "Browser");

    uint32_t taskbar_h = 48;
    uint32_t taskbar_y = g_info.fb_height - taskbar_h;
    
    // Glass taskbar (blended)
    for (uint32_t dy = 0; dy < taskbar_h; dy++) {
        for (uint32_t x = 0; x < g_info.fb_width; x++) {
            uint32_t bg = 0x003b6f;
            compositor_put_pixel(x, taskbar_y + dy, blend_colors(0x0f172a, bg, 150));
        }
    }
    
    // Start Button (Circular-ish)
    compositor_draw_rect(10, taskbar_y + 4, 40, 40, 0x005fad);
    compositor_draw_string("T", 24, taskbar_y + 16, 0xFFFFFF);
    
    // Active App Tab
    compositor_draw_rect(60, taskbar_y + 4, 120, 40, 0x55FFFFFF); // Transparent white
    compositor_draw_string("Terminal", 80, taskbar_y + 16, 0x000000);

    // Window with Aero Glass border
    uint32_t border = 8;
    for (uint32_t dy = 0; dy < ui_state.term_h + border * 2; dy++) {
        for (uint32_t dx = 0; dx < ui_state.term_w + border * 2; dx++) {
            if (dx >= border && dx < ui_state.term_w + border && dy >= 32 && dy < ui_state.term_h + border) continue; 
            uint32_t x = ui_state.term_x - border + dx;
            uint32_t y = ui_state.term_y - border + dy;
            if (x < g_info.fb_width && y < g_info.fb_height) {
                uint32_t bg = 0x003b6f;
                double_buffer[y * g_info.fb_pitch + x] = blend_colors(0x88CCFF, bg, 100);
            }
        }
    }
    
    compositor_draw_string("T-OS Explorer", ui_state.term_x + 10, ui_state.term_y + 8, 0x000000);
    compositor_draw_rect(ui_state.term_x + ui_state.term_w - 40, ui_state.term_y, 32, 24, 0xCC0000);
    compositor_draw_rect(ui_state.term_x, ui_state.term_y + 32, ui_state.term_w, ui_state.term_h - 32, 0x000000);
    
    // Clock
    compositor_draw_string("10:42 PM", g_info.fb_width - 80, taskbar_y + 16, 0xFFFFFF);
}

static void draw_cyber_biolume_theme() {
    // Abyss background
    compositor_draw_rect(0, 0, g_info.fb_width, g_info.fb_height, 0x0e0e0e);
    
    // Ambient Glow (subtle rects)
    compositor_draw_rect(g_info.fb_width/4, g_info.fb_height/4, 200, 200, 0x010505);

    // Top Bar
    compositor_draw_rect(0, 0, g_info.fb_width, 32, 0x0e0e0e);
    compositor_draw_rect(0, 31, g_info.fb_width, 1, 0x00f5d4); // Cyan line
    compositor_draw_string("T-OS BIOLUME", 20, 8, 0x00f5d4);
    compositor_draw_string("14:02:59 KRNL_ACTIVE", g_info.fb_width - 200, 8, 0x00f5d4);

    // Terminal window
    uint32_t border = 1;
    compositor_draw_rect(ui_state.term_x - border, ui_state.term_y - border, ui_state.term_w + border*2, ui_state.term_h + border*2, 0x3300f5d4);
    compositor_draw_rect(ui_state.term_x, ui_state.term_y, ui_state.term_w, 32, 0x202020);
    compositor_draw_string("root@t-os: ~/kernel/bio_sync", ui_state.term_x + 10, ui_state.term_y + 8, 0x83948f);
    compositor_draw_rect(ui_state.term_x, ui_state.term_y + 32, ui_state.term_w, ui_state.term_h - 32, 0x050505);

    // Biolume Load Widget
    uint32_t wx = g_info.fb_width - 250, wy = 100, ww = 200, wh = 250;
    compositor_draw_rect(wx, wy, ww, wh, 0x151515);
    compositor_draw_rect(wx, wy, ww, 2, 0x00f5d4);
    compositor_draw_string("BIOLUME_LOAD", wx + 40, wy + 20, 0x00f5d4);
    
    // Progress Bars
    compositor_draw_string("Neural Sync", wx + 10, wy + 60, 0x83948f);
    compositor_draw_rect(wx + 10, wy + 80, 180, 4, 0x222222);
    compositor_draw_rect(wx + 10, wy + 80, 150, 4, 0x00f5d4); // 84%
    
    compositor_draw_string("Core Temp", wx + 10, wy + 110, 0x83948f);
    compositor_draw_rect(wx + 10, wy + 130, 180, 4, 0x222222);
    compositor_draw_rect(wx + 10, wy + 130, 60, 4, 0x94d3c3); // 32%
    
    // Bottom Dock
    uint32_t dock_w = 400, dock_h = 50;
    uint32_t dock_x = (g_info.fb_width - dock_w) / 2;
    uint32_t dock_y = g_info.fb_height - 70;
    compositor_draw_rect(dock_x, dock_y, dock_w, dock_h, 0x202020);
    compositor_draw_rect(dock_x, dock_y, dock_w, 1, 0x33d7fff3);
    compositor_draw_string("HOME  TERM  FILES  NODES  MEDIA  POWER", dock_x + 20, dock_y + 16, 0xd7fff3);
}

static void draw_bios_theme() {
    // Classic BIOS Blue
    compositor_draw_rect(0, 0, g_info.fb_width, g_info.fb_height, 0x0000AA);
    
    // Grey header
    compositor_draw_rect(20, 20, g_info.fb_width - 40, 30, 0xAAAAAA);
    compositor_draw_string("T-OS Setup Utility - Copyright (C) 2026", 40, 30, 0x000000);
    
    // Main setup area
    compositor_draw_rect(20, 60, g_info.fb_width - 40, g_info.fb_height - 100, 0xAAAAAA);
    compositor_draw_rect(ui_state.term_x, ui_state.term_y, ui_state.term_w, ui_state.term_h, 0x000000);
}

static void draw_tgo_editor() {
    // Editor Background
    compositor_draw_rect(0, 0, g_info.fb_width, g_info.fb_height, 0x1E1E1E);
    
    // Top Bar (TGO IDE)
    compositor_draw_rect(0, 0, g_info.fb_width, 32, 0x333333);
    compositor_draw_string("TGO IDE v1.0 - [Bio-Sync LSP Active]", 20, 8, 0x00FF00);
    compositor_draw_string("Live Compile: READY", g_info.fb_width - 200, 8, 0xFFFF00);

    // Sidebar (Files)
    compositor_draw_rect(0, 32, 150, g_info.fb_height - 32, 0x252526);
    compositor_draw_string("FILES", 10, 42, 0x888888);
    compositor_draw_string("main.c", 20, 70, 0x00A4FF);
    compositor_draw_string("shader.tgg", 20, 90, 0x00F5D4);
    compositor_draw_string("kernel.hc", 20, 110, 0xFFA500);

    // Code Area (Mocking some code with "LSP" highlighting)
    uint32_t cx = 170, cy = 50;
    compositor_draw_string("// T-OS HolyC Kernel Snippet", cx, cy, 0x6A9955);
    cy += 20;
    compositor_draw_string("U0 ", cx, cy, 0x569CD6); // HolyC U0
    compositor_draw_string("Main() {", cx + 24, cy, 0xD4D4D4);
    cy += 20;
    compositor_draw_string("    Print(", cx, cy, 0xDCDCAA);
    compositor_draw_string("\"Hello T-OS\"", cx + 80, cy, 0xCE9178);
    compositor_draw_string(");", cx + 180, cy, 0xD4D4D4);
    cy += 20;
    compositor_draw_string("}", cx, cy, 0xD4D4D4);

    // Compile Error Console (Phase 3: Compile Errors check)
    uint32_t console_y = g_info.fb_height - 150;
    compositor_draw_rect(150, console_y, g_info.fb_width - 150, 150, 0x000000);
    compositor_draw_rect(150, console_y, g_info.fb_width - 150, 2, 0xFF0000);
    compositor_draw_string("PROBLEMS", 170, console_y + 10, 0xFFFFFF);
    
    // Simulating a compiler error check
    compositor_draw_string("Error: Unknown target '-target=tgo' in shader.tgg", 170, console_y + 40, 0xFF5555);
    compositor_draw_string("      Did you mean '-target=tgg'?", 170, console_y + 60, 0xFFAAAA);
    compositor_draw_string("[LSP] issue: [ERR_MISPELLED_TARGET_0x42]", 170, console_y + 90, 0xFF0000);
}

void compositor_draw_desktop(void) {
    switch (ui_state.current_theme) {
        case THEME_FEDORA_DARK: draw_fedora_theme(); break;
        case THEME_WINDOWS_AERO: draw_aero_theme(); break;
        case THEME_CYBER_BIOLUME: draw_cyber_biolume_theme(); break;
        case THEME_BIOS_DEFAULT: draw_bios_theme(); break;
        case THEME_TGO_EDITOR: draw_tgo_editor(); break;
    }
    ui_state.cur_x = 0;
    ui_state.cur_y = 0;
}

void compositor_toggle_theme(void) {
    ui_state.current_theme = (ui_state.current_theme + 1) % 5;
    compositor_draw_desktop();
    compositor_print("THEME SWITCHED\n", 0x00FF00);
}

void compositor_handle_interrupt(uint8_t scancode) {
    // 0x58 is F12 in PS/2 Set 1
    if (scancode == 0x58) {
        compositor_print("F12 PRESSED - TOGGLING THEME\n", 0x00FF00);
        compositor_toggle_theme();
    }
}
