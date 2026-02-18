#include "uefi.h"

EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    (void)ImageHandle;

    // Reset Console
    SystemTable->ConOut->Reset(SystemTable->ConOut, 0);

    // Set Colors: White Text on Blue Background (XP Style)
    // 0x1F: Background Blue (0x10) | Foreground White (0x0F)
    SystemTable->ConOut->SetAttribute(SystemTable->ConOut, EFI_BACKGROUND_BLUE | EFI_WHITE);
    SystemTable->ConOut->ClearScreen(SystemTable->ConOut);

    // Print Message
    SystemTable->ConOut->OutputString(SystemTable->ConOut, (CHAR16*)L"Initializing Tri-Brid OS...\r\n\r\n");
    SystemTable->ConOut->OutputString(SystemTable->ConOut, (CHAR16*)L"Press any key to continue...\r\n");

    // Wait for Key Press
    EFI_INPUT_KEY Key;
    while (SystemTable->ConIn->ReadKeyStroke(SystemTable->ConIn, &Key) == EFI_NOT_READY) {
        // Busy loop. In a real OS we would use BootServices->WaitForEvent to save power.
    }

    return EFI_SUCCESS;
}
