#include <stdint.h>
#include <stdbool.h>
#include "t_ps2_mouse.h"
#include "mouse_event.h"

// Phase 1: Hardware Abstraction & Interrupt Handling

// Inline Assembly for I/O Port Interaction
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// Wait for the PS/2 controller to be ready
static void mouse_wait(uint8_t type) {
    uint32_t timeout = 100000;
    if (type == 0) { // Wait for data to be available (Read)
        while (timeout--) {
            if ((inb(0x64) & 1) == 1) return;
        }
    } else { // Wait for the input buffer to be empty (Write)
        while (timeout--) {
            if ((inb(0x64) & 2) == 0) return;
        }
    }
}

// Phase 1, Req 2: Read data packet from I/O port
static void mouse_write(uint8_t data) {
    mouse_wait(1);
    outb(0x64, 0xD4); // Command 0xD4: Send byte to mouse
    mouse_wait(1);
    outb(0x60, data);
}

static uint8_t mouse_read() {
    mouse_wait(0);
    return inb(0x60);
}

void ps2_mouse_init() {
    uint8_t status;
    
    // Phase 1: Initialization via PS/2 Controller
    // Enable Auxiliary Device
    mouse_wait(1);
    outb(0x64, 0xA8);
    
    // Enable interrupts (bit 1 of controller configuration byte)
    mouse_wait(1);
    outb(0x64, 0x20); // Get Command Byte
    mouse_wait(0);
    status = (inb(0x60) | 2); // Set Bit 1: Enable IRQ 12
    mouse_wait(1);
    outb(0x64, 0x60); // Set Command Byte
    mouse_wait(1);
    outb(0x60, status);
    
    // Phase 2: Set default mouse settings
    mouse_write(0xF6); // Set Default Settings
    mouse_read();      // ACK
    
    // Enable data reporting
    mouse_write(0xF4);
    mouse_read();      // ACK

    mouse_queue_init();
}

// Phase 1, Req 3: Handle the packet state machine
void mouse_handler_v2() {
    uint8_t status = inb(0x64);
    
    // Check if data is available and if it's from the mouse (bit 5 of status register)
    if (!(status & 1) || !(status & 0x20)) {
        goto end;
    }

    static uint8_t packet[3];
    static uint8_t state = 0;
    
    uint8_t b = inb(0x60);
    
    // Sync bit: Bit 3 of byte 0 should always be 1
    if (state == 0 && !(b & 0x08)) {
        goto end;
    }
    
    packet[state++] = b;
    
    if (state == 3) {
        state = 0;
        
        // Phase 2: Data Parsing & Protocol Decoding
        MousePacket mp;
        
        // Extract button states (Bit 0: Left, Bit 1: Right, Bit 2: Middle)
        mp.left_button   = (packet[0] & 0x01) != 0;
        mp.right_button  = (packet[0] & 0x02) != 0;
        mp.middle_button = (packet[0] & 0x04) != 0;
        
        // Extract X/Y deltas with sign extension
        int32_t rel_x = (int32_t)packet[1];
        int32_t rel_y = (int32_t)packet[2];
        
        // Use sign bits from byte 0 (Bit 4: X sign, Bit 5: Y sign)
        if (packet[0] & 0x10) rel_x |= 0xFFFFFF00;
        if (packet[0] & 0x20) rel_y |= 0xFFFFFF00;
        
        mp.dx = rel_x;
        mp.dy = rel_y;
        
        // Phase 3: Push decoded packet to event queue
        mouse_queue_push(mp);
    }

end:
    // Send End-of-Interrupt (EOI) to PIC
    outb(0xA0, 0x20); // Slave PIC
    outb(0x20, 0x20); // Master PIC
}

// Legacy API compatibility
int ps2_mouse_poll(int* x, int* y, int* buttons) {
    MousePacket mp;
    if (mouse_queue_pop(&mp)) {
        *x = mp.dx;
        *y = mp.dy;
        *buttons = (mp.left_button ? 1 : 0) | (mp.right_button ? 2 : 0) | (mp.middle_button ? 4 : 0);
        return 1;
    }
    return 0;
}

// Bootloader stub for gui.c integration
void mouse_get_state(int32_t *x, int32_t *y, uint8_t *buttons,
                     int32_t screen_w, int32_t screen_h) {
    static int32_t bx = -1, by = -1; // -1 = uninitialised; will be centred on first call
    static uint8_t bb = 0;
    int dx, dy, btn;

    // Lazy-initialise to screen centre so the cursor starts in the middle.
    if (bx < 0) bx = screen_w / 2;
    if (by < 0) by = screen_h / 2;

    if (ps2_mouse_poll(&dx, &dy, &btn)) {
        bx += dx;
        by -= dy; // PS/2 Y-axis is inverted
        bb = (uint8_t)btn;
    }

    // Clamp here so the internal state never diverges from the displayed position.
    if (bx < 0) bx = 0;
    if (by < 0) by = 0;
    if (bx >= screen_w) bx = screen_w - 1;
    if (by >= screen_h) by = screen_h - 1;

    *x = bx;
    *y = by;
    *buttons = bb;
}
