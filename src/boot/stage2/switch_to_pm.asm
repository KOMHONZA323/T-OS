[bits 16]
switch_to_pm:
    cli                    ; 1. Disable interrupts
    lgdt [gdt_descriptor]  ; 2. Load the GDT descriptor

    mov eax, cr0
    or eax, 0x1            ; 3. Set 32-bit mode bit in CR0
    mov cr0, eax

    jmp CODE_SEG:init_pm   ; 4. Far jump by using a different segment

[bits 32]
init_pm:
    mov ax, DATA_SEG        ; 5. Update the segment registers
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov ebp, 0x90000        ; 6. Update the stack right at the top of the free space
    mov esp, ebp

    call BEGIN_PM           ; 7. Call some well-known label
