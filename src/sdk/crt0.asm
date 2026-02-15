[BITS 32]

section .text
    global _start
    extern main
    extern exit

_start:
    ; Standard C Start Routine

    ; 1. Set up stack frame (optional, but good practice)
    ; Assuming the kernel or loader sets ESP to a valid stack top.

    ; 2. Initialize arguments (argc, argv) - for now 0
    push 0          ; argv
    push 0          ; argc

    ; 3. Call main()
    call main       ; Returns exit code in EAX

    ; 4. Clean up stack (standard C convention: caller cleans up)
    add esp, 8      ; Pop 2 arguments

    ; 5. Call exit(status) with result from main
    push eax        ; Status code
    call exit       ; Should not return

    ; Failsafe loop if exit returns
    jmp $
