[BITS 32]
global _start
extern main
extern exit

section .text
_start:
    ; Setup stack?
    ; The loader should have set up ESP.
    ; If not, we might need to, but for now assume loader does it.

    call main

    ; If main returns, call exit
    call exit

    ; Should not reach here
    jmp $
