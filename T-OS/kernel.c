#include "bootinfo.h"
#include "compositor.h"
#include "idt.h"
#include "t_hal_pci.h"
#include "t_hal_gpu.h"
#include "gui/t_ps2_mouse.h"
#include <stdint.h>

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

void draw_char(BootInfo *bi, char c, uint32_t x, uint32_t y, uint32_t color,
               int scale) {
  if (c > 127 || c < 0 || scale <= 0)
    return;
  uint32_t *fb = (uint32_t *)bi->fb_base;
  uint32_t fb_width = bi->fb_width;
  uint32_t fb_height = bi->fb_height;
  uint32_t fb_pitch = bi->fb_pitch;

  uint8_t *glyph = font[(uint8_t)c];
  for (int i = 0; i < 8; i++) {
    uint8_t row_data = glyph[i];
    if (!row_data)
      continue;

    uint32_t base_py = y + i * scale;
    if (base_py >= fb_height)
      break;
    uint32_t py_max = base_py + scale;
    if (py_max > fb_height)
      py_max = fb_height;

    for (int j = 0; j < 8; j++) {
      if ((row_data >> j) & 1) {
        uint32_t base_px = x + (7 - j) * scale;
        if (base_px >= fb_width)
          continue;
        uint32_t px_max = base_px + scale;
        if (px_max > fb_width)
          px_max = fb_width;

        uint32_t *row_ptr = &fb[base_py * fb_pitch];
        for (uint32_t py = base_py; py < py_max; py++) {
          for (uint32_t px = base_px; px < px_max; px++) {
            row_ptr[px] = color;
          }
          row_ptr += fb_pitch;
        }
      }
    }
  }
}

void draw_string(BootInfo *bi, const char *s, uint32_t x, uint32_t y,
                 uint32_t color, int scale) {
  while (*s) {
    draw_char(bi, *s, x, y, color, scale);
    x += 8 * scale;
    s++;
  }
}

static inline void outb(uint16_t port, uint8_t val) {
  __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
  uint8_t ret;
  __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
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
    __asm__ volatile("hlt");
  }
}

void kpanic(BootInfo *bi) {
  uint32_t *fb = (uint32_t *)bi->fb_base;

  // Clear screen to red for panic
  uint32_t num_pixels = bi->fb_size / 4;
  for (uint32_t i = 0; i < num_pixels; i++) {
    fb[i] = 0x00FF0000; // Red color
  }

  if (bi->fb_base != 0 && bi->fb_width > 0 && bi->fb_height > 0) {
    uint32_t screen_center_x = bi->fb_width / 2;
    uint32_t screen_center_y = bi->fb_height / 2;

    uint32_t panic_len = 5 * 8 * 5; // 5 chars, 8 pixels wide, 5x scale
    draw_string(bi, "PANIC", screen_center_x - panic_len / 2, screen_center_y,
                0xFFFFFFFF, 5); // White text
  }

  // Disable interrupts on panic
  __asm__ volatile("cli");
  while (1) {
    __asm__ __volatile__("hlt");
  }
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

  outb(
      0x21,
      4); // ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
  outb(0x80, 0);
  outb(0xA1, 2); // ICW3: tell Slave PIC its cascade identity (0000 0010)
  outb(0x80, 0);

  outb(0x21, 0x01); // ICW4: have the PICs use 8086 mode (and not 8080 mode)
  outb(0x80, 0);
  outb(0xA1, 0x01);
  outb(0x80, 0);

  // Disable all interrupts except IRQ0 (timer), IRQ1 (keyboard) and IRQ12 (mouse)
  outb(0x21, 0xFC); // 1111 1100 (Timer and Keyboard on)
  outb(0xA1, 0xEF); // 1110 1111 (Mouse on IRQ 12)
}

static void pit_init(uint32_t frequency) {
  uint32_t divisor = 1193180 / frequency;
  outb(0x43,
       0x36); // Command byte: Channel 0, LSB/MSB, Mode 3 (Square Wave), Binary
  outb(0x40, (uint8_t)(divisor & 0xFF));        // LSB
  outb(0x40, (uint8_t)((divisor >> 8) & 0xFF)); // MSB
}

extern void irq0_isr(); // from entry.s
extern void irq1_isr(); // from entry.s
extern void irq12_isr(); // from entry.s

void keyboard_handler() {
  uint8_t scancode = inb(0x60);
  compositor_handle_interrupt(scancode);
  // Send EOI to PIC
  outb(0x20, 0x20);
}

// Simple bump allocator for kmalloc
static uint8_t heap[1024 * 1024 * 4]; // 4MB heap
static uint64_t heap_ptr = 0;

void *kmalloc(uint64_t size) {
  if (heap_ptr + size > sizeof(heap))
    return (void *)0;
  void *ptr = &heap[heap_ptr];
  heap_ptr += size;
  return ptr;
}

void kmain(BootInfo *bi) {
  uint32_t *fb = (uint32_t *)bi->fb_base;

  // Clear screen to black
  uint32_t num_pixels = bi->fb_size / 4;
  for (uint32_t i = 0; i < num_pixels; i++) {
    fb[i] = 0x00000000;
  }

  // Initialize IDT and remap PIC
  idt_init();
  pic_remap();

  // Set IRQ0 (vector 32) to our ISR handler. Attributes 0x8E = Present, Ring 0,
  // Interrupt Gate
  idt_set_descriptor(32, irq0_isr, 0x8E);
  // Set IRQ1 (vector 33) to our ISR handler.
  idt_set_descriptor(33, irq1_isr, 0x8E);
  // Set IRQ12 (vector 44) to our ISR handler.
  idt_set_descriptor(44, irq12_isr, 0x8E);

  // Initialize PIT to 1000 Hz
  pit_init(1000);

  // Initialize Mouse
  ps2_mouse_init();

  // Initialize Compositor
  compositor_init((GOP_Info *)bi);
  compositor_print("T-OS Kernel Started\n", 0x00FF00);

  // PCI & GPU initialization
  uint8_t gpu_bus, gpu_slot;
  pci_find_gpu(&gpu_bus, &gpu_slot);
  if (gpu_bus != 0xFF) {
      compositor_print("GPU Found via PCI\n", 0x00FF00);
      gpu_init(gpu_bus, gpu_slot);
      compositor_print("GPU Ring Buffer Initialized\n", 0x00FF00);
  } else {
      compositor_print("No PCI GPU found\n", 0xFF0000);
  }

  // Enable interrupts
  __asm__ volatile("sti");

  // If we have a valid framebuffer, draw the success message
  if (bi->fb_base != 0 && bi->fb_width > 0 && bi->fb_height > 0) {
    uint32_t screen_center_x = bi->fb_width / 2;
    uint32_t screen_center_y = bi->fb_height / 2;

    // "BOOT"
    uint32_t boot_len = 4 * 8 * 5; // 4 chars, 8 pixels wide, 5x scale
    draw_string(bi, "BOOT", screen_center_x - boot_len / 2,
                screen_center_y - 8 * 5, 0xFF00FF00, 5);

    // "SUCCESS"
    uint32_t success_len = 7 * 8 * 5; // 7 chars
    draw_string(bi, "SUCCESS", screen_center_x - success_len / 2,
                screen_center_y, 0xFF00FF00, 5);
  }

  compositor_print("Boot Success\n", 0x00FF00);

  // Main Compositor Loop
  while (1) {
    compositor_update_mouse();
    compositor_draw_desktop();
    compositor_draw_cursor();
    compositor_swap_buffers();
    __asm__ volatile("hlt");
  }
}
