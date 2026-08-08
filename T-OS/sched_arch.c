#include "sched.h"

// Defined in switch.s.
extern void sched_thread_trampoline(void);

// Build the stack image that sched_arch_switch() will pop when this thread is
// dispatched for the first time. The order below must mirror the pop sequence
// in switch.s exactly.
uint64_t sched_arch_init_stack(sched_thread_t *t) {
  uint64_t top = (uint64_t)t->stack_base + t->stack_size;
  top &= ~(uint64_t)15; // 16-byte align the stack top

  uint64_t *sp = (uint64_t *)top;

  *--sp = (uint64_t)sched_thread_trampoline; // 'return' address
  *--sp = 0;                                 // rbp
  *--sp = 0;                                 // rbx
  *--sp = (uint64_t)t->arg;                  // r12 -> trampoline's rdi
  *--sp = (uint64_t)t->entry;                // r13 -> call target
  *--sp = 0;                                 // r14
  *--sp = 0;                                 // r15
  *--sp = 0x202;                             // rflags: IF set, bit 1 reserved

  return (uint64_t)sp;
}
