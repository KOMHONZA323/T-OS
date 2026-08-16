# .text.boot is placed first by kernel.ld so that _start sits at the very
# start of the flat kernel image, which is where the bootloader jumps.
.section .text.boot
.global _start
.extern kmain

_start:
    cli
    # Set up kernel stack
    movq $stack_top, %rsp
    movq %rsp, %rbp
    andq $-16, %rsp # 16-byte alignment for System V ABI

    # ABI Transition:
    # UEFI (MS ABI) passes pointer in RCX
    # Kernel (System V) expects pointer in RDI
    movq %rcx, %rdi

    # Jump to Kernel
    call kmain

halt:
    hlt
    jmp halt

.section .bss
.align 16
stack_bottom:
.skip 16384 # 16KiB
stack_top:

.section .text
.global irq0_isr
.extern timer_handler

irq0_isr:
    pushq %rax
    pushq %rcx
    pushq %rdx
    pushq %rsi
    pushq %rdi
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11

    call timer_handler

    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rdi
    popq %rsi
    popq %rdx
    popq %rcx
    popq %rax

    iretq

.global irq1_isr
.extern keyboard_handler

irq1_isr:
    pushq %rax
    pushq %rcx
    pushq %rdx
    pushq %rsi
    pushq %rdi
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11

    call keyboard_handler

    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rdi
    popq %rsi
    popq %rdx
    popq %rcx
    popq %rax

    iretq

.global irq12_isr
.extern mouse_handler_v2

irq12_isr:
    pushq %rax
    pushq %rcx
    pushq %rdx
    pushq %rsi
    pushq %rdi
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11

    call mouse_handler_v2

    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rdi
    popq %rsi
    popq %rdx
    popq %rcx
    popq %rax

    iretq

.section .note.GNU-stack,"",@progbits
