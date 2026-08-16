/*
 * x86-64 kernel-thread context switch for the T-OS CPU scheduler.
 *
 * Only the System V callee-saved registers plus RFLAGS have to be preserved
 * here: everything else is already caller-saved across the C call to
 * sched_arch_switch(), and the interrupted user of the CPU (the IRQ0 stub)
 * saves the caller-saved set itself.
 *
 * Stack image left behind by a switched-out thread, low address first:
 *
 *     rflags, r15, r14, r13, r12, rbx, rbp, return address
 *
 * sched_arch_init_stack() in sched_arch.c builds exactly that image for a
 * brand new thread, with the return address pointing at the trampoline below.
 */

.section .text

/*
 * void sched_arch_switch(uint64_t *save_rsp, uint64_t load_rsp)
 *   rdi = where to store the outgoing stack pointer
 *   rsi = stack pointer of the thread to resume
 */
.global sched_arch_switch
.type sched_arch_switch, @function
sched_arch_switch:
    pushq %rbp
    pushq %rbx
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    pushfq

    movq %rsp, (%rdi)       # save the outgoing context
    movq %rsi, %rsp         # adopt the incoming thread's stack

    popfq
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rbx
    popq %rbp
    ret                     # resume the incoming thread
.size sched_arch_switch, . - sched_arch_switch

/*
 * First instruction executed by a freshly created thread. r13 holds its entry
 * point and r12 its argument, both planted by sched_arch_init_stack().
 */
.global sched_thread_trampoline
.type sched_thread_trampoline, @function
sched_thread_trampoline:
    andq $-16, %rsp         # SysV wants rsp 16-byte aligned before a call
    movq %r12, %rdi         # arg
    movq %r13, %rax         # entry point
    callq *%rax
    callq sched_exit        # entry returned: retire the thread
1:  hlt                     # sched_exit never comes back
    jmp 1b
.size sched_thread_trampoline, . - sched_thread_trampoline

.section .note.GNU-stack,"",@progbits
