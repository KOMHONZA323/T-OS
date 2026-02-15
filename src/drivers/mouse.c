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

    // Enable Interrupts (Get Compaq Status Byte)
    mouse_wait(1);
    port_byte_out(0x64, 0x20); // Command: Read Byte 0
    mouse_wait(0);
    status = (port_byte_in(0x60) | 2); // Enable IRQ 12
    // Also clear bit 5 (Disable Mouse) just in case? No, bit 5=1 means disabled. So we want bit 5=0.
    // Standard byte: [7:Reserved][6:Translate][5:DisableAux][4:DisableKey][3:Reserved][2:System][1:EnableAuxIRQ][0:EnableKeyIRQ]
    // We want bit 1=1 (Enable Aux IRQ) and bit 5=0 (Enable Aux).
    // We ALSO want Bit 6=1 (Enable Translation) to ensure keyboard scancodes are Set 1 (XT).
    status = (status | 2 | 0x40) & ~0x20;

    mouse_wait(1);
    port_byte_out(0x64, 0x60); // Command: Write Byte 0
    mouse_wait(1);
    port_byte_out(0x60, status);

    // Default Settings
    mouse_write(0xF6);
    mouse_read(); // Acknowledge (0xFA)

    // Set Sample Rate to 200 (reduce lag)
    mouse_write(0xF3);
    mouse_read(); // ACK
    mouse_write(200);
    mouse_read(); // ACK

    // Enable Data Reporting
    mouse_write(0xF4);
    mouse_read(); // Acknowledge (0xFA)

    // Center mouse
    mouse_x = g_width / 2;
    mouse_y = g_height / 2;
}

void mouse_handler(registers_t *regs) {
    // Check status register to ensure data is for mouse?
    // Bit 5 (0x20) = 1 means AUX.
    // If we handle both keyboard and mouse, checking this is good practice to avoid mixing packets.
    uint8_t status = port_byte_in(0x64);
    if (!(status & 0x01)) return; // Output buffer empty?
    // If bit 5 is 0, it's keyboard data that triggered IRQ1?
    // But we are in IRQ12 handler.
    // QEMU sometimes quirks this. We will just read 0x60.

    uint8_t b = port_byte_in(0x60);

    // Visual Debug: Draw a pixel at 0,0 that toggles color to prove interrupts work
    static int debug_color = 0;
    put_pixel(0, 0, (debug_color++) % 2 ? 0xFFFF0000 : 0xFF00FF00);

    switch (mouse_cycle) {
        case 0:
            // Bit 3 MUST be 1. But some mice might not set it?
            // If it's 0, we are definitely out of sync.
            // With USB Tablet, packet format might differ if not legacy mode?
            // Standard PS/2: Yovfl Xovfl Ysign Xsign 1 M R L
            // If we use USB Tablet, it should be absolute?
            // If BIOS emulation is active, it sends standard relative PS/2 packets.
            // Let's assume standard PS/2 for now.
            if ((b & 0x08) == 0) {
                // Try to resync? Skip?
                // For now, return and wait for next byte hoping it's the start
                // Or maybe we just accept it if it's 0 but others are valid?
                // Let's relax this check slightly or just return.
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

            uint8_t flags = mouse_byte[0];

            // Overflow handling
            if (flags & 0xC0) return; // Overflow X or Y

            int32_t rel_x = (int32_t)mouse_byte[1];
            int32_t rel_y = (int32_t)mouse_byte[2];

            // If we cast to int8_t first, sign extension happens automatically?
            // Yes, (int32_t)(int8_t)x works.
            // But let's be explicit with flags
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
