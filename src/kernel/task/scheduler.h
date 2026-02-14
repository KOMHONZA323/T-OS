#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "../isr.h"

void init_scheduler();
registers_t* schedule(registers_t *regs);

#endif
