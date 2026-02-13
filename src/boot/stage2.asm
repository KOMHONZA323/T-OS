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
; 3. Set VBE Mode
; ------------------------------------------------------------------
call set_vbe_mode

; ------------------------------------------------------------------
; 4. Load Kernel
; ------------------------------------------------------------------
; Kernel starts at Sector 10 (LBA 9).
; Load to 0x10000 (ES=0x1000, BX=0x0000)
; We load 60 sectors (30KB) just to be safe.
mov ax, 0x1000
mov es, ax
mov bx, 0x0000

mov cx, 60        ; Sectors to load
mov ax, 9         ; Start LBA (Sector 10 = LBA 9)

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
    ; Check 64KB segment wrap?
    ; 0x1000:0xFFFF -> next is 0x2000:0x0000? No, linear.
    ; Real mode limit is 64KB offset.
    ; If BX overflows 0 (wraps), we need to increment ES.
    jnc no_seg_inc
    mov dx, es
    add dx, 0x1000 ; Add 4KB (64KB / 16) to segment?
    ; No, 64KB = 0x1000 paragraphs.
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

lba_to_chs:
    ; Input: AX = LBA
    ; Output: CH = Cyl, DH = Head, CL = Sector

    push bx
    ; Do not push DX, we return value in DH

    xor dx, dx
    mov bl, [SECTORS_PER_TRACK]
    xor bh, bh ; Ensure BH is 0
    div bx      ; AX = LBA / SPT, DX = LBA % SPT

    inc dx      ; Sector = (LBA % SPT) + 1
    mov cl, dl  ; CL = Sector

    xor dx, dx
    mov bl, [HEADS]
    xor bh, bh
    div bx      ; AX = Temp / Heads, DX = Temp % Heads

    mov ch, al  ; Cylinder (Low 8 bits)
    mov dh, dl  ; Head

    pop bx
    ret

read_config:
    ; Read last sector (LBA 2879)
    mov ax, 2879
    call lba_to_chs

    mov ax, 0
    mov es, ax
    mov bx, CONFIG_BUFFER

    mov ah, 0x02
    mov al, 1
    ; CH, CL, DH set by lba_to_chs
    mov dl, [BOOT_DRIVE]
    int 0x13
    ret

write_config:
    ; Write CONFIG_BUFFER to LBA 2879
    mov ax, 2879
    call lba_to_chs

    mov ax, 0
    mov es, ax
    mov bx, CONFIG_BUFFER

    mov ah, 0x03 ; Write
    mov al, 1
    ; CH, CL, DH set by lba_to_chs
    mov dl, [BOOT_DRIVE]
    int 0x13
    ret

show_menu:
    mov si, MSG_MENU
    call print_string_real

    ; Wait for key
wait_key:
    mov ah, 0x00
    int 0x16

    cmp al, '0'
    je sel_auto
    cmp al, '1'
    je sel_720p
    cmp al, '2'
    je sel_1080p
    cmp al, '3'
    je sel_1440p
    jmp wait_key

sel_auto:
    mov byte [SELECTED_RES], 0xFF
    jmp menu_done
sel_720p:
    mov byte [SELECTED_RES], 0
    jmp menu_done
sel_1080p:
    mov byte [SELECTED_RES], 1
    jmp menu_done
sel_1440p:
    mov byte [SELECTED_RES], 2

menu_done:
    ; Update Config Buffer
    mov byte [CONFIG_MAGIC], 0xAB
    mov al, [SELECTED_RES]
    mov [CONFIG_RES], al
    call write_config
    ret

set_vbe_mode:
    ; 1. Get VBE Info
    mov ax, 0
    mov es, ax
    mov di, VBE_INFO_BLOCK
    mov ax, 0x4F00
    int 0x10
    cmp ax, 0x004F
    jne vbe_error

    ; 2. Iterate modes to find best match
    ; VideoPtr is at VBE_INFO_BLOCK + 14 (DWORD)
    mov ax, [VBE_INFO_BLOCK + 14]
    mov fs, [VBE_INFO_BLOCK + 16] ; Segment
    mov si, ax                    ; Offset

    ; Target Width/Height
    mov al, [SELECTED_RES]
    cmp al, 0xFF
    je auto_detect
    cmp al, 0
    je target_720p
    cmp al, 1
    je target_1080p
    cmp al, 2
    je target_1440p

target_720p:
    mov cx, 1280
    mov dx, 720
    jmp find_mode
target_1080p:
    mov cx, 1920
    mov dx, 1080
    jmp find_mode
target_1440p:
    mov cx, 2560
    mov dx, 1440
    jmp find_mode

auto_detect:
    ; Loop through all modes and find highest resolution
    mov word [BEST_MODE], 0xFFFF
    mov dword [BEST_PIXELS], 0

next_auto_mode:
    mov bx, [fs:si]
    add si, 2
    cmp bx, 0xFFFF
    je set_auto_mode ; End of list, set the best found

    ; Get Mode Info
    push si
    push fs

    mov cx, bx ; Mode number
    or cx, 0x4000 ; LFB
    mov ax, 0x4F01
    mov di, MODE_INFO_BLOCK
    int 0x10

    pop fs
    pop si

    cmp ax, 0x004F
    jne next_auto_mode

    ; Check BPP (Offset 25) - Must be 32
    mov al, [MODE_INFO_BLOCK + 25]
    cmp al, 32
    jne next_auto_mode

    ; Check Attributes (Offset 0) - Must support LFB (bit 7) and Graphics (bit 4)
    mov ax, [MODE_INFO_BLOCK]
    test ax, 0x0080 ; LFB
    jz next_auto_mode
    test ax, 0x0010 ; Graphics
    jz next_auto_mode

    ; Calculate Resolution (Width * Height)
    mov ax, [MODE_INFO_BLOCK + 18] ; Width
    mov dx, [MODE_INFO_BLOCK + 20] ; Height

    ; Simple 32-bit Multiply (AX * DX -> DX:AX)
    mov cx, dx
    mul cx ; DX:AX = Width * Height

    ; Compare with BEST_PIXELS (stored as low dword, ignoring overflow > 4G which is impossible)
    ; Actually mul produces DX:AX. Width*Height fits in 32 bits easily (4K is ~8M pixels)
    push bx
    mov bx, word [BEST_PIXELS]
    mov cx, word [BEST_PIXELS+2]

    ; Compare High Word (DX vs CX)
    cmp dx, cx
    ja new_best
    jb not_best
    ; Compare Low Word (AX vs BX)
    cmp ax, bx
    ja new_best
    jmp not_best

new_best:
    mov word [BEST_PIXELS], ax
    mov word [BEST_PIXELS+2], dx
    mov bx, [fs:si-2] ; Retrieve mode number
    mov [BEST_MODE], bx

not_best:
    pop bx
    jmp next_auto_mode

set_auto_mode:
    mov bx, [BEST_MODE]
    cmp bx, 0xFFFF
    je vbe_error ; No suitable mode found

    ; Found best mode, set it
    or bx, 0x4000     ; Enable LFB
    mov ax, 0x4F02
    int 0x10
    cmp ax, 0x004F
    jne vbe_error

    ; Re-read mode info for the chosen mode to populate kernel info
    mov cx, [BEST_MODE]
    or cx, 0x4000
    mov ax, 0x4F01
    mov di, MODE_INFO_BLOCK
    int 0x10

    jmp save_info

find_mode:
    ; CX = Width, DX = Height
    ; Loop through FS:SI
next_mode:
    mov bx, [fs:si]
    add si, 2
    cmp bx, 0xFFFF
    je vbe_error ; End of list, mode not found

    ; Get Mode Info
    push cx
    push dx
    push si
    push fs

    mov cx, bx ; Mode number
    or cx, 0x4000 ; LFB
    mov ax, 0x4F01
    mov di, MODE_INFO_BLOCK
    int 0x10

    pop fs
    pop si
    pop dx
    pop cx

    cmp ax, 0x004F
    jne next_mode

    ; Check Width (Offset 18)
    mov ax, [MODE_INFO_BLOCK + 18]
    cmp ax, cx
    jne next_mode

    ; Check Height (Offset 20)
    mov ax, [MODE_INFO_BLOCK + 20]
    cmp ax, dx
    jne next_mode

    ; Check BPP (Offset 25)
    mov al, [MODE_INFO_BLOCK + 25]
    cmp al, 32
    jne next_mode

    ; Found it! Set mode.
    mov bx, [fs:si-2] ; Retrieve mode number
    or bx, 0x4000     ; Enable LFB
    mov ax, 0x4F02
    int 0x10
    cmp ax, 0x004F
    jne vbe_error

save_info:
    ; Save Info for Kernel
    ; Copy relevant data to 0x5000
    mov ax, 0
    mov es, ax
    mov di, 0x5000

    mov ax, [MODE_INFO_BLOCK + 18] ; Width
    mov [es:di], ax
    mov ax, [MODE_INFO_BLOCK + 20] ; Height
    mov [es:di+2], ax
    mov al, [MODE_INFO_BLOCK + 25] ; BPP
    mov [es:di+4], al

    ; VBE 3.0+ Pitch Check
    mov ax, [VBE_INFO_BLOCK + 4]
    cmp ax, 0x0300
    jl use_legacy_pitch

    mov ax, [MODE_INFO_BLOCK + 50] ; LinBytesPerScanLine
    test ax, ax
    jz use_legacy_pitch
    jmp save_pitch

use_legacy_pitch:
    mov ax, [MODE_INFO_BLOCK + 16] ; Pitch (Bytes per scanline)

save_pitch:
    mov [es:di+5], ax

    mov eax, [MODE_INFO_BLOCK + 40] ; Framebuffer (Phys Addr)
    mov [es:di+7], eax

    ret

vbe_error:
    mov si, MSG_VBE_ERR
    call print_string_real
    jmp $

disk_error:
    mov si, MSG_DISK_ERR
    call print_string_real
    jmp $

print_string_real:
    mov ah, 0x0E
.loop:
    lodsb
    test al, al
    jz .done
    int 0x10
    jmp .loop
.done:
    ret

; Variables
BOOT_DRIVE db 0
SELECTED_RES db 0
SECTORS_PER_TRACK db 18
HEADS db 2
MSG_MENU db "Select Resolution:", 13, 10, "0. Auto", 13, 10, "1. 720p", 13, 10, "2. 1080p", 13, 10, "3. 1440p", 13, 10, 0
MSG_VBE_ERR db "VBE Error or Resolution Not Supported!", 0
MSG_DISK_ERR db "Disk Read Error!", 0

BEST_MODE dw 0
BEST_PIXELS dd 0

; Buffers
CONFIG_BUFFER:
    CONFIG_MAGIC db 0
    CONFIG_RES db 0
    times 510 db 0 ; Padding

VBE_INFO_BLOCK equ 0x2000
MODE_INFO_BLOCK equ 0x3000

%include "src/boot/gdt.asm"
%include "src/boot/switch_to_pm.asm"
%include "src/boot/print_string_pm.asm"

[bits 32]
BEGIN_PM:
    mov ebx, MSG_PROT_MODE
    call print_string_pm

    ; Jump to Kernel (0x10000)
    call 0x10000
    jmp $

MSG_PROT_MODE db "Landed in 32-bit PM", 0
