#include "uefi.h"

EFI_SYSTEM_TABLE *ST;

void print(CHAR16 *str) {
    ST->ConOut->OutputString(ST->ConOut, str);
}

void print_char(CHAR16 c) {
    CHAR16 str[2];
    str[0] = c;
    str[1] = 0;
    print(str);
}

int strcmp16(CHAR16 *s1, CHAR16 *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *s1 - *s2;
}

void execute_command(CHAR16 *cmd) {
    if (cmd[0] == 0) return;
    
    if (strcmp16(cmd, (CHAR16*)L"help") == 0) {
        print((CHAR16*)L"Available commands:\r\n");
        print((CHAR16*)L"  help  - Show this help message\r\n");
        print((CHAR16*)L"  hello - Print hello message\r\n");
    } else if (strcmp16(cmd, (CHAR16*)L"hello") == 0) {
        print((CHAR16*)L"Hello from T-OS!\r\n");
    } else {
        print((CHAR16*)L"Unknown command. Type 'help' for a list of commands.\r\n");
    }
}

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    ST = SystemTable;
    
    // Clear screen
    ST->ConOut->ClearScreen(ST->ConOut);
    
    print((CHAR16*)L"Welcome to T-OS!\r\n");
    print((CHAR16*)L"Type 'help' to see available commands.\r\n");
    
    CHAR16 input_buffer[256];
    UINTN input_length = 0;
    
    while (1) {
        print((CHAR16*)L"T-OS> ");
        
        input_length = 0;
        input_buffer[0] = 0;
        
        while (1) {
            UINTN index;
            EFI_INPUT_KEY key;
            
            // Wait for key event
            ST->BootServices->WaitForEvent(1, &ST->ConIn->WaitForKey, &index);
            
            // Read key
            EFI_STATUS status = ST->ConIn->ReadKeyStroke(ST->ConIn, &key);
            if (status == EFI_SUCCESS) {
                if (key.UnicodeChar == '\r') { // Enter
                    print((CHAR16*)L"\r\n");
                    input_buffer[input_length] = 0;
                    break;
                } else if (key.UnicodeChar == '\b') { // Backspace
                    if (input_length > 0) {
                        input_length--;
                        print((CHAR16*)L"\b \b");
                    }
                } else if (key.UnicodeChar >= ' ' && key.UnicodeChar <= '~') { // Printable characters
                    if (input_length < 255) {
                        input_buffer[input_length++] = key.UnicodeChar;
                        print_char(key.UnicodeChar);
                    }
                }
            }
        }
        
        execute_command(input_buffer);
    }
    
    return EFI_SUCCESS;
}
