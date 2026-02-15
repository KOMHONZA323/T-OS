#include "idt.h"
#include "isr.h"
#include "../drivers/ports.h"
#include "../drivers/utils.h"
#include "../drivers/screen.h"

// Defined in timer.c for now, or just extern
extern registers_t* timer_callback(registers_t *regs);
extern void keyboard_handler(registers_t *regs); // Changed from keyboard_callback to keyboard_handler
extern void mouse_handler(registers_t *regs);
extern void syscall_handler(registers_t *regs);
extern uint32_t tick;

void isr_handler(registers_t *regs) {
    if (regs->int_no == 14) { // Page Fault
        uint32_t cr2;
        asm volatile("mov %%cr2, %0" : "=r"(cr2));
        kprint("PAGE FAULT! Addr: ");
        char buf[32];
        int_to_ascii(cr2, buf); // Decimal only for now in utils? Or implement hex?
        // We have int_to_ascii in utils.c which is decimal.
        // It's better than nothing.
        kprint(buf);
        kprint("\n");
        while(1);
    }
}

registers_t* irq_handler(registers_t *regs) {
    registers_t* ret = regs;

    // Handle IRQs
    // IRQ0 (32) is Timer
    if (regs->int_no == 32) {
        ret = timer_callback(regs);
    }
    // IRQ1 (33) is Keyboard
    if (regs->int_no == 33) { keyboard_handler(regs); }

    // IRQ12 (44) is Mouse
    if (regs->int_no == 44) { mouse_handler(regs); }

    // Send EOI to PICs
    if (regs->int_no >= 40) {
        port_byte_out(0xA0, 0x20); // Slave
    }
    port_byte_out(0x20, 0x20); // Master

    return ret;
}

void init_idt() {
    set_idt();

    // Remap the PIC
    port_byte_out(0x20, 0x11);
    port_byte_out(0xA0, 0x11);
    port_byte_out(0x21, 0x20);
    port_byte_out(0xA1, 0x28);
    port_byte_out(0x21, 0x04);
    port_byte_out(0xA1, 0x02);
    port_byte_out(0x21, 0x01);
    port_byte_out(0xA1, 0x01);
    // Unmask Timer (IRQ0) and Keyboard (IRQ1) and Slave Cascade (IRQ2)
    // 0xFA = 1111 1010 (Timer=0, Keyboard=1, Slave=2)
    // 1111 1000 = F8
    port_byte_out(0x21, 0xF8);

    // Unmask Mouse (IRQ12 -> Slave 4)
    // 1110 1111 = EF
    port_byte_out(0xA1, 0xEF);

    // Set gates
    // Exceptions
    set_idt_gate(0, (uint32_t)isr0);
    set_idt_gate(1, (uint32_t)isr1);
    set_idt_gate(2, (uint32_t)isr2);
    set_idt_gate(3, (uint32_t)isr3);
    set_idt_gate(4, (uint32_t)isr4);
    set_idt_gate(5, (uint32_t)isr5);
    set_idt_gate(6, (uint32_t)isr6);
    set_idt_gate(7, (uint32_t)isr7);
    set_idt_gate(8, (uint32_t)isr8);
    set_idt_gate(9, (uint32_t)isr9);
    set_idt_gate(10, (uint32_t)isr10);
    set_idt_gate(11, (uint32_t)isr11);
    set_idt_gate(12, (uint32_t)isr12);
    set_idt_gate(13, (uint32_t)isr13);
    set_idt_gate(14, (uint32_t)isr14);
    set_idt_gate(15, (uint32_t)isr15);
    set_idt_gate(16, (uint32_t)isr16);
    set_idt_gate(17, (uint32_t)isr17);
    set_idt_gate(18, (uint32_t)isr18);
    set_idt_gate(19, (uint32_t)isr19);
    set_idt_gate(20, (uint32_t)isr20);
    set_idt_gate(21, (uint32_t)isr21);
    set_idt_gate(22, (uint32_t)isr22);
    set_idt_gate(23, (uint32_t)isr23);
    set_idt_gate(24, (uint32_t)isr24);
    set_idt_gate(25, (uint32_t)isr25);
    set_idt_gate(26, (uint32_t)isr26);
    set_idt_gate(27, (uint32_t)isr27);
    set_idt_gate(28, (uint32_t)isr28);
    set_idt_gate(29, (uint32_t)isr29);
    set_idt_gate(30, (uint32_t)isr30);
    set_idt_gate(31, (uint32_t)isr31);

    // IRQs
    set_idt_gate(32, (uint32_t)irq0);
    set_idt_gate(33, (uint32_t)irq1);
    set_idt_gate(34, (uint32_t)irq2);
    set_idt_gate(35, (uint32_t)irq3);
    set_idt_gate(36, (uint32_t)irq4);
    set_idt_gate(37, (uint32_t)irq5);
    set_idt_gate(38, (uint32_t)irq6);
    set_idt_gate(39, (uint32_t)irq7);
    set_idt_gate(40, (uint32_t)irq8);
    set_idt_gate(41, (uint32_t)irq9);
    set_idt_gate(42, (uint32_t)irq10);
    set_idt_gate(43, (uint32_t)irq11);
    set_idt_gate(44, (uint32_t)irq12);
    set_idt_gate(45, (uint32_t)irq13);
    set_idt_gate(46, (uint32_t)irq14);
    set_idt_gate(47, (uint32_t)irq15);
    extern void isr128();
    set_idt_gate(128, (uint32_t)isr128);
}
