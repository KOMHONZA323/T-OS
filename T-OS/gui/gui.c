#include "font.h"
#include "gui.h"

EFI_SYSTEM_TABLE *gui_ST;
BootInfo gui_boot_info;
uint32_t *back_buffer = NULL;
EFI_SIMPLE_POINTER_PROTOCOL *g_mouse = NULL;

// Colors - Softer Windows 7 Palette
#define COLOR_DESKTOP 0x00336699
#define COLOR_TASKBAR 0x00D0D0D0
#define COLOR_WINDOW 0x00FFFFFF
#define COLOR_TITLEBAR 0x00004A7A
#define COLOR_TEXT_W 0x00FFFFFF
#define COLOR_TEXT_B 0x00000000
#define COLOR_TERMINAL 0x00000000
#define COLOR_START_BTN 0x002E8B57

// Terminal State
#define TERM_COLS 80
#define TERM_ROWS 25
char term_buf[TERM_ROWS][TERM_COLS];
int term_cx = 0, term_cy = 0;

int mouse_x = 0, mouse_y = 0;
BOOLEAN mouse_lb = FALSE;
BOOLEAN prev_mouse_lb = FALSE;

GuiWindow main_window;
BOOLEAN start_menu_open = FALSE;

void gui_init(EFI_SYSTEM_TABLE *ST, BootInfo bi) {
  gui_ST = ST;
  gui_boot_info = bi;
  mouse_x = bi.fb_width / 2;
  mouse_y = bi.fb_height / 2;

  for (int i = 0; i < TERM_ROWS; i++) {
    for (int j = 0; j < TERM_COLS; j++) {
      term_buf[i][j] = ' ';
    }
  }

  if (back_buffer == NULL) {
    ST->BootServices->AllocatePool(
        2, (UINTN)bi.fb_width * (UINTN)bi.fb_height * 4, (void **)&back_buffer);
  }

  EFI_GUID mouse_guid = EFI_SIMPLE_POINTER_PROTOCOL_GUID;
  ST->BootServices->LocateProtocol(&mouse_guid, NULL, (void **)&g_mouse);
  if (g_mouse)
    g_mouse->Reset(g_mouse, TRUE);

  main_window.x = 100;
  main_window.y = 100;
  main_window.w = TERM_COLS * 8 + 8;
  main_window.h = TERM_ROWS * 8 + 28;
  const char *title = "Command Prompt";
  int i = 0;
  while (title[i]) {
    main_window.title[i] = title[i];
    i++;
  }
  main_window.title[i] = 0;
  main_window.minimized = FALSE;
  main_window.maximized = FALSE;
  main_window.dragging = FALSE;
}

void gui_draw_pixel(UINT32 x, UINT32 y, UINT32 color) {
  if (x >= gui_boot_info.fb_width || y >= gui_boot_info.fb_height)
    return;
  back_buffer[(uint64_t)y * (uint64_t)gui_boot_info.fb_width + (uint64_t)x] =
      color;
}

void gui_swap_buffers() {
  if (!back_buffer)
    return;
  uint32_t *fb = (uint32_t *)gui_boot_info.fb_base;
  uint32_t width = gui_boot_info.fb_width;
  uint32_t height = gui_boot_info.fb_height;
  uint32_t pitch = gui_boot_info.fb_pitch;

  if (pitch == width) {
    gui_ST->BootServices->CopyMem(fb, back_buffer, (UINTN)width * height * 4);
  } else {
    for (uint64_t y = 0; y < (uint64_t)height; y++) {
      gui_ST->BootServices->CopyMem(&fb[y * pitch], &back_buffer[y * width],
                                    (UINTN)width * 4);
    }
  }
}

void gui_fill_rect(UINT32 x, UINT32 y, UINT32 w, UINT32 h, UINT32 color) {
  if (x >= gui_boot_info.fb_width || y >= gui_boot_info.fb_height)
    return;
  if (x + w > gui_boot_info.fb_width)
    w = gui_boot_info.fb_width - x;
  if (y + h > gui_boot_info.fb_height)
    h = gui_boot_info.fb_height - y;

  for (UINT32 j = 0; j < h; j++) {
    uint32_t *line =
        &back_buffer[(uint64_t)(y + j) * (uint64_t)gui_boot_info.fb_width + x];
    for (UINT32 i = 0; i < w; i++) {
      line[i] = color;
    }
  }
}

void gui_draw_rect(UINT32 x, UINT32 y, UINT32 w, UINT32 h, UINT32 color) {
  if (w == 0 || h == 0 || !back_buffer)
    return;

  UINT32 sw = gui_boot_info.fb_width;
  UINT32 sh = gui_boot_info.fb_height;

  UINT32 x2 = x + w - 1;
  UINT32 y2 = y + h - 1;

  // Horizontal lines (top and bottom)
  if (y < sh && x < sw) {
    UINT32 end = (x2 >= sw) ? (sw - 1) : x2;
    uint32_t *p = &back_buffer[(UINT64)y * sw + x];
#if defined(__x86_64__) || defined(__i386__)
    UINT32 count = end - x + 1;
    __asm__ volatile("rep stosl" : "+D"(p), "+c"(count) : "a"(color) : "memory");
#else
    for (UINT32 i = x; i <= end; i++)
      *p++ = color;
#endif
  }
  if (y2 < sh && y2 != y && x < sw) {
    UINT32 end = (x2 >= sw) ? (sw - 1) : x2;
    uint32_t *p = &back_buffer[(UINT64)y2 * sw + x];
#if defined(__x86_64__) || defined(__i386__)
    UINT32 count = end - x + 1;
    __asm__ volatile("rep stosl" : "+D"(p), "+c"(count) : "a"(color) : "memory");
#else
    for (UINT32 i = x; i <= end; i++)
      *p++ = color;
#endif
  }

  // Vertical lines (left and right)
  if (x < sw && y < sh) {
    UINT32 end = (y2 >= sh) ? (sh - 1) : y2;
    uint32_t *p = &back_buffer[(UINT64)y * sw + x];
    for (UINT32 j = y; j <= end; j++) {
      *p = color;
      p += sw;
    }
  }
  if (x2 < sw && x2 != x && y < sh) {
    UINT32 end = (y2 >= sh) ? (sh - 1) : y2;
    uint32_t *p = &back_buffer[(UINT64)y * sw + x2];
    for (UINT32 j = y; j <= end; j++) {
      *p = color;
      p += sw;
    }
  }
}

void gui_draw_char(char c, UINT32 x, UINT32 y, UINT32 color, UINT32 bg_color) {
  if (c < 0 || c > 127)
    return;
  char *glyph = font8x8_basic[(uint8_t)c];
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      if ((glyph[i] >> j) & 1) {
        gui_draw_pixel(x + j, y + i, color);
      } else if (bg_color != 0xFFFFFFFF) {
        gui_draw_pixel(x + j, y + i, bg_color);
      }
    }
  }
}

void gui_draw_string(const char *s, UINT32 x, UINT32 y, UINT32 color,
                     UINT32 bg_color) {
  while (*s) {
    gui_draw_char(*s, x, y, color, bg_color);
    x += 8;
    s++;
  }
}

void gui_draw_desktop() {
  gui_fill_rect(0, 0, gui_boot_info.fb_width, gui_boot_info.fb_height,
                COLOR_DESKTOP);
}

void gui_draw_taskbar() {
  UINT32 h = 40;
  UINT32 y = gui_boot_info.fb_height - h;
  gui_fill_rect(0, y, gui_boot_info.fb_width, h, COLOR_TASKBAR);
  gui_draw_rect(0, y, gui_boot_info.fb_width, h, COLOR_TEXT_W);

  gui_fill_rect(5, y + 5, 60, 30,
                start_menu_open ? COLOR_TITLEBAR : COLOR_WINDOW);
  gui_draw_rect(5, y + 5, 60, 30, COLOR_TEXT_B);
  gui_draw_string("Start", 15, y + 15,
                  start_menu_open ? COLOR_TEXT_W : COLOR_TEXT_B,
                  start_menu_open ? COLOR_TITLEBAR : COLOR_WINDOW);

  if (main_window.minimized) {
    gui_fill_rect(70, y + 5, 120, 30, COLOR_WINDOW);
    gui_draw_rect(70, y + 5, 120, 30, COLOR_TEXT_B);
    gui_draw_string("Terminal", 80, y + 15, COLOR_TEXT_B, COLOR_WINDOW);
  }

  if (start_menu_open) {
    gui_fill_rect(5, y - 200, 150, 200, COLOR_WINDOW);
    gui_draw_rect(5, y - 200, 150, 200, COLOR_TEXT_B);
    gui_draw_string("T-OS Menu", 15, y - 190, COLOR_TEXT_B, COLOR_WINDOW);
    gui_draw_string("----------", 15, y - 175, COLOR_TEXT_B, COLOR_WINDOW);
    gui_draw_string("Documents", 15, y - 150, COLOR_TEXT_B, COLOR_WINDOW);
    gui_draw_string("Computer", 15, y - 130, COLOR_TEXT_B, COLOR_WINDOW);
    gui_draw_string("Control Panel", 15, y - 110, COLOR_TEXT_B, COLOR_WINDOW);
    gui_draw_string("Shutdown", 15, y - 40, COLOR_TEXT_B, COLOR_WINDOW);
  }
}

void gui_draw_window(UINT32 x, UINT32 y, UINT32 w, UINT32 h, const char *title,
                     BOOLEAN minimized, BOOLEAN maximized) {
  if (minimized)
    return;
  UINT32 draw_x = maximized ? 0 : x;
  UINT32 draw_y = maximized ? 0 : y;
  UINT32 draw_w = maximized ? gui_boot_info.fb_width : w;
  UINT32 draw_h = maximized ? gui_boot_info.fb_height - 40 : h;

  gui_fill_rect(draw_x, draw_y, draw_w, draw_h, COLOR_WINDOW);
  gui_draw_rect(draw_x, draw_y, draw_w, draw_h, COLOR_TEXT_B);
  gui_draw_rect(draw_x + 1, draw_y + 1, draw_w - 2, draw_h - 2, COLOR_TEXT_W);

  gui_fill_rect(draw_x + 2, draw_y + 2, draw_w - 4, 20, COLOR_TITLEBAR);
  gui_draw_string(title, draw_x + 5, draw_y + 8, COLOR_TEXT_W, COLOR_TITLEBAR);

  gui_fill_rect(draw_x + draw_w - 20, draw_y + 4, 16, 16, COLOR_TASKBAR);
  gui_draw_rect(draw_x + draw_w - 20, draw_y + 4, 16, 16, COLOR_TEXT_B);
  gui_draw_string("X", draw_x + draw_w - 16, draw_y + 8, COLOR_TEXT_B,
                  COLOR_TASKBAR);

  gui_fill_rect(draw_x + draw_w - 40, draw_y + 4, 16, 16, COLOR_TASKBAR);
  gui_draw_rect(draw_x + draw_w - 40, draw_y + 4, 16, 16, COLOR_TEXT_B);
  gui_draw_string("M", draw_x + draw_w - 36, draw_y + 8, COLOR_TEXT_B,
                  COLOR_TASKBAR);

  gui_fill_rect(draw_x + draw_w - 60, draw_y + 4, 16, 16, COLOR_TASKBAR);
  gui_draw_rect(draw_x + draw_w - 60, draw_y + 4, 16, 16, COLOR_TEXT_B);
  gui_draw_string("_", draw_x + draw_w - 56, draw_y + 8, COLOR_TEXT_B,
                  COLOR_TASKBAR);
}

void gui_draw_cursor(UINT32 x, UINT32 y) {
  // Visible Arrow Cursor
  for (int i = 0; i < 15; i++) {
    for (int j = 0; j < i; j++) {
      gui_draw_pixel(x + j, y + i, COLOR_TEXT_B);
    }
    gui_draw_pixel(x + i, y + i, COLOR_TEXT_W);
    gui_draw_pixel(x, y + i, COLOR_TEXT_W);
  }
  for (int i = 0; i < 15; i++)
    gui_draw_pixel(x + i, y, COLOR_TEXT_W);
}

void term_print(const char *str) {
  while (*str) {
    if (*str == '\n') {
      term_cx = 0;
      term_cy++;
    } else if (*str == '\r') {
      term_cx = 0;
    } else if (*str == '\b') {
      if (term_cx > 0) {
        term_cx--;
        term_buf[term_cy][term_cx] = ' ';
      }
    } else {
      if (term_cx >= TERM_COLS) {
        term_cx = 0;
        term_cy++;
      }
      if (term_cy < TERM_ROWS) {
        term_buf[term_cy][term_cx] = *str;
      }
      term_cx++;
    }
    if (term_cy >= TERM_ROWS) {
      for (int i = 1; i < TERM_ROWS; i++) {
        for (int j = 0; j < TERM_COLS; j++) {
          term_buf[i - 1][j] = term_buf[i][j];
        }
      }
      for (int j = 0; j < TERM_COLS; j++) {
        term_buf[TERM_ROWS - 1][j] = ' ';
      }
      term_cy = TERM_ROWS - 1;
    }
    str++;
  }
}

void term_draw() {
  if (main_window.minimized)
    return;
  UINT32 draw_x = main_window.maximized ? 0 : main_window.x;
  UINT32 draw_y = main_window.maximized ? 0 : main_window.y;
  UINT32 draw_w =
      main_window.maximized ? gui_boot_info.fb_width : main_window.w;
  UINT32 draw_h =
      main_window.maximized ? gui_boot_info.fb_height - 40 : main_window.h;

  UINT32 term_x = draw_x + 4;
  UINT32 term_y = draw_y + 24;

  gui_fill_rect(term_x, term_y, draw_w - 8, draw_h - 28, COLOR_TERMINAL);
  for (int i = 0; i < TERM_ROWS; i++) {
    for (int j = 0; j < TERM_COLS; j++) {
      if (term_buf[i][j] != ' ') {
        gui_draw_char(term_buf[i][j], term_x + j * 8, term_y + i * 8,
                      COLOR_TEXT_W, COLOR_TERMINAL);
      }
    }
  }
  gui_fill_rect(term_x + term_cx * 8, term_y + term_cy * 8, 8, 8, COLOR_TEXT_W);
}

void gui_term_print(const char *str) { term_print(str); }

void gui_poll_mouse() {
  if (g_mouse) {
    EFI_SIMPLE_POINTER_STATE state;
    if (g_mouse->GetState(g_mouse, &state) == EFI_SUCCESS) {
      // Highly reduced sensitivity for better control on real hardware
      mouse_x += state.RelativeMovementX / 15000;
      mouse_y += state.RelativeMovementY / 15000;
      if (mouse_x < 0)
        mouse_x = 0;
      if (mouse_y < 0)
        mouse_y = 0;
      if ((UINT32)mouse_x >= gui_boot_info.fb_width)
        mouse_x = gui_boot_info.fb_width - 1;
      if ((UINT32)mouse_y >= gui_boot_info.fb_height)
        mouse_y = gui_boot_info.fb_height - 1;
      prev_mouse_lb = mouse_lb;
      mouse_lb = state.LeftButton;
    }
  }

  UINT32 draw_x = main_window.maximized ? 0 : main_window.x;
  UINT32 draw_y = main_window.maximized ? 0 : main_window.y;
  UINT32 draw_w =
      main_window.maximized ? gui_boot_info.fb_width : main_window.w;

  if (mouse_lb && !prev_mouse_lb && !main_window.maximized &&
      !main_window.minimized) {
    if (mouse_x >= (int)draw_x + 2 && mouse_x <= (int)(draw_x + draw_w - 60) &&
        mouse_y >= (int)draw_y + 2 && mouse_y <= (int)draw_y + 22) {
      main_window.dragging = TRUE;
      main_window.drag_off_x = mouse_x - (int)main_window.x;
      main_window.drag_off_y = mouse_y - (int)main_window.y;
    }
  }
  if (!mouse_lb)
    main_window.dragging = FALSE;
  if (main_window.dragging) {
    main_window.x = mouse_x - main_window.drag_off_x;
    main_window.y = mouse_y - main_window.drag_off_y;
  }

  if (mouse_lb && !prev_mouse_lb) {
    if (mouse_x >= (int)(draw_x + draw_w - 20) &&
        mouse_x <= (int)(draw_x + draw_w - 4) && mouse_y >= (int)draw_y + 4 &&
        mouse_y <= (int)draw_y + 20) {
    }
    if (mouse_x >= (int)(draw_x + draw_w - 40) &&
        mouse_x <= (int)(draw_x + draw_w - 24) && mouse_y >= (int)draw_y + 4 &&
        mouse_y <= (int)draw_y + 20) {
      main_window.maximized = !main_window.maximized;
    }
    if (mouse_x >= (int)(draw_x + draw_w - 60) &&
        mouse_x <= (int)(draw_x + draw_w - 44) && mouse_y >= (int)draw_y + 4 &&
        mouse_y <= (int)draw_y + 20) {
      main_window.minimized = TRUE;
    }
    if (main_window.minimized && mouse_x >= 70 && mouse_x <= 190 &&
        mouse_y >= (int)gui_boot_info.fb_height - 35 &&
        mouse_y <= (int)gui_boot_info.fb_height - 5) {
      main_window.minimized = FALSE;
    }
    if (mouse_x >= 5 && mouse_x <= 65 &&
        mouse_y >= (int)gui_boot_info.fb_height - 35 &&
        mouse_y <= (int)gui_boot_info.fb_height - 5) {
      start_menu_open = !start_menu_open;
    } else if (start_menu_open) {
      start_menu_open = FALSE;
    }
  }
}

char gui_term_getchar() {
  while (1) {
    EFI_INPUT_KEY key;
    if (gui_ST->ConIn->ReadKeyStroke(gui_ST->ConIn, &key) == EFI_SUCCESS) {
      if (key.UnicodeChar == '\r')
        return '\n';
      if (key.UnicodeChar != 0)
        return (char)key.UnicodeChar;
    }

    gui_poll_mouse();

    gui_draw_desktop();
    gui_draw_taskbar();
    gui_draw_window(main_window.x, main_window.y, main_window.w, main_window.h,
                    (const char *)main_window.title, main_window.minimized,
                    main_window.maximized);
    term_draw();
    gui_draw_cursor((UINT32)mouse_x, (UINT32)mouse_y);
    gui_swap_buffers();
    gui_ST->BootServices->Stall(20000); // 50 FPS
  }
}

void gui_start() {
  gui_draw_desktop();
  gui_draw_taskbar();
  gui_draw_window(main_window.x, main_window.y, main_window.w, main_window.h,
                  (const char *)main_window.title, main_window.minimized,
                  main_window.maximized);
  term_draw();
  gui_draw_cursor((UINT32)mouse_x, (UINT32)mouse_y);
  gui_swap_buffers();
}
