#include "uefi.h"

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    (void)ImageHandle; // Unused
    UINTN Index;
    EFI_INPUT_KEY Key;

    // Reset console
    SystemTable->ConOut->Reset(SystemTable->ConOut, 0);

    // Set colors: White text on Blue background (XP BSOD / Setup style)
    SystemTable->ConOut->SetAttribute(SystemTable->ConOut, EFI_BACKGROUND_BLUE | EFI_WHITE);
    SystemTable->ConOut->ClearScreen(SystemTable->ConOut);

    // Print message
    // Note: Compile with -fshort-wchar to ensure L"" strings are 16-bit characters.
    SystemTable->ConOut->OutputString(SystemTable->ConOut, (CHAR16*)L"Initializing Tri-Brid OS...\r\n");
    SystemTable->ConOut->OutputString(SystemTable->ConOut, (CHAR16*)L"Press any key to continue...\r\n");

    // Wait for key
    SystemTable->BootServices->WaitForEvent(1, &SystemTable->ConIn->WaitForKey, &Index);
    SystemTable->ConIn->ReadKeyStroke(SystemTable->ConIn, &Key);

    // Clear screen before exiting
    SystemTable->ConOut->ClearScreen(SystemTable->ConOut);

    return EFI_SUCCESS;
}
