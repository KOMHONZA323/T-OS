set_vbe_mode:
    ; Check Selection
    mov al, [SELECTED_RES]
    cmp al, 9
    je set_vga_mode

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
    cmp al, 0xFF
    je find_best_mode ; Auto Detect
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
; VGA MODE (640x480 16-color, 4 planes)
; ---------------------------------------------------
set_vga_mode:
    mov ax, 0x0012
    int 0x10

    ; Populate ScreenInfo at 0x5000 manually for VGA Mode
; TEXT MODE (VGA Mode 3)
; ---------------------------------------------------
set_vga_mode:
    mov ax, 0x0012
    int 0x10

    ; Populate ScreenInfo at 0x5000 manually for VGA Mode
    mov ax, 0
    mov es, ax
    mov di, 0x5000

    mov word [es:di], 640      ; Width
    mov word [es:di+2], 480    ; Height
    mov byte [es:di+4], 4      ; BPP = 4 (Indicator for 4-bit Planar)
    mov word [es:di+5], 80     ; Pitch (640 pixels / 8 bits per byte = 80 bytes)
    mov dword [es:di+7], 0xA0000 ; Framebuffer Address (VGA window)

    
    ret

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
