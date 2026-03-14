#include "compositor.h"
#include "gui/font_renderer.h"

// Basic assumptions requested by prompt
extern void* kmalloc(uint64_t size);

static GOP_Info g_info;
static uint32_t* double_buffer = NULL;
static UIState ui_state;

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

    uint32_t r_blend = ((fg >> 16) & 0xFF) * alpha + ((bg >> 16) & 0xFF) * inv_alpha;
    uint8_t r = (uint8_t)((r_blend + 1 + (r_blend >> 8)) >> 8);

    uint32_t g_blend = ((fg >> 8) & 0xFF) * alpha + ((bg >> 8) & 0xFF) * inv_alpha;
    uint8_t g = (uint8_t)((g_blend + 1 + (g_blend >> 8)) >> 8);

    uint32_t b_blend = (fg & 0xFF) * alpha + (bg & 0xFF) * inv_alpha;
    uint8_t b = (uint8_t)((b_blend + 1 + (b_blend >> 8)) >> 8);

    return (r << 16) | (g << 8) | b;
}

static void draw_aero_theme() {
    compositor_draw_rect(0, 0, g_info.fb_width, g_info.fb_height, 0x103050); 
    
    uint32_t taskbar_h = 40;
    uint32_t taskbar_y = g_info.fb_height - taskbar_h;

    uint32_t r_step = (0x10 << 16) / taskbar_h;
    uint32_t g_step = (0x20 << 16) / taskbar_h;
    uint32_t b_step = (0x40 << 16) / taskbar_h;

    uint32_t r_acc = 0x20 << 16;
    uint32_t g_acc = 0x40 << 16;
    uint32_t b_acc = 0x80 << 16;

    uint32_t half_h = taskbar_h / 2;

    for (uint32_t dy = 0; dy < taskbar_h; dy++) {
        uint8_t r = r_acc >> 16;
        uint8_t g = g_acc >> 16;
        uint8_t b = b_acc >> 16;

        if (dy < half_h) { r += 0x20; g += 0x20; b += 0x20; }
        uint32_t color = (r << 16) | (g << 8) | b;
        compositor_draw_rect(0, taskbar_y + dy, g_info.fb_width, 1, color);

        r_acc += r_step;
        g_acc += g_step;
        b_acc += b_step;
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
