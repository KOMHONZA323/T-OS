#ifndef IDT_H
#define IDT_H

#include <stdint.h>

typedef struct {
    uint16_t isr_low;      // The lower 16 bits of the ISR's address
    uint16_t kernel_cs;    // The GDT segment selector that the CPU will load into CS before calling the ISR
    uint8_t  ist;          // The IST in the TSS that the CPU will load into RSP; set to zero for now
    uint8_t  attributes;   // Type and attributes; see the IDT page
    uint16_t isr_mid;      // The higher 16 bits of the lower 32 bits of the ISR's address
    uint32_t isr_high;     // The higher 32 bits of the ISR's address
    uint32_t reserved;     // Set to zero
} __attribute__((packed)) idt_entry_t;

typedef struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) idtr_t;

// Register layout handed to exception_handler() by the stubs in exceptions.s.
// Field order matches the push order there; do not reorder one without the
// other.
typedef struct {
    uint64_t rax, rbx, rcx, rdx, rsi, rdi, rbp;
    uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
    uint64_t vector, error_code;
    uint64_t rip, cs, rflags, rsp, ss;
} __attribute__((packed)) exception_frame_t;

void idt_set_descriptor(uint8_t vector, void* isr, uint8_t flags);
void idt_init();

// Point vectors 0-31 at the fault reporter. Call after idt_init().
void idt_install_exceptions(void);

void exception_handler(exception_frame_t* f);

#endif
