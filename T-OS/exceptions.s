/*
 * CPU exception stubs.
 *
 * Without these the IDT has no entries for vectors 0-31, so the first fault
 * escalates straight through #GP and #DF to a triple fault and the machine
 * silently reboots - which tells you nothing. These stubs normalise the two
 * stack layouts the CPU can produce (with and without an error code) and hand
 * a single frame to exception_handler() in idt.c, which prints it and halts.
 */

.section .text

.macro ISR_NOERR vec
.global isr_stub_\vec
isr_stub_\vec:
    pushq $0                # dummy error code so every frame looks the same
    pushq $\vec
    jmp isr_common
.endm

.macro ISR_ERR vec
.global isr_stub_\vec
isr_stub_\vec:
                            # the CPU already pushed a real error code
    pushq $\vec
    jmp isr_common
.endm

ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7
ISR_ERR   8
ISR_NOERR 9
ISR_ERR   10
ISR_ERR   11
ISR_ERR   12
ISR_ERR   13
ISR_ERR   14
ISR_NOERR 15
ISR_NOERR 16
ISR_ERR   17
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_ERR   21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_ERR   29
ISR_ERR   30
ISR_NOERR 31

/*
 * Push order here is the reverse of the field order in exception_frame_t, so
 * that rsp ends up pointing at the start of the struct.
 */
isr_common:
    pushq %r15
    pushq %r14
    pushq %r13
    pushq %r12
    pushq %r11
    pushq %r10
    pushq %r9
    pushq %r8
    pushq %rbp
    pushq %rdi
    pushq %rsi
    pushq %rdx
    pushq %rcx
    pushq %rbx
    pushq %rax

    movq %rsp, %rdi
    call exception_handler

1:  cli                     # exception_handler does not return
    hlt
    jmp 1b

/*
 * Catch-all for interrupt vectors nothing else claims. A spurious IRQ7/IRQ15
 * arriving at a not-present gate would otherwise escalate #GP -> #DF -> triple
 * fault and silently reboot the machine. Acknowledge both PICs and return.
 */
.global irq_default_isr
.type irq_default_isr, @function
irq_default_isr:
    pushq %rax
    movb $0x20, %al
    outb %al, $0xA0         # EOI to the slave, harmless if it was the master
    outb %al, $0x20         # EOI to the master
    popq %rax
    iretq
.size irq_default_isr, . - irq_default_isr

/* Table so idt.c can install all 32 without 32 extern declarations. */
.section .rodata
.global isr_stub_table
.align 8
isr_stub_table:
    .quad isr_stub_0,  isr_stub_1,  isr_stub_2,  isr_stub_3
    .quad isr_stub_4,  isr_stub_5,  isr_stub_6,  isr_stub_7
    .quad isr_stub_8,  isr_stub_9,  isr_stub_10, isr_stub_11
    .quad isr_stub_12, isr_stub_13, isr_stub_14, isr_stub_15
    .quad isr_stub_16, isr_stub_17, isr_stub_18, isr_stub_19
    .quad isr_stub_20, isr_stub_21, isr_stub_22, isr_stub_23
    .quad isr_stub_24, isr_stub_25, isr_stub_26, isr_stub_27
    .quad isr_stub_28, isr_stub_29, isr_stub_30, isr_stub_31

.section .note.GNU-stack,"",@progbits
