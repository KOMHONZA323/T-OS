[org 0x7c00]
KERNEL_OFFSET equ 0x1000 ; This will now be the Stage 2 offset
; Wait, I said 0x7E00 for Stage 2. Let's stick to 0x7E00 to keep 0x1000 free for kernel if needed,
; or better yet, load Stage 2 at 0x9000 to be far away.
; Actually, typical is 0x7E00 (right after boot sector).

STAGE2_OFFSET equ 0x7E00

    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00

    mov [BOOT_DRIVE], dl ; Remember the drive number

    ; Load Stage 2
    mov bx, STAGE2_OFFSET
    mov dh, 16           ; Load 16 sectors (8KB) for Stage 2. Safer size.
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
