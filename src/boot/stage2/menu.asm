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
