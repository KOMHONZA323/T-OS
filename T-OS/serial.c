#include "serial.h"
#include "compositor.h"

#define COM1 0x3F8

static int serial_ready = 0;
static uint32_t mirror_color = 0xCCCCCC;
static int mirror_enabled = 0;

static inline void outb(uint16_t port, uint8_t val) {
  __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
  uint8_t ret;
  __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
  return ret;
}

void serial_init(void) {
  outb(COM1 + 1, 0x00); // disable interrupts
  outb(COM1 + 3, 0x80); // enable DLAB
  outb(COM1 + 0, 0x01); // divisor low  (115200 baud)
  outb(COM1 + 1, 0x00); // divisor high
  outb(COM1 + 3, 0x03); // 8 bits, no parity, one stop bit
  outb(COM1 + 2, 0xC7); // FIFO on, clear, 14-byte threshold
  outb(COM1 + 4, 0x03); // RTS/DSR set
  serial_ready = 1;
}

void serial_putc(char c) {
  if (!serial_ready)
    return;
  if (c == '\n')
    serial_putc('\r');
  // Wait for the transmit holding register to drain.
  while ((inb(COM1 + 5) & 0x20) == 0)
    ;
  outb(COM1, (uint8_t)c);
}

void serial_write(const char *s) {
  while (*s)
    serial_putc(*s++);
}

void kprintf_set_color(uint32_t color) { mirror_color = color; }

void kprintf_set_mirror(int enabled) { mirror_enabled = enabled; }

static void emit(const char *s) {
  serial_write(s);
  if (mirror_enabled)
    compositor_print(s, mirror_color);
}

static void emit_char(char c) {
  char buf[2] = {c, 0};
  emit(buf);
}

// Render an integer into buf (NUL-terminated) and return its length.
static int render_int(char *buf, int cap, uint64_t v, unsigned base, int neg) {
  const char *digits = "0123456789abcdef";
  char tmp[24];
  int n = 0;

  if (v == 0)
    tmp[n++] = '0';
  while (v && n < (int)sizeof(tmp)) {
    tmp[n++] = digits[v % base];
    v /= base;
  }
  if (neg && n < (int)sizeof(tmp))
    tmp[n++] = '-';

  int len = 0;
  while (n-- > 0 && len < cap - 1)
    buf[len++] = tmp[n];
  buf[len] = 0;
  return len;
}

// Emit `s` padded to `width`; negative width means left-justified.
static void emit_padded(const char *s, int len, int width, char pad_char,
                        int left) {
  int fill = width - len;
  if (!left)
    for (int i = 0; i < fill; i++)
      emit_char(pad_char);
  emit(s);
  if (left)
    for (int i = 0; i < fill; i++)
      emit_char(' ');
}

void kvprintf(const char *fmt, va_list ap) {
  char buf[32];

  for (const char *p = fmt; *p; p++) {
    if (*p != '%') {
      emit_char(*p);
      continue;
    }

    p++;
    if (*p == 0)
      break;

    // Flags.
    int left = 0;
    char pad_char = ' ';
    for (;;) {
      if (*p == '-') {
        left = 1;
        p++;
      } else if (*p == '0') {
        pad_char = '0';
        p++;
      } else {
        break;
      }
    }

    // Width.
    int width = 0;
    while (*p >= '0' && *p <= '9')
      width = width * 10 + (*p++ - '0');

    // Length modifier; long and long long are the same width here.
    int is_long = 0;
    while (*p == 'l' || *p == 'z') {
      is_long = 1;
      p++;
    }

    switch (*p) {
    case 's': {
      const char *s = va_arg(ap, const char *);
      if (!s)
        s = "(null)";
      int len = 0;
      while (s[len])
        len++;
      emit_padded(s, len, width, ' ', left);
      break;
    }
    case 'c': {
      buf[0] = (char)va_arg(ap, int);
      buf[1] = 0;
      emit_padded(buf, 1, width, ' ', left);
      break;
    }
    case 'd':
    case 'i': {
      int64_t v = is_long ? va_arg(ap, int64_t) : (int64_t)va_arg(ap, int);
      int neg = v < 0;
      int len = render_int(buf, sizeof(buf), (uint64_t)(neg ? -v : v), 10, neg);
      emit_padded(buf, len, width, pad_char, left);
      break;
    }
    case 'u': {
      uint64_t v =
          is_long ? va_arg(ap, uint64_t) : (uint64_t)va_arg(ap, unsigned int);
      int len = render_int(buf, sizeof(buf), v, 10, 0);
      emit_padded(buf, len, width, pad_char, left);
      break;
    }
    case 'x': {
      uint64_t v =
          is_long ? va_arg(ap, uint64_t) : (uint64_t)va_arg(ap, unsigned int);
      int len = render_int(buf, sizeof(buf), v, 16, 0);
      emit_padded(buf, len, width, pad_char, left);
      break;
    }
    case 'p': {
      int len = render_int(buf, sizeof(buf), (uint64_t)va_arg(ap, void *), 16, 0);
      emit("0x");
      emit_padded(buf, len, width, pad_char, left);
      break;
    }
    case '%':
      emit_char('%');
      break;
    default:
      emit_char('%');
      emit_char(*p);
      break;
    }
  }
}

void kprintf(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  kvprintf(fmt, ap);
  va_end(ap);
}
