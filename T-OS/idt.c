#include "idt.h"
#include "sched.h"
#include "serial.h"

__attribute__((aligned(0x10)))
idt_entry_t idt[256];
static idtr_t idtr;

extern void *isr_stub_table[32];
extern void irq_default_isr(void);

static const char *const exception_names[32] = {
    "divide error",
    "debug",
    "NMI",
    "breakpoint",
    "overflow",
    "BOUND range exceeded",
    "invalid opcode",
    "device not available",
    "double fault",
    "coprocessor segment overrun",
    "invalid TSS",
    "segment not present",
    "stack-segment fault",
    "general protection fault",
    "page fault",
    "reserved",
    "x87 FPU error",
    "alignment check",
    "machine check",
    "SIMD FP exception",
    "virtualisation exception",
    "control protection",
    "reserved", "reserved", "reserved", "reserved",
    "reserved", "reserved",
    "hypervisor injection",
    "VMM communication",
    "security exception",
    "reserved",
};

void idt_install_exceptions(void) {
    for (int v = 0; v < 32; v++)
        idt_set_descriptor((uint8_t)v, isr_stub_table[v], 0x8E);

    // Everything above the exception range gets a stub that just acknowledges
    // the PICs. Callers overwrite the vectors they actually handle.
    for (int v = 32; v < 256; v++)
        idt_set_descriptor((uint8_t)v, (void *)irq_default_isr, 0x8E);
}

// Last stop before a triple fault: print enough to identify the faulting code
// and which thread was on the CPU, then stop the machine.
void exception_handler(exception_frame_t *f) {
    __asm__ volatile("cli");

    uint64_t cr2;
    __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));

    const char *name =
        f->vector < 32 ? exception_names[f->vector] : "unknown";

    kprintf("\n\n*** CPU EXCEPTION %lu: %s ***\n", (unsigned long)f->vector,
            name);
    kprintf("  error=%lx  rip=%lx  cs=%lx  rflags=%lx\n",
            (unsigned long)f->error_code, (unsigned long)f->rip,
            (unsigned long)f->cs, (unsigned long)f->rflags);
    kprintf("  rsp=%lx  ss=%lx  cr2=%lx\n", (unsigned long)f->rsp,
            (unsigned long)f->ss, (unsigned long)cr2);
    kprintf("  rax=%lx rbx=%lx rcx=%lx rdx=%lx\n", (unsigned long)f->rax,
            (unsigned long)f->rbx, (unsigned long)f->rcx,
            (unsigned long)f->rdx);
    kprintf("  rsi=%lx rdi=%lx rbp=%lx\n", (unsigned long)f->rsi,
            (unsigned long)f->rdi, (unsigned long)f->rbp);

    sched_thread_t *cur = sched_current();
    if (cur) {
        kprintf("  thread %u '%s' state=%d prio=%u stack=%p..%p\n", cur->tid,
                cur->name, (int)cur->state, cur->prio, cur->stack_base,
                (void *)((uint64_t)cur->stack_base + cur->stack_size));
        // A fault with rsp just below the stack base is the classic overflow.
        if (cur->stack_base && f->rsp < (uint64_t)cur->stack_base)
            kprintf("  >>> rsp is below the thread stack: STACK OVERFLOW\n");
    }

    kprintf("*** halted ***\n");
    for (;;)
        __asm__ volatile("hlt");
}

void idt_set_descriptor(uint8_t vector, void* isr, uint8_t flags) {
    idt_entry_t* descriptor = &idt[vector];

    uint64_t addr = (uint64_t)isr;
    uint16_t cs;
    __asm__ volatile("mov %%cs, %0" : "=r"(cs)); // read current code segment

    descriptor->isr_low    = (uint16_t)(addr & 0xFFFF);
    descriptor->kernel_cs  = cs;
    descriptor->ist        = 0;
    descriptor->attributes = flags;
    descriptor->isr_mid    = (uint16_t)((addr >> 16) & 0xFFFF);
    descriptor->isr_high   = (uint32_t)((addr >> 32) & 0xFFFFFFFF);
    descriptor->reserved   = 0;
}

void idt_init() {
    idtr.base = (uint64_t)&idt[0];
    idtr.limit = (uint16_t)sizeof(idt_entry_t) * 256 - 1;

    // Load IDT
    __asm__ volatile ("lidt %0" : : "m"(idtr));
}
