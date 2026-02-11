#include "keyboard.h"
#include "ports.h"

// Scancode Set 1 (US QWERTY)
char scancode_map[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

char get_char() {
    // Check Status Register (0x64)
    // Bit 0: Output Buffer Full (1 = Data available)
    if (port_byte_in(0x64) & 0x01) {
        // Read Scancode
        unsigned char scancode = port_byte_in(0x60);

        // Ignore Break Codes (0x80 bit set)
        if (scancode & 0x80) {
            return 0;
        }

        // Extended codes (0xE0) - simplify and ignore for now
        if (scancode == 0xE0) {
            return 0;
        }

        if (scancode < 128) {
            return scancode_map[scancode];
        }
    }
    return 0;
}
