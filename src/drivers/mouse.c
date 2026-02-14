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
    port_byte_out(0x64, 0x20);
    mouse_wait(0);
    status = (port_byte_in(0x60) | 2);
    mouse_wait(1);
    port_byte_out(0x64, 0x60);
    mouse_wait(1);
    port_byte_out(0x60, status);

    // Default Settings
    mouse_write(0xF6);
    mouse_read(); // Acknowledge

    // Enable Data Reporting
    mouse_write(0xF4);
    mouse_read(); // Acknowledge

    // Center mouse
    mouse_x = g_width / 2;
    mouse_y = g_height / 2;
}

void mouse_handler(registers_t *regs) {
    uint8_t status = port_byte_in(0x64);
    if (!(status & 0x20)) return; // Not mouse? (Wait, bit 5 is AUX buffer full?)
    // Actually, bit 5 of status register indicates if the byte is from mouse (1) or keyboard (0).
    // Correct.

    uint8_t b = port_byte_in(0x60);

    switch (mouse_cycle) {
        case 0:
            if ((b & 0x08) == 0) return; // Alignment check bit 3 must be 1
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

            mouse_x += x_rel;
            mouse_y -= y_rel;

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
