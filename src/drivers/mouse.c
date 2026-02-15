#include "mouse.h"
#include "ports.h"
#include "../kernel/isr.h"
#include "screen.h"

static uint8_t mouse_cycle = 0;
static int8_t mouse_byte[3];
static int32_t mouse_x = 0;
static int32_t mouse_y = 0;
static uint8_t mouse_buttons = 0;

void mouse_wait(uint8_t type) {
    uint32_t timeout = 100000;
    if (type == 0) { // Data
        while (timeout--) {
            if ((port_byte_in(0x64) & 1) == 1) return;
        }
    } else { // Signal
        while (timeout--) {
            if ((port_byte_in(0x64) & 2) == 0) return;
        }
    }
}

void mouse_write(uint8_t write) {
    mouse_wait(1);
    port_byte_out(0x64, 0xD4);
    mouse_wait(1);
    port_byte_out(0x60, write);
}

uint8_t mouse_read() {
    mouse_wait(0);
    return port_byte_in(0x60);
}

void init_mouse() {
    uint8_t status;

    // Enable Auxiliary Device
    mouse_wait(1);
    port_byte_out(0x64, 0xA8);

    // Enable Interrupts
    mouse_wait(1);
    port_byte_out(0x64, 0x20); // Command: Read Byte 0
    mouse_wait(0);
    status = (port_byte_in(0x60) | 2); // Enable IRQ 12
    mouse_wait(1);
    port_byte_out(0x64, 0x60); // Command: Write Byte 0
    mouse_wait(1);
    port_byte_out(0x60, status);

    // Default Settings
    mouse_write(0xF6);
    mouse_read(); // Acknowledge (0xFA)

    // Enable Data Reporting
    mouse_write(0xF4);
    mouse_read(); // Acknowledge (0xFA)

    // Center mouse
    mouse_x = g_width / 2;
    mouse_y = g_height / 2;
}

void mouse_handler(registers_t *regs) {
    uint8_t status = port_byte_in(0x64);
    // Bit 5 (0x20) is Mouse (Aux)
    // However, some emulators/hardware might not set it reliably if we just got an IRQ 12.
    // If we are in this handler, it *is* an IRQ 12 (verified in isr.c).
    // So we should just read the byte.
    // if (!(status & 0x20)) return;

    uint8_t b = port_byte_in(0x60);

    // Visual Debug: Draw a pixel at 0,0 that toggles color to prove interrupts work
    static int debug_color = 0;
    put_pixel(0, 0, (debug_color++) % 2 ? 0xFFFF0000 : 0xFF00FF00);

    switch (mouse_cycle) {
        case 0:
            // Bit 3 MUST be 1. But some mice might not set it?
            // If it's 0, we are definitely out of sync.
            if ((b & 0x08) == 0) {
                // Try to resync? Skip?
                // For now, return and wait for next byte hoping it's the start
                return;
            }
            mouse_byte[0] = b;
            mouse_cycle++;
            break;
        case 1:
            mouse_byte[1] = b;
            mouse_cycle++;
            break;
        case 2:
            mouse_byte[2] = b;
            mouse_cycle = 0;

            // Process Packet
            int8_t x_rel = mouse_byte[1];
            int8_t y_rel = mouse_byte[2];

            // Flags
            uint8_t flags = mouse_byte[0];

            // Overflow handling
            if (flags & 0xC0) return; // Overflow X or Y

            // Sign extension for 9-bit mode (if applicable, but usually 8-bit rel)
            // But standard PS/2 is 9-bit X/Y signed? No, it's 8-bit with sign bit in byte 0.
            // Byte 0: Yovfl Xovfl Ysign Xsign 1 M R L

            int32_t rel_x = (int32_t)mouse_byte[1];
            int32_t rel_y = (int32_t)mouse_byte[2];

            if (flags & 0x10) rel_x |= 0xFFFFFF00; // Sign extend X
            if (flags & 0x20) rel_y |= 0xFFFFFF00; // Sign extend Y

            mouse_x += rel_x;
            mouse_y -= rel_y; // Y is inverted usually

            // Clamp
            if (mouse_x < 0) mouse_x = 0;
            if (mouse_y < 0) mouse_y = 0;
            if (mouse_x >= g_width) mouse_x = g_width - 1;
            if (mouse_y >= g_height) mouse_y = g_height - 1;

            mouse_buttons = mouse_byte[0] & 0x07;
            break;
    }
}

int32_t get_mouse_x() { return mouse_x; }
int32_t get_mouse_y() { return mouse_y; }
uint8_t get_mouse_buttons() { return mouse_buttons; }
