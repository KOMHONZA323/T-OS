#include "uefi.h"

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    (void)ImageHandle; // Unused

    // Reset Console
    SystemTable->ConOut->Reset(SystemTable->ConOut, 0);

    // Clear Screen
    SystemTable->ConOut->ClearScreen(SystemTable->ConOut);

    // Set Color: Bright Light Blue on Black (mimicking XP blue console)
    // EFI_LIGHTBLUE | EFI_BACKGROUND_BLACK
    SystemTable->ConOut->SetAttribute(SystemTable->ConOut, EFI_LIGHTBLUE | EFI_BACKGROUND_BLACK);

    // Print Message
    SystemTable->ConOut->OutputString(SystemTable->ConOut, (CHAR16*)L"Initializing Tri-Brid OS...\r\n");
    SystemTable->ConOut->OutputString(SystemTable->ConOut, (CHAR16*)L"Press any key to exit...\r\n");

    // Wait for key press
    EFI_INPUT_KEY Key;
    EFI_STATUS Status;

    // Reset Input
    SystemTable->ConIn->Reset(SystemTable->ConIn, 0);

    while (1) {
        Status = SystemTable->ConIn->ReadKeyStroke(SystemTable->ConIn, &Key);
        if (Status == EFI_SUCCESS) {
            break;
        }
        // Ideally we should use WaitForEvent here to save CPU, but busy loop is fine for Hello World without full BootServices definition
    }

    // Reset color before exit
    SystemTable->ConOut->SetAttribute(SystemTable->ConOut, EFI_LIGHTGRAY | EFI_BACKGROUND_BLACK);
    SystemTable->ConOut->ClearScreen(SystemTable->ConOut);

    return EFI_SUCCESS;
}
