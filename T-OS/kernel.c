#include "bootinfo.h"
#include "compositor.h"
#include "idt.h"
#include "t_hal_pci.h"
#include "t_hal_gpu.h"
#include "gui/t_ps2_mouse.h"
#include "nsas_scheduler.h"
#include "sched.h"
#include "serial.h"
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
static int scheduler_online = 0;

void timer_handler() {
  timer_ticks++;

  // NPU affinity accounting stays where it was; it does not touch the CPU.
  nsas_schedule();

  int resched = scheduler_online ? sched_tick() : 0;

  // The EOI has to go out before the context switch: once we switch stacks we
  // are running some other thread and will not come back to this instruction
  // until that thread is scheduled again, and the PIC would stay masked in the
  // meantime.
  outb(0x20, 0x20);

  if (resched)
    sched_preempt();
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

// Simple bump allocator for kmalloc.
//
// 16 MB rather than 4: the compositor's double buffer alone is ~4 MB at
// 1280x800, which used to leave only a few tens of kilobytes for every thread
// stack in the system. Allocations are never freed, so headroom is cheap
// insurance against a silent out-of-memory that returns NULL to a caller that
// does not check.
static uint8_t heap[1024 * 1024 * 16];
static uint64_t heap_ptr = 0;

void *kmalloc(uint64_t size) {
  // Keep allocations 16-byte aligned; thread stacks and the double buffer both
  // care.
  size = (size + 15) & ~(uint64_t)15;
  if (heap_ptr + size > sizeof(heap)) {
    kprintf("kmalloc: out of memory requesting %lu bytes (%lu of %lu used)\n",
            (unsigned long)size, (unsigned long)heap_ptr,
            (unsigned long)sizeof(heap));
    return (void *)0;
  }
  void *ptr = &heap[heap_ptr];
  heap_ptr += size;
  return ptr;
}

// Tiny text builders for the on-screen status panel. The compositor takes
// plain strings, and kprintf goes to the serial port, so the desktop needs its
// own formatting.
static int str_append(char *dst, int pos, int cap, const char *s) {
  while (*s && pos < cap - 1)
    dst[pos++] = *s++;
  return pos;
}

static int u64_append(char *dst, int pos, int cap, uint64_t v) {
  char digits[24];
  int d = 0;
  if (v == 0)
    digits[d++] = '0';
  while (v && d < (int)sizeof(digits)) {
    digits[d++] = (char)('0' + (v % 10));
    v /= 10;
  }
  while (d-- > 0 && pos < cap - 1)
    dst[pos++] = digits[d];
  return pos;
}

// Desktop thread. It is the only writer to the compositor, which keeps the
// on-screen terminal free of interleaving from the other threads, and it
// sleeps between frames so it behaves like the interactive workload it is.
static void thread_desktop(void *arg) {
  (void)arg;

  for (;;) {
    compositor_update_mouse();
    compositor_draw_desktop(); // repaints the desktop and resets the cursor

    // Draw the status panel into the same frame, before the swap: the next
    // frame's draw_desktop() would otherwise wipe it straight away.
    char line[96];
    int n = 0;
    sched_thread_t *cur = sched_current();

    n = str_append(line, n, sizeof(line), "T-OS scheduler   tick ");
    n = u64_append(line, n, sizeof(line), sched_ticks());
    n = str_append(line, n, sizeof(line), "   switches ");
    n = u64_append(line, n, sizeof(line), sched_total_switches());
    n = str_append(line, n, sizeof(line), "\n");
    n = str_append(line, n, sizeof(line), "running: ");
    n = str_append(line, n, sizeof(line), cur->name);
    n = str_append(line, n, sizeof(line), "   runnable ");
    n = u64_append(line, n, sizeof(line), sched_runnable_count());
    n = str_append(line, n, sizeof(line), "\n");
    line[n] = 0;
    compositor_print(line, 0x00FF88);

    compositor_draw_cursor();
    compositor_swap_buffers();

    sched_sleep(16); // ~60 Hz at a 1 kHz tick
  }
}

void kmain(BootInfo *bi) {
  uint32_t *fb = (uint32_t *)bi->fb_base;

  // Bring the serial console up first so everything after this point is
  // visible on a headless VM (qemu -serial stdio).
  serial_init();
  kprintf("\n=== T-OS kernel entry ===\n");
  kprintf("framebuffer %ux%u pitch=%u at %p\n", bi->fb_width, bi->fb_height,
          bi->fb_pitch, (void *)bi->fb_base);

  // Clear screen to black
  uint32_t num_pixels = bi->fb_size / 4;
  for (uint32_t i = 0; i < num_pixels; i++) {
    fb[i] = 0x00000000;
  }

  // Initialize IDT and remap PIC
  idt_init();
  idt_install_exceptions(); // report faults instead of triple-faulting
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

  // Initialize NSAS
  nsas_init();

  // Run NSAS Tests
  void run_nsas_tests();
  run_nsas_tests();

  // Create simulated tasks (SMP Multi-threading)
  pcb_t* p_main = create_process(0x1000000);
  create_thread(p_main, (void*)0x1000); // Thread 1

  // DOOM Initialization
  uint8_t* doom_wad_data = (uint8_t*)0x2000000; // Expected address from bootloader
  uint32_t doom_wad_size = 4196020;             // size of shareware WAD
  void D_DoomMain(void);
  compositor_print("DOOM: Starting Game Core...\n", 0xFF00FF);
  // In a real port, we'd spawn a thread:
  // create_thread(p_main, D_DoomMain);

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
  kprintf("boot complete, bringing up the CPU scheduler\n");

  // ---- CPU scheduler ----
  //
  // From here on kmain is no longer a loop: it becomes the idle thread. Every
  // other piece of work - the desktop, the demo workload - is a kernel thread
  // that the timer interrupt preempts.
  sched_init();
  void sched_demo_start(void);
  sched_demo_start();
  sched_create_thread("desktop", thread_desktop, (void *)0, 1);

  scheduler_online = 1;
  __asm__ volatile("sti"); // enable interrupts: IRQ0 now drives preemption

  sched_start(); // returns only if sched_stop() is ever called

  kprintf("scheduler stopped; halting\n");
  for (;;)
    __asm__ volatile("hlt");
}
