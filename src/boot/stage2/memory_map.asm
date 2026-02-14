detect_memory_map:
    pusha
    push es
    xor ax, ax
    mov es, ax
    mov di, 0x4004          ; Store map entries at 0x4004
    xor ebX, ebX            ; EBX must be 0 to start
    xor bp, bp              ; Keep an entry count in BP
    mov edx, 0x534D4150     ; Place "SMAP" into edx
    mov eax, 0xE820
    mov [es:di + 20], dword 1 ; force a valid ACPI 3.X entry
    mov ecx, 24             ; ask for 24 bytes
    int 0x15
    jc .failed              ; carry set on first call means "unsupported function"
    mov edx, 0x534D4150     ; Some BIOSes trash this register?
    cmp eax, edx            ; on success, eax must have been reset to "SMAP"
    jne .failed
    test ebx, ebx           ; ebx = 0 implies list is only 1 entry long (worthless)
    je .failed
    jmp .jumpin

.e820lp:
    mov eax, 0xE820         ; eax, ecx get trashed on every int 0x15 call
    mov [es:di + 20], dword 1 ; force a valid ACPI 3.X entry
    mov ecx, 24             ; ask for 24 bytes again
    int 0x15
    jc .e820f               ; carry set means "end of list already reached"
    mov edx, 0x534D4150     ; repair potentially trashed register

.jumpin:
    jcxz .skipent           ; skip any 0 length entries
    cmp cl, 20              ; got a 24 byte ACPI 3.X response?
    jbe .notext
    test byte [es:di + 20], 1 ; if so: is the "ignore this data" bit clear?
    je .skipent

.notext:
    mov ecx, [es:di + 8]    ; get lower uint32_t of memory region length
    or ecx, [es:di + 12]    ; "or" it with upper uint32_t to test for zero
    jz .skipent             ; if length is 0, skip
    inc bp                  ; got a good entry: ++count, move to next storage spot
    add di, 24

.skipent:
    test ebx, ebx           ; if ebx resets to 0, list is complete
    jne .e820lp

.e820f:
    mov [es:0x4000], bp     ; store the entry count
    clc                     ; there is "no error", so clear the carry bit
    pop es
    popa
    ret

.failed:
    stc                     ; "function unsupported", so set carry bit
    pop es
    popa
    ret
