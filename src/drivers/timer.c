#include "timer.h"
#include "ports.h"
#include "../kernel/isr.h"
#include "../kernel/task/scheduler.h"

uint32_t tick = 0;

registers_t* timer_callback(registers_t *regs) {
    tick++;
    return schedule(regs);
}

uint32_t get_tick_count() {
    return tick;
}

void init_timer(uint32_t frequency) {
    uint32_t divisor = 1193180 / frequency;

    port_byte_out(0x43, 0x36);

    uint8_t l = (uint8_t)(divisor & 0xFF);
    uint8_t h = (uint8_t)( (divisor>>8) & 0xFF );

    port_byte_out(0x40, l);
    port_byte_out(0x40, h);
}
