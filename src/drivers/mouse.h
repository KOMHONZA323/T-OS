#ifndef MOUSE_H
#define MOUSE_H

#include <stdint.h>
#include "../kernel/isr.h"

void init_mouse();
void mouse_handler(registers_t *regs);
int32_t get_mouse_x();
int32_t get_mouse_y();
uint8_t get_mouse_buttons();

#endif
