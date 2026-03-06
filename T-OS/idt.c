#include "idt.h"

__attribute__((aligned(0x10)))
idt_entry_t idt[256];
static idtr_t idtr;

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
