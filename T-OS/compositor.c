#include "compositor.h"

// Basic assumptions requested by prompt
extern void* kmalloc(uint64_t size);

static GOP_Info g_info;
static uint32_t* double_buffer = NULL;
static UIState ui_state;

// Simple 8x8 font
static uint8_t font[128][8] = {
    [0x20] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // Space
    [0x30] = {0x3C, 0x66, 0x6E, 0x76, 0x66, 0x66, 0x3C, 0x00}, // 0
    [0x31] = {0x18, 0x38, 0x18, 0x18, 0x18, 0x18, 0x7E, 0x00}, // 1
    [0x32] = {0x3C, 0x66, 0x06, 0x0C, 0x18, 0x30, 0x7E, 0x00}, // 2
    [0x41] = {0x18, 0x3C, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x00}, // A
    [0x42] = {0x7C, 0x66, 0x66, 0x7C, 0x66, 0x66, 0x7C, 0x00}, // B
    [0x43] = {0x3C, 0x66, 0x60, 0x60, 0x60, 0x66, 0x3C, 0x00}, // C
    [0x44] = {0x78, 0x6C, 0x66, 0x66, 0x66, 0x6C, 0x78, 0x00}, // D
    [0x45] = {0x7E, 0x60, 0x60, 0x7C, 0x60, 0x60, 0x7E, 0x00}, // E
    [0x46] = {0x7E, 0x60, 0x60, 0x7C, 0x60, 0x60, 0x60, 0x00}, // F
    [0x47] = {0x3C, 0x66, 0x60, 0x6E, 0x66, 0x66, 0x3C, 0x00}, // G
    [0x48] = {0x66, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x66, 0x00}, // H
    [0x49] = {0x3C, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C, 0x00}, // I
    [0x4A] = {0x1E, 0x0C, 0x0C, 0x0C, 0x0C, 0x6C, 0x38, 0x00}, // J
    [0x4B] = {0x66, 0x6C, 0x78, 0x70, 0x78, 0x6C, 0x66, 0x00}, // K
    [0x4C] = {0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x7E, 0x00}, // L
    [0x4D] = {0x63, 0x77, 0x7F, 0x6B, 0x63, 0x63, 0x63, 0x00}, // M
    [0x4E] = {0x63, 0x67, 0x6F, 0x7B, 0x73, 0x63, 0x63, 0x00}, // N
    [0x4F] = {0x3C, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x00}, // O
    [0x50] = {0x7C, 0x66, 0x66, 0x7C, 0x60, 0x60, 0x60, 0x00}, // P
    [0x51] = {0x3C, 0x66, 0x66, 0x66, 0x6E, 0x3C, 0x0E, 0x00}, // Q
    [0x52] = {0x7C, 0x66, 0x66, 0x7C, 0x78, 0x6C, 0x66, 0x00}, // R
    [0x53] = {0x3C, 0x66, 0x30, 0x18, 0x0C, 0x66, 0x3C, 0x00}, // S
    [0x54] = {0x7E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00}, // T
    [0x55] = {0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x00}, // U
    [0x56] = {0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x18, 0x00}, // V
    [0x57] = {0x63, 0x63, 0x63, 0x6B, 0x7F, 0x77, 0x63, 0x00}, // W
    [0x58] = {0x66, 0x66, 0x3C, 0x18, 0x3C, 0x66, 0x66, 0x00}, // X
    [0x59] = {0x66, 0x66, 0x66, 0x3C, 0x18, 0x18, 0x18, 0x00}, // Y
    [0x5A] = {0x7E, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x7E, 0x00}, // Z
    [0x61] = {0x00, 0x00, 0x3C, 0x06, 0x3E, 0x66, 0x3E, 0x00}, // a
    [0x62] = {0x60, 0x60, 0x7C, 0x66, 0x66, 0x66, 0x7C, 0x00}, // b
    [0x63] = {0x00, 0x00, 0x3C, 0x60, 0x60, 0x66, 0x3C, 0x00}, // c
    [0x3A] = {0x00, 0x18, 0x18, 0x00, 0x18, 0x18, 0x00, 0x00}, // :
    [0x2E] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00}, // .
    [0x2D] = {0x00, 0x00, 0x00, 0x3E, 0x00, 0x00, 0x00, 0x00}, // -
};

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

static void draw_char(char c, uint32_t x, uint32_t y, uint32_t color) {
    if (c > 127 || !font[(uint8_t)c][0]) return; 
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            if ((font[(uint8_t)c][i] >> (7 - j)) & 1) {
                compositor_put_pixel(x + j, y + i, color);
            }
        }
    }
}

void compositor_draw_string(const char* str, uint32_t x, uint32_t y, uint32_t color) {
    while (*str) {
        draw_char(*str, x, y, color);
        x += 8;
        str++;
    }
}

// Inline ASM optimizations for swapping
void compositor_swap_buffers(void) {
    if (!double_buffer || !g_info.fb_base) return;
    
    uint32_t* fb = (uint32_t*)g_info.fb_base;
    uint64_t dwords = (uint64_t)g_info.fb_pitch * g_info.fb_height;
    
    // Using REP MOVSQ for high-speed block transfer
    uint64_t qwords = dwords / 2;
    uint32_t rem = dwords % 2;
    
    __asm__ volatile (
        "cld\n\t"
        "rep movsq\n\t"
        : "+S"(double_buffer), "+D"(fb), "+c"(qwords)
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
            draw_char(*str, ui_state.term_x + 5 + ui_state.cur_x, ui_state.term_y + 32 + 5 + ui_state.cur_y, color);
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
    uint8_t r = (uint8_t)(((uint32_t)((fg >> 16) & 0xFF) * alpha + (uint32_t)((bg >> 16) & 0xFF) * inv_alpha) / 255);
    uint8_t g = (uint8_t)(((uint32_t)((fg >> 8) & 0xFF) * alpha + (uint32_t)((bg >> 8) & 0xFF) * inv_alpha) / 255);
    uint8_t b = (uint8_t)(((uint32_t)(fg & 0xFF) * alpha + (uint32_t)(bg & 0xFF) * inv_alpha) / 255);
    return (r << 16) | (g << 8) | b;
}

static void draw_aero_theme() {
    compositor_draw_rect(0, 0, g_info.fb_width, g_info.fb_height, 0x103050); 
    
    uint32_t taskbar_h = 40;
    uint32_t taskbar_y = g_info.fb_height - taskbar_h;
    for (uint32_t dy = 0; dy < taskbar_h; dy++) {
        uint8_t r = 0x20 + (dy * 0x10 / taskbar_h);
        uint8_t g = 0x40 + (dy * 0x20 / taskbar_h);
        uint8_t b = 0x80 + (dy * 0x40 / taskbar_h);
        if (dy < taskbar_h / 2) { r += 0x20; g += 0x20; b += 0x20; }
        uint32_t color = (r << 16) | (g << 8) | b;
        compositor_draw_rect(0, taskbar_y + dy, g_info.fb_width, 1, color);
    }
    
    uint32_t sb_x = 10, sb_y = taskbar_y + 4;
    compositor_draw_rect(sb_x, sb_y, 32, 32, 0x00A0FF);
    compositor_draw_rect(sb_x+4, sb_y+4, 10, 10, 0xFF4040);
    compositor_draw_rect(sb_x+18, sb_y+4, 10, 10, 0x40FF40);
    compositor_draw_rect(sb_x+4, sb_y+18, 10, 10, 0x4040FF);
    compositor_draw_rect(sb_x+18, sb_y+18, 10, 10, 0xFFFF40);
    
    uint32_t border = 8;
    for (uint32_t dy = 0; dy < ui_state.term_h + border * 2; dy++) {
        for (uint32_t dx = 0; dx < ui_state.term_w + border * 2; dx++) {
            if (dx >= border && dx < ui_state.term_w + border && dy >= 32 && dy < ui_state.term_h + border) continue; 
            uint32_t x = ui_state.term_x - border + dx;
            uint32_t y = ui_state.term_y - border + dy;
            if (x < g_info.fb_width && y < g_info.fb_height) {
                uint32_t bg = double_buffer[y * g_info.fb_pitch + x];
                double_buffer[y * g_info.fb_pitch + x] = blend_colors(0x88CCFF, bg, 100);
            }
        }
    }
    compositor_draw_rect(ui_state.term_x + ui_state.term_w - 40, ui_state.term_y, 32, 24, 0xCC0000);
    compositor_draw_rect(ui_state.term_x, ui_state.term_y + 32, ui_state.term_w, ui_state.term_h - 32, 0x000000);
}

void compositor_draw_desktop(void) {
    if (ui_state.current_theme == THEME_FEDORA_DARK) draw_fedora_theme();
    else draw_aero_theme();
    ui_state.cur_x = 0;
    ui_state.cur_y = 0;
    compositor_swap_buffers();
}

void compositor_toggle_theme(void) {
    ui_state.current_theme = (ui_state.current_theme == THEME_FEDORA_DARK) ? THEME_WINDOWS_AERO : THEME_FEDORA_DARK;
    compositor_draw_desktop();
}

void compositor_handle_interrupt(uint8_t scancode) {
    if (scancode == 0x58) compositor_toggle_theme();
}
