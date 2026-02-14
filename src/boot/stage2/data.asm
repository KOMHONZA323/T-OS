; Variables
BOOT_DRIVE db 0
SELECTED_RES db 0
SECTORS_PER_TRACK db 18
HEADS db 2
; Updated Menu
MSG_MENU db "Select Resolution:", 13, 10, "0. Auto (Best)", 13, 10, "1. 720p", 13, 10, "2. 1080p", 13, 10, "3. 1440p", 13, 10, "4. 1920p", 13, 10, "5. 4K", 13, 10, "9. VGA (640x480)", 13, 10, 0
MSG_VBE_ERR db "VBE Error!", 0
MSG_DISK_ERR db "Disk Error!", 0

BEST_MODE dw 0
BEST_PIXELS dd 0

; Buffers
CONFIG_BUFFER:
    CONFIG_MAGIC db 0
    CONFIG_RES db 0
    times 510 db 0 ; Padding

VBE_INFO_BLOCK equ 0x2000
MODE_INFO_BLOCK equ 0x3000
MSG_PROT_MODE db "Landed in 32-bit PM", 0
