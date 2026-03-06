#ifndef _GUI_H_
#define _GUI_H_

#include "../uefi.h"
#include "../bootinfo.h"

extern EFI_SYSTEM_TABLE *gui_ST;
extern BootInfo gui_boot_info;

void gui_init(EFI_SYSTEM_TABLE *ST, BootInfo bi);
void gui_start();

// Drawing primitives
void gui_draw_pixel(UINT32 x, UINT32 y, UINT32 color);
void gui_draw_rect(UINT32 x, UINT32 y, UINT32 w, UINT32 h, UINT32 color);
void gui_fill_rect(UINT32 x, UINT32 y, UINT32 w, UINT32 h, UINT32 color);
void gui_draw_char(char c, UINT32 x, UINT32 y, UINT32 color, UINT32 bg_color);
void gui_draw_string(const char *s, UINT32 x, UINT32 y, UINT32 color, UINT32 bg_color);

// UI elements
void gui_draw_desktop();
void gui_draw_taskbar();
void gui_draw_window(UINT32 x, UINT32 y, UINT32 w, UINT32 h, const char *title, BOOLEAN minimized, BOOLEAN maximized);
void gui_draw_cursor(UINT32 x, UINT32 y);

// State management
typedef struct {
    UINT32 x, y, w, h;
    CHAR8 title[64];
    BOOLEAN minimized;
    BOOLEAN maximized;
    BOOLEAN dragging;
    INT32 drag_off_x, drag_off_y;
} GuiWindow;

extern GuiWindow main_window;
extern BOOLEAN start_menu_open;

// Shell integration
void gui_term_print(const char *str);
char gui_term_getchar();

#endif