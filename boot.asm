[org 0x7c00]      ; The BIOS loads the bootloader at this memory address

mov ah, 0x0e      ; BIOS teletype output function
mov al, 'A'       ; Move 'H' into the AL register
int 0x10          ; Trigger the video interrupt
mov al, 'H'
int 0x10
mov al, 'O'
int 0x10
mov al, 'J'
int 0x10
mov al, 'K'
int 0x10
mov al, 'Y'
int 0x10

jmp $             ; Infinite loop to prevent the CPU from executing random memory

times 510-($-$$) db 0  ; Pad the rest of the 512 bytes with zeros
dw 0xaa55              ; The "Magic Number" that tells BIOS this is bootable
