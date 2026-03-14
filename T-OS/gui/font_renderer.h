#ifndef FONT_RENDERER_H
#define FONT_RENDERER_H

#include <stdint.h>
#include "bootinfo.h"

void font_draw_char(BootInfo* bi, char c, uint32_t x, uint32_t y, uint32_t color, int scale);
void font_draw_string(BootInfo* bi, const char* s, uint32_t x, uint32_t y, uint32_t color, int scale);

#endif
