[org 0x7E00]
[bits 16]

; Stack setup
mov bp, 0x9000
mov sp, bp

; Save boot drive
mov [BOOT_DRIVE], dl

; Get Drive Geometry (SPT and Heads)
mov ah, 0x08
mov dl, [BOOT_DRIVE]
int 0x13
jc geometry_error
; DH = Max Head (0-based)
; CL = Max Sector (bits 0-5)
; CH = Max Cylinder (low 8 bits)
and cl, 0x3F ; Mask sector bits
mov [SECTORS_PER_TRACK], cl
inc dh
mov [HEADS], dh

jmp font_load

geometry_error:
    ; Fallback to Floppy defaults if it fails
    mov byte [SECTORS_PER_TRACK], 18
    mov byte [HEADS], 2

font_load:
; ------------------------------------------------------------------
; 1. Load Font (BIOS 8x16)
; ------------------------------------------------------------------
; ES:BP points to font table. INT 10h/AX=1130h/BH=06h
push es
mov ax, 0x1130
mov bh, 0x06
int 0x10
; ES:BP is font. Copy 4096 bytes (256 chars * 16 bytes) to 0x9000 (safe high memory)
push ds
push es
pop ds
mov si, bp
mov ax, 0
mov es, ax
mov di, 0xA000
mov cx, 4096
rep movsb
pop ds
pop es

; ------------------------------------------------------------------
; 2. Read Configuration
; ------------------------------------------------------------------
call read_config
cmp byte [CONFIG_MAGIC], 0xAB
je config_found

; If not found, show menu
call show_menu
jmp save_and_continue

config_found:
mov al, [CONFIG_RES]
mov [SELECTED_RES], al

save_and_continue:
; ------------------------------------------------------------------
; 3. Detect Memory Map
; ------------------------------------------------------------------
call detect_memory_map

; ------------------------------------------------------------------
; 4. Set VBE Mode
; ------------------------------------------------------------------
call set_vbe_mode

; ------------------------------------------------------------------
; 5. Load Kernel
; ------------------------------------------------------------------
; Kernel starts at Sector 10 (LBA 9).
; Load to 0x10000 (ES=0x1000, BX=0x0000)
; We load 128 sectors (64KB) just to be safe.
mov ax, 0x1000
mov es, ax
mov bx, 0x0000

mov cx, 128       ; Sectors to load
mov ax, 17        ; Start LBA. Boot=1, Stage2=16 -> Kernel starts at LBA 17.

load_loop:
    push ax
    push cx

    call lba_to_chs ; Returns CHS in CH, DH, CL

    mov ah, 0x02    ; Read
    mov al, 1       ; 1 Sector
    mov dl, [BOOT_DRIVE]
    int 0x13
    jc disk_error

    pop cx
    pop ax

    add bx, 512     ; Advance buffer
    jnc no_seg_inc
    mov dx, es
    add dx, 0x1000
    mov es, dx
no_seg_inc:

    inc ax          ; Next LBA
    loop load_loop

; ------------------------------------------------------------------
; 5. Switch to Protected Mode
; ------------------------------------------------------------------
call switch_to_pm

jmp $ ; Should not reach here

; ------------------------------------------------------------------
; Functions
; ------------------------------------------------------------------

%include "src/boot/stage2/utils.asm"
%include "src/boot/stage2/disk.asm"
%include "src/boot/stage2/menu.asm"
%include "src/boot/stage2/vbe.asm"
%include "src/boot/stage2/memory_map.asm"

vbe_error:
    mov si, MSG_VBE_ERR
    call print_string_real
    jmp $

disk_error:
    mov si, MSG_DISK_ERR
    call print_string_real
    jmp $

%include "src/boot/stage2/data.asm"
%include "src/boot/stage2/gdt.asm"
%include "src/boot/stage2/switch_to_pm.asm"
%include "src/boot/stage2/print_string_pm.asm"

[bits 32]
BEGIN_PM:
    mov ebx, MSG_PROT_MODE
    call print_string_pm
    call 0x10000
    jmp $
