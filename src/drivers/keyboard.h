#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "../kernel/isr.h"

char get_char();
void keyboard_handler(registers_t *regs);

#endif
