#ifndef SERIAL_H
#define SERIAL_H

#include <stdarg.h>
#include <stdint.h>

// 16550 UART on COM1. Gives the kernel a text console that survives on a
// headless VM (qemu -serial stdio), which is where the scheduler traces go.
void serial_init(void);
void serial_putc(char c);
void serial_write(const char *s);

// Minimal formatted print: %s %c %d %i %u %x %p %% and %lu/%lx/%ld.
// Output goes to COM1 and, once the compositor is up, to the on-screen
// terminal as well.
void kprintf(const char *fmt, ...);
void kvprintf(const char *fmt, va_list ap);

// Colour used for the compositor mirror of kprintf output.
void kprintf_set_color(uint32_t color);

// Enable/disable the on-screen mirror (the compositor is far slower than the
// UART, so timing-sensitive traces can turn it off).
void kprintf_set_mirror(int enabled);

#endif // SERIAL_H
