#include <stdint.h>
#include "bootinfo.h"
#include "idt.h"

// A simple 8x8 font
static uint8_t font[128][8] = {
    // Basic characters for "SUCCESS"
    [0x41] = {0x18, 0x3c, 0x66, 0x66, 0x7e, 0x66, 0x66, 0}, // A
    [0x42] = {0x7e, 0x18, 0x18, 0x18, 0x18, 0x18, 0x7e, 0}, // B
    [0x43] = {0x3c, 0x66, 0x06, 0x06, 0x06, 0x66, 0x3c, 0}, // C
    [0x45] = {0x7e, 0x06, 0x06, 0x3e, 0x06, 0x06, 0x7e, 0}, // E
    [0x49] = {0x7e, 0x18, 0x18, 0x18, 0x18, 0x18, 0x7e, 0}, // I
    [0x4e] = {0x66, 0x76, 0x7e, 0x6e, 0x66, 0x66, 0x66, 0}, // N
    [0x4f] = {0x3c, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3c, 0}, // O
    [0x50] = {0x7e, 0x66, 0x66, 0x7e, 0x06, 0x06, 0x06, 0}, // P
    [0x53] = {0x3c, 0x66, 0x06, 0x3c, 0x60, 0x66, 0x3c, 0}, // S
    [0x54] = {0x7e, 0x7e, 0x18, 0x18, 0x18, 0x18, 0x18, 0}, // T
    [0x55] = {0x66, 0x66, 0x66, 0x66, 0x66, 0x3e, 0x3c, 0}, // U
};

void draw_char(BootInfo* bi, char c, uint32_t x, uint32_t y, uint32_t color, int scale) {
    if (c > 127) return;
    uint32_t* fb = (uint32_t*)bi->fb_base;

    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            if ((font[(uint8_t)c][i] >> j) & 1) {
                for (int k = 0; k < scale; k++) {
                    for (int l = 0; l < scale; l++) {
                        uint32_t px = x + (8 - j - 1) * scale + k;
                        uint32_t py = y + i * scale + l;
                        if (px < bi->fb_width && py < bi->fb_height) {
                            fb[py * bi->fb_pitch + px] = color;
                        }
                    }
                }
            }
        }
    }
}

void draw_string(BootInfo* bi, const char* s, uint32_t x, uint32_t y, uint32_t color, int scale) {
    while (*s) {
        draw_char(bi, *s, x, y, color, scale);
        x += 8 * scale;
        s++;
    }
}

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

volatile uint64_t timer_ticks = 0;

void timer_handler() {
    timer_ticks++;
    // Send EOI to PIC master
    outb(0x20, 0x20);
}

void delay(uint64_t milliseconds) {
    uint64_t target_ticks = timer_ticks + milliseconds;
    while (timer_ticks < target_ticks) {
        __asm__ volatile ("hlt");
    }
}

void kpanic(BootInfo* bi) {
    uint32_t* fb = (uint32_t*)bi->fb_base;

    // Clear screen to red for panic
    for (uint32_t i = 0; i < bi->fb_size / 4; i++) {
        fb[i] = 0x00FF0000; // Red color
    }

    if (bi->fb_base != 0 && bi->fb_width > 0 && bi->fb_height > 0) {
        uint32_t screen_center_x = bi->fb_width / 2;
        uint32_t screen_center_y = bi->fb_height / 2;
        
        uint32_t panic_len = 5 * 8 * 5; // 5 chars, 8 pixels wide, 5x scale
        draw_string(bi, "PANIC", screen_center_x - panic_len / 2, screen_center_y, 0xFFFFFFFF, 5); // White text
    }

    // Disable interrupts on panic
    __asm__ volatile ("cli");
    while(1) { __asm__ __volatile__("hlt"); }
}

// Remap PIC to move IRQs from 0x00-0x0F to 0x20-0x2F (32-47)
static void pic_remap() {
    uint8_t a1, a2;
    a1 = inb(0x21); // save masks
    a2 = inb(0xA1);

    outb(0x20, 0x11); // starts the initialization sequence (in cascade mode)
    // io_wait(); // we'll just do a small outb to port 0x80 as wait
    outb(0x80, 0);
    outb(0xA0, 0x11);
    outb(0x80, 0);

    outb(0x21, 0x20); // ICW2: Master PIC vector offset (32)
    outb(0x80, 0);
    outb(0xA1, 0x28); // ICW2: Slave PIC vector offset (40)
    outb(0x80, 0);

    outb(0x21, 4);    // ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
    outb(0x80, 0);
    outb(0xA1, 2);    // ICW3: tell Slave PIC its cascade identity (0000 0010)
    outb(0x80, 0);

    outb(0x21, 0x01); // ICW4: have the PICs use 8086 mode (and not 8080 mode)
    outb(0x80, 0);
    outb(0xA1, 0x01);
    outb(0x80, 0);

    // Disable all interrupts except IRQ0 (timer)
    outb(0x21, 0xFE);
    outb(0xA1, 0xFF);
}

static void pit_init(uint32_t frequency) {
    uint32_t divisor = 1193180 / frequency;
    outb(0x43, 0x36); // Command byte: Channel 0, LSB/MSB, Mode 3 (Square Wave), Binary
    outb(0x40, (uint8_t)(divisor & 0xFF)); // LSB
    outb(0x40, (uint8_t)((divisor >> 8) & 0xFF)); // MSB
}

extern void irq0_isr(); // from entry.s

void kmain(BootInfo* bi) {
    uint32_t* fb = (uint32_t*)bi->fb_base;

    // Clear screen to black
    for (uint32_t i = 0; i < bi->fb_size / 4; i++) {
        fb[i] = 0x00000000;
    }

    // Initialize IDT and remap PIC
    idt_init();
    pic_remap();

    // Set IRQ0 (vector 32) to our ISR handler. Attributes 0x8E = Present, Ring 0, Interrupt Gate
    idt_set_descriptor(32, irq0_isr, 0x8E);

    // Initialize PIT to 1000 Hz
    pit_init(1000);

    // Enable interrupts
    __asm__ volatile ("sti");

    // If we have a valid framebuffer, draw the success message
    if (bi->fb_base != 0 && bi->fb_width > 0 && bi->fb_height > 0) {
        uint32_t screen_center_x = bi->fb_width / 2;
        uint32_t screen_center_y = bi->fb_height / 2;
        
        // "BOOT"
        uint32_t boot_len = 4 * 8 * 5; // 4 chars, 8 pixels wide, 5x scale
        draw_string(bi, "BOOT", screen_center_x - boot_len / 2, screen_center_y - 8*5, 0xFF00FF00, 5);
        
        // "SUCCESS"
        uint32_t success_len = 7 * 8 * 5; // 7 chars
        draw_string(bi, "SUCCESS", screen_center_x - success_len / 2, screen_center_y, 0xFF00FF00, 5);
    }
    
    // Delay for 10 seconds (approximate)
    delay(10000); // 10000 milliseconds = 10 seconds

    // Trigger panic
    kpanic(bi);

    // This part should not be reached after kpanic
    while(1) { __asm__ __volatile__("hlt"); }
}
