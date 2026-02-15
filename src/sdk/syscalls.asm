[BITS 32]
global exit
global print
global get_char
global os_create_window
global os_spawn_process
global sleep

; Syscall Numbers (Match syscalls.h)
SYS_EXIT equ 1
SYS_PRINT equ 2
SYS_GET_CHAR equ 3
SYS_CREATE_WINDOW equ 4
SYS_SPAWN_PROCESS equ 5
SYS_SLEEP equ 6

exit:
    mov eax, SYS_EXIT
    int 0x80
    ret

print:
    ; void print(char* message)
    ; message in [esp+4]
    mov eax, SYS_PRINT
    mov ebx, [esp+4]
    int 0x80
    ret

get_char:
    ; char get_char()
    mov eax, SYS_GET_CHAR
    int 0x80
    ; Result in al
    ret

os_create_window:
    ; int os_create_window(int width, int height, char* title)
    ; width [esp+4], height [esp+8], title [esp+12]
    push ebx
    push ecx
    push edx

    mov eax, SYS_CREATE_WINDOW
    mov ebx, [esp+16] ; width
    mov ecx, [esp+20] ; height
    mov edx, [esp+24] ; title
    int 0x80

    pop edx
    pop ecx
    pop ebx
    ret

os_spawn_process:
    ; int os_spawn_process(char* filename)
    ; filename [esp+4]
    push ebx

    mov eax, SYS_SPAWN_PROCESS
    mov ebx, [esp+8]
    int 0x80

    pop ebx
    ret

sleep:
    ; void sleep(int ms)
    push ebx

    mov eax, SYS_SLEEP
    mov ebx, [esp+8]
    int 0x80

    pop ebx
    ret
