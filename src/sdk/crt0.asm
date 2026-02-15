[BITS 32]
global _start
extern main
extern exit

section .text
_start:
    ; Setup stack args for main(int argc, char** argv)
    ; Since kernel doesn't pass args yet, we push defaults.
    push 0          ; argv = NULL
    push 0          ; argc = 0

    call main

    ; If main returns, call exit
    call exit

    ; Should not reach here
    jmp $
