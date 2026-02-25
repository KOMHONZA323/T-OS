#include "uefi.h"

EFI_STATUS __attribute__((ms_abi)) efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    (void)ImageHandle; // Unused

    // Set text attribute: Blue Background, White Text (Windows XP style)
    SystemTable->ConOut->SetAttribute(SystemTable->ConOut, EFI_BACKGROUND_BLUE | EFI_WHITE);

    // Clear the screen to apply the background color
    SystemTable->ConOut->ClearScreen(SystemTable->ConOut);

    // Print the initialization message
    // Note: L"..." creates a wide string literal. We cast to CHAR16* for safety,
    // assuming -fshort-wchar is used during compilation.
    SystemTable->ConOut->OutputString(SystemTable->ConOut, (CHAR16*)L"Initializing Tri-Brid OS...\r\n");

    // Prompt user
    SystemTable->ConOut->OutputString(SystemTable->ConOut, (CHAR16*)L"Press any key to continue...\r\n");

    // Wait for a key press
    UINTN Index;
    SystemTable->BootServices->WaitForEvent(1, &SystemTable->ConIn->WaitForKey, &Index);

    return EFI_SUCCESS;
}
