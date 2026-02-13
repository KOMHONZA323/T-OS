lba_to_chs:
    push bx
    xor dx, dx
    mov bl, [SECTORS_PER_TRACK]
    xor bh, bh
    div bx      ; AX = LBA / SPT, DX = LBA % SPT

    inc dx      ; Sector
    mov cl, dl

    xor dx, dx
    mov bl, [HEADS]
    xor bh, bh
    div bx      ; AX = Temp / Heads, DX = Temp % Heads

    mov ch, al  ; Cylinder
    mov dh, dl  ; Head

    pop bx
    ret

read_config:
    mov ax, 2879
    call lba_to_chs

    mov ax, 0
    mov es, ax
    mov bx, CONFIG_BUFFER

    mov ah, 0x02
    mov al, 1
    mov dl, [BOOT_DRIVE]
    int 0x13
    ret

write_config:
    mov ax, 2879
    call lba_to_chs

    mov ax, 0
    mov es, ax
    mov bx, CONFIG_BUFFER

    mov ah, 0x03
    mov al, 1
    mov dl, [BOOT_DRIVE]
    int 0x13
    ret
