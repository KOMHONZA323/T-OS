disk_load:
    ; DH = sector count, DL = drive, ES:BX = buffer (0:0x1000)
    push dx
    mov ah, 0x02    ; CHS read - required for floppy (LBA often not supported)
    mov al, dh      ; Sectors to read
    mov ch, 0x00    ; Cylinder 0
    mov cl, 0x02    ; Sector 2 (1-based = first sector after boot)
    mov dh, 0x00    ; Head 0
    ; DL = drive already set by caller
    int 0x13
    jc disk_error
    pop dx
    cmp al, dh
    jne disk_error
    ret

disk_error:
    mov bx, DISK_ERROR_MSG
    call print_string
    jmp $

DISK_ERROR_MSG db "Disk read error!", 0
