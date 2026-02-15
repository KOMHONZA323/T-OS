[BITS 32]

section .text
    global os_create_window
    global os_spawn_process
    global print
    global get_char
    global exit
    global sleep

; Syscall Interface
; EAX: Syscall Number
; EBX: Arg 1
; ECX: Arg 2
; EDX: Arg 3
; ESI: Arg 4
; EDI: Arg 5

; -----------------------------------------------------------------------------
; void exit(int status);
; Syscall: 1
; -----------------------------------------------------------------------------
exit:
    mov eax, 1          ; Syscall 1
    mov ebx, [esp+4]    ; Arg 1: status
    int 0x80
    ret

; -----------------------------------------------------------------------------
; void print(char* message);
; Syscall: 2
; -----------------------------------------------------------------------------
print:
    push ebx            ; Preserve Callee-Saved Regs
    mov eax, 2          ; Syscall 2
    mov ebx, [esp+8]    ; Arg 1: message (offset shifted by push)
    int 0x80
    pop ebx
    ret

; -----------------------------------------------------------------------------
; char get_char();
; Syscall: 3
; -----------------------------------------------------------------------------
get_char:
    push ebx            ; Preserve Callee-Saved Regs
    mov eax, 3          ; Syscall 3
    int 0x80
    pop ebx
    ret

; -----------------------------------------------------------------------------
; void os_create_window(int width, int height, char* title);
; Syscall: 4
; -----------------------------------------------------------------------------
os_create_window:
    push ebx            ; Preserve Callee-Saved Regs
    push ecx
    push edx

    mov eax, 4          ; Syscall 4
    mov ebx, [esp+16]   ; Arg 1: width
    mov ecx, [esp+20]   ; Arg 2: height
    mov edx, [esp+24]   ; Arg 3: title

    int 0x80

    pop edx
    pop ecx
    pop ebx
    ret

; -----------------------------------------------------------------------------
; int os_spawn_process(char* filename);
; Syscall: 5
; -----------------------------------------------------------------------------
os_spawn_process:
    push ebx            ; Preserve Callee-Saved Regs
    mov eax, 5          ; Syscall 5
    mov ebx, [esp+8]    ; Arg 1: filename
    int 0x80
    pop ebx
    ret

; -----------------------------------------------------------------------------
; void sleep(int ms);
; Syscall: 6
; -----------------------------------------------------------------------------
sleep:
    push ebx            ; Preserve Callee-Saved Regs
    mov eax, 6          ; Syscall 6
    mov ebx, [esp+8]    ; Arg 1: ms
    int 0x80
    pop ebx
    ret
