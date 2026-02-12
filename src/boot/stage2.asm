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

show_menu:
    mov si, MSG_MENU
    call print_string_real

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
    cmp al, '4'
    je sel_1920p
    cmp al, '5'
    je sel_4k
    jmp wait_key

sel_auto:
    mov byte [SELECTED_RES], 0
    jmp menu_done
sel_720p:
    mov byte [SELECTED_RES], 1
    jmp menu_done
sel_1080p:
    mov byte [SELECTED_RES], 2
    jmp menu_done
sel_1440p:
    mov byte [SELECTED_RES], 3
    jmp menu_done
sel_1920p:
    mov byte [SELECTED_RES], 4
    jmp menu_done
sel_4k:
    mov byte [SELECTED_RES], 5

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

    ; Setup iterator
    mov ax, [VBE_INFO_BLOCK + 14]
    mov fs, [VBE_INFO_BLOCK + 16] ; Segment
    mov si, ax                    ; Offset

    ; Check Selection
    mov al, [SELECTED_RES]
    cmp al, 0
    je find_best_mode ; Auto Detect
    cmp al, 1
    je target_720p
    cmp al, 2
    je target_1080p
    cmp al, 3
    je target_1440p
    cmp al, 4
    je target_1920p
    cmp al, 5
    je target_4k

target_720p:
    mov cx, 1280
    mov dx, 720
    jmp find_specific_mode
target_1080p:
    mov cx, 1920
    mov dx, 1080
    jmp find_specific_mode
target_1440p:
    mov cx, 2560
    mov dx, 1440
    jmp find_specific_mode
target_1920p:
    mov cx, 2560
    mov dx, 1920
    jmp find_specific_mode
target_4k:
    mov cx, 3840
    mov dx, 2160
    jmp find_specific_mode

; ---------------------------------------------------
; AUTO DETECT MODE (Highest Res 32bpp)
; ---------------------------------------------------
find_best_mode:
    ; Variables: BX (Best Mode #), CX (Best Width), DX (Best Height)
    xor bx, bx
    xor cx, cx
    xor dx, dx

.scan_loop:
    push bx ; Save best mode number
    mov bx, [fs:si] ; Get next mode from list
    add si, 2
    cmp bx, 0xFFFF
    je .scan_done

    ; Get Mode Info
    push cx
    push dx
    push si
    push fs

    mov cx, bx
    or cx, 0x4000 ; LFB
    mov ax, 0x4F01
    mov di, MODE_INFO_BLOCK
    int 0x10

    pop fs
    pop si
    pop dx
    pop cx
    pop ax ; This is the 'best mode' we pushed as BX. Pop into AX temp.

    cmp byte [MODE_INFO_BLOCK + 25], 32 ; Check BPP
    jne .restore_and_next

    ; Check Resolution
    mov bx, [MODE_INFO_BLOCK + 18] ; Width
    cmp bx, cx
    jb .restore_and_next ; Current < Best
    ja .new_best         ; Current > Best

    ; If Widths equal, check height
    mov bx, [MODE_INFO_BLOCK + 20] ; Height
    cmp bx, dx
    jbe .restore_and_next

.new_best:
    ; Found better mode
    mov cx, [MODE_INFO_BLOCK + 18] ; Update Best Width
    mov dx, [MODE_INFO_BLOCK + 20] ; Update Best Height
    mov bx, [fs:si-2]              ; Update Best Mode ID
    jmp .scan_loop

.restore_and_next:
    mov bx, ax ; Restore best mode back to BX
    jmp .scan_loop

.scan_done:
    pop bx ; Just to balance the push at start of .scan_loop before jump
           ; Actually logic is tricky: .scan_loop pushes BX. 
           ; .scan_done jumps out. We need to pop the BX that was pushed at start of .scan_loop.
           ; Wait, the jump to .scan_done happens immediately after fetch.
           ; "push bx" happens AT START of loop.
           ; If we jump, stack has one BX pushed.
           ; So yes, pop bx. This BX is the best mode found so far.

    cmp bx, 0
    je vbe_error ; No suitable mode found

    ; Set the best mode
    push bx ; Save mode number
    or bx, 0x4000
    mov ax, 0x4F02
    int 0x10
    pop bx ; Restore mode number for info query
    
    ; Refresh Info Block for the chosen mode (since we scanned past it)
    mov cx, bx
    or cx, 0x4000
    mov ax, 0x4F01
    mov di, MODE_INFO_BLOCK
    int 0x10
    
    jmp save_mode_info

; ---------------------------------------------------
; SPECIFIC MODE SEARCH
; ---------------------------------------------------
find_specific_mode:
    ; CX = Target Width, DX = Target Height
next_specific:
    mov bx, [fs:si]
    add si, 2
    cmp bx, 0xFFFF
    je vbe_error

    push cx
    push dx
    push si
    push fs

    mov cx, bx
    or cx, 0x4000
    mov ax, 0x4F01
    mov di, MODE_INFO_BLOCK
    int 0x10

    pop fs
    pop si
    pop dx
    pop cx

    ; Check Width
    mov ax, [MODE_INFO_BLOCK + 18]
    cmp ax, cx
    jne next_specific

    ; Check Height
    mov ax, [MODE_INFO_BLOCK + 20]
    cmp ax, dx
    jne next_specific

    ; Check BPP
    mov al, [MODE_INFO_BLOCK + 25]
    cmp al, 32
    jne next_specific

    ; Found
    mov bx, [fs:si-2]
    or bx, 0x4000
    mov ax, 0x4F02
    int 0x10
    
    ; Fall through to save info

save_mode_info:
    mov ax, 0
    mov es, ax
    mov di, 0x5000

    mov ax, [MODE_INFO_BLOCK + 18] ; Width
    mov [es:di], ax
    mov ax, [MODE_INFO_BLOCK + 20] ; Height
    mov [es:di+2], ax
    mov al, [MODE_INFO_BLOCK + 25] ; BPP
    mov [es:di+4], al

    ; --- PITCH CORRECTION ---
    mov ax, [MODE_INFO_BLOCK + 50] ; LinBytesPerScanLine
    cmp ax, 0
    jne use_lin_pitch
    mov ax, [MODE_INFO_BLOCK + 16] ; BytesPerScanLine
use_lin_pitch:
    mov [es:di+5], ax
    ; ------------------------

    mov eax, [MODE_INFO_BLOCK + 40] ; Framebuffer
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
; Updated Menu
MSG_MENU db "Select Resolution:", 13, 10, "0. Auto (Best)", 13, 10, "1. 720p", 13, 10, "2. 1080p", 13, 10, "3. 1440p", 13, 10, "4. 1920p", 13, 10, "5. 4K", 13, 10, 0
MSG_VBE_ERR db "VBE Error!", 0
MSG_DISK_ERR db "Disk Error!", 0

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
    call 0x10000
    jmp $

MSG_PROT_MODE db "Landed in 32-bit PM", 0
