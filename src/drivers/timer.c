#include "timer.h"
#include "../kernel/idt.h" // For IRQ handler setup (if needed via set_idt_gate here, or in isr.c)
#include "ports.h"

// We need a way to register the callback.
// For now, we will implement the irq_handler dispatch logic here or in a separate isr.c
// Since we don't have isr.c yet, I will create a basic isr.c as part of the integration
// OR I can put the C handler logic in kernel.c or here.
// Let's create `src/kernel/isr.c` properly in the next step to handle dispatch.
// But first, let's implement the timer logic.

#include "../kernel/isr.h"

uint32_t tick = 0;

void timer_callback(registers_t *regs) {
    tick++;
}

uint32_t get_tick_count() {
    return tick;
}

void init_timer(uint32_t frequency) {
    // The value we send to the PIT is the value to divide it's input clock
    // (1193180 Hz) by, to get our required frequency.
    // Divisor must fit in 16 bits.
    uint32_t divisor = 1193180 / frequency;

    // Send the command byte.
    port_byte_out(0x43, 0x36);

    // Split divisor into low and high bytes.
    uint8_t l = (uint8_t)(divisor & 0xFF);
    uint8_t h = (uint8_t)( (divisor>>8) & 0xFF );

    // Send the frequency divisor.
    port_byte_out(0x40, l);
    port_byte_out(0x40, h);
}
