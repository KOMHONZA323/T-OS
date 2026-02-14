#include "keyboard.h"
#include "ports.h"
#include "../kernel/isr.h"

// Scancode Set 1 (US QWERTY)
char scancode_map[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

// Circular Buffer
#define KBD_BUFFER_SIZE 256
volatile char kbd_buffer[KBD_BUFFER_SIZE];
volatile uint8_t kbd_head = 0;
volatile uint8_t kbd_tail = 0;

void keyboard_handler(registers_t *regs) {
    uint8_t scancode = port_byte_in(0x60);

    if (scancode & 0x80) {
        // Break code (Key Up)
    } else {
        // Make code (Key Down)
        if (scancode < 128) {
            char c = scancode_map[scancode];
            if (c) {
                uint8_t next_head = (kbd_head + 1) % KBD_BUFFER_SIZE;
                if (next_head != kbd_tail) {
                    kbd_buffer[kbd_head] = c;
                    kbd_head = next_head;
                }
            }
        }
    }
}

char get_char() {
    if (kbd_head == kbd_tail) return 0;

    char c = kbd_buffer[kbd_tail];
    kbd_tail = (kbd_tail + 1) % KBD_BUFFER_SIZE;
    return c;
}
