#ifndef T_PS2_MOUSE_H
#define T_PS2_MOUSE_H

#include <stdint.h>

void ps2_mouse_init();
void mouse_handler_v2();
int ps2_mouse_poll(int* x, int* y, int* buttons);
int ps2_keyboard_poll(uint8_t* scancode);
void mouse_get_state(int32_t *x, int32_t *y, uint8_t *buttons, int32_t screen_w, int32_t screen_h);

#endif
