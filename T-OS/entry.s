.section .text
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

.global irq0_isr
.extern timer_handler

irq0_isr:
    # Save scratch registers (caller-saved in System V AMD64 ABI)
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

    # Restore scratch registers
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
