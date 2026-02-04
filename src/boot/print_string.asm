print_string:
    pusha
    mov ah, 0x0e
.loop:
    mov al, [bx]
    cmp al, 0
    je .done
    int 0x10
    add bx, 1
    jmp .loop
.done:
    ; Print newline
    mov al, 0x0a
    int 0x10
    mov al, 0x0d
    int 0x10

    popa
    ret
