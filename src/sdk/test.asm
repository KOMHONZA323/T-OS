start:
    push 0
    call main
    mov eax, 1
    int 0x80

main:
    mov eax, 2
    mov ebx, msg
    int 0x80
    ret

msg: db "Hello World", 0
