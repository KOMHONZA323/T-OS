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
