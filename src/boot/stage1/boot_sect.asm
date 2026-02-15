[org 0x7c00]
    jmp short start
    nop

    ; BPB (BIOS Parameter Block) Placeholder
    ; make_fat.py will overwrite this area (offset 3 to ~62)
    times 60 db 0

start:
    KERNEL_OFFSET equ 0x1000
    STAGE2_OFFSET equ 0x7E00

    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00

    mov [BOOT_DRIVE], dl ; Remember the drive number

    ; Load Stage 2
    mov bx, STAGE2_OFFSET
    mov dh, 16           ; Load 16 sectors (8KB)
    mov dl, [BOOT_DRIVE]
    call disk_load

    ; Jump to Stage 2
    jmp STAGE2_OFFSET

%include "src/boot/stage1/disk_load.asm"
%include "src/boot/stage1/print_string.asm"

; Variables
BOOT_DRIVE db 0

; Padding
times 510-($-$$) db 0
dw 0xaa55
