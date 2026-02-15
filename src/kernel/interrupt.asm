; Interrupt Service Routine Wrappers
; This file handles the transition from interrupt to C handler

; Defined in C (idt.c / isr.c)
extern isr_handler
extern irq_handler

; Make these global so C can link them
global isr0
global isr1
global isr2
global isr3
global isr4
global isr5
global isr6
global isr7
global isr8
global isr9
global isr10
global isr11
global isr12
global isr13
global isr14
global isr15
global isr16
global isr17
global isr18
global isr19
global isr20
global isr21
global isr22
global isr23
global isr24
global isr25
global isr26
global isr27
global isr28
global isr29
global isr30
global isr31

; IRQs
global irq0
global irq1
global irq2
global irq3
global irq4
global irq5
global irq6
global irq7
global irq8
global irq9
global irq10
global irq11
global irq12
global irq13
global irq14
global irq15

; SYSCALL
global isr128

; Common ISR Stub
; Pushes registers, calls C handler, restores registers
isr_common_stub:
    pusha           ; Pushes edi,esi,ebp,esp,ebx,edx,ecx,eax
    mov ax, ds      ; Lower 16-bits of eax = ds.
    push eax        ; save the data segment descriptor
    mov ax, 0x10    ; load the kernel data segment descriptor
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp       ; Push pointer to registers_t
    call isr_handler
    add esp, 4     ; Clean up pushed pointer

    pop eax        ; reload the original data segment descriptor
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    popa           ; Pops edi,esi,ebp,esp,ebx,edx,ecx,eax
    add esp, 8     ; Cleans up the pushed error code and pushed ISR number
    iret           ; pops 5 things at once: CS, EIP, EFLAGS, SS, ESP

; Common IRQ Stub
irq_common_stub:
    pusha
    mov ax, ds
    push eax
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp       ; Push pointer to registers_t
    call irq_handler
    mov esp, eax   ; Switch to the returned stack pointer

    pop eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    popa
    add esp, 8
    iret

; -----------------------------------------------------------------------------
; ISR Definitions
; -----------------------------------------------------------------------------

; 0: Divide By Zero Exception
isr0:
    cli
    push byte 0    ; Dummy error code
    push byte 0    ; Interrupt number
    jmp isr_common_stub

; 1: Debug Exception
isr1:
    cli
    push byte 0
    push byte 1
    jmp isr_common_stub

; 2: Non Maskable Interrupt Exception
isr2:
    cli
    push byte 0
    push byte 2
    jmp isr_common_stub

; 3: Breakpoint Exception
isr3:
    cli
    push byte 0
    push byte 3
    jmp isr_common_stub

; 4: Into Detected Overflow Exception
isr4:
    cli
    push byte 0
    push byte 4
    jmp isr_common_stub

; 5: Out of Bounds Exception
isr5:
    cli
    push byte 0
    push byte 5
    jmp isr_common_stub

; 6: Invalid Opcode Exception
isr6:
    cli
    push byte 0
    push byte 6
    jmp isr_common_stub

; 7: No Coprocessor Exception
isr7:
    cli
    push byte 0
    push byte 7
    jmp isr_common_stub

; 8: Double Fault Exception (With Error Code)
isr8:
    cli
    push byte 8
    jmp isr_common_stub

; 9: Coprocessor Segment Overrun Exception
isr9:
    cli
    push byte 0
    push byte 9
    jmp isr_common_stub

; 10: Bad TSS Exception (With Error Code)
isr10:
    cli
    push byte 10
    jmp isr_common_stub

; 11: Segment Not Present Exception (With Error Code)
isr11:
    cli
    push byte 11
    jmp isr_common_stub

; 12: Stack Fault Exception (With Error Code)
isr12:
    cli
    push byte 12
    jmp isr_common_stub

; 13: General Protection Fault Exception (With Error Code)
isr13:
    cli
    push byte 13
    jmp isr_common_stub

; 14: Page Fault Exception (With Error Code)
isr14:
    cli
    push byte 14
    jmp isr_common_stub

; 15: Reserved Exception
isr15:
    cli
    push byte 0
    push byte 15
    jmp isr_common_stub

; 16: Floating Point Exception
isr16:
    cli
    push byte 0
    push byte 16
    jmp isr_common_stub

; 17: Alignment Check Exception
isr17:
    cli
    push byte 0
    push byte 17
    jmp isr_common_stub

; 18: Machine Check Exception
isr18:
    cli
    push byte 0
    push byte 18
    jmp isr_common_stub

; 19-31: Reserved
isr19:
    cli
    push byte 0
    push byte 19
    jmp isr_common_stub

isr20:
    cli
    push byte 0
    push byte 20
    jmp isr_common_stub

isr21:
    cli
    push byte 0
    push byte 21
    jmp isr_common_stub

isr22:
    cli
    push byte 0
    push byte 22
    jmp isr_common_stub

isr23:
    cli
    push byte 0
    push byte 23
    jmp isr_common_stub

isr24:
    cli
    push byte 0
    push byte 24
    jmp isr_common_stub

isr25:
    cli
    push byte 0
    push byte 25
    jmp isr_common_stub

isr26:
    cli
    push byte 0
    push byte 26
    jmp isr_common_stub

isr27:
    cli
    push byte 0
    push byte 27
    jmp isr_common_stub

isr28:
    cli
    push byte 0
    push byte 28
    jmp isr_common_stub

isr29:
    cli
    push byte 0
    push byte 29
    jmp isr_common_stub

isr30:
    cli
    push byte 0
    push byte 30
    jmp isr_common_stub

isr31:
    cli
    push byte 0
    push byte 31
    jmp isr_common_stub

; -----------------------------------------------------------------------------
; IRQ Definitions
; -----------------------------------------------------------------------------

; IRQ 0: Timer
irq0:
    cli
    push byte 0
    push byte 32
    jmp irq_common_stub

; IRQ 1: Keyboard
irq1:
    cli
    push byte 1
    push byte 33
    jmp irq_common_stub

; IRQ 2-15
irq2:
    cli
    push byte 2
    push byte 34
    jmp irq_common_stub
irq3:
    cli
    push byte 3
    push byte 35
    jmp irq_common_stub
irq4:
    cli
    push byte 4
    push byte 36
    jmp irq_common_stub
irq5:
    cli
    push byte 5
    push byte 37
    jmp irq_common_stub
irq6:
    cli
    push byte 6
    push byte 38
    jmp irq_common_stub
irq7:
    cli
    push byte 7
    push byte 39
    jmp irq_common_stub
irq8:
    cli
    push byte 8
    push byte 40
    jmp irq_common_stub
irq9:
    cli
    push byte 9
    push byte 41
    jmp irq_common_stub
irq10:
    cli
    push byte 10
    push byte 42
    jmp irq_common_stub
irq11:
    cli
    push byte 11
    push byte 43
    jmp irq_common_stub
irq12:
    cli
    push byte 12
    push byte 44
    jmp irq_common_stub
irq13:
    cli
    push byte 13
    push byte 45
    jmp irq_common_stub
irq14:
    cli
    push byte 14
    push byte 46
    jmp irq_common_stub
irq15:
    cli
    push byte 15
    push byte 47
    jmp irq_common_stub

; SYSCALL (INT 0x80)
extern syscall_handler_c

isr128:
    cli
    pusha           ; Pushes edi,esi,ebp,esp,ebx,edx,ecx,eax
    mov ax, ds      ; Lower 16-bits of eax = ds.
    push eax        ; save the data segment descriptor
    mov ax, 0x10    ; load the kernel data segment descriptor
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp        ; Push pointer to registers_t (FIX: Added this)
    call syscall_handler_c
    add esp, 4      ; Clean up stack (FIX: Added this)

    pop eax         ; reload the original data segment descriptor
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    popa            ; Pops edi,esi,ebp,esp,ebx,edx,ecx,eax
    sti
    iret
