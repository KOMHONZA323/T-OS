#include "uefi.h"

EFI_STATUS __attribute__((ms_abi)) efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    (void)ImageHandle;

    // Reset console
    SystemTable->ConOut->Reset(SystemTable->ConOut, 0);

    // Set colors: Bright White text on Blue background (XP style)
    SystemTable->ConOut->SetAttribute(SystemTable->ConOut, EFI_BACKGROUND_BLUE | EFI_WHITE);

    // Clear screen
    SystemTable->ConOut->ClearScreen(SystemTable->ConOut);

    // Print message
    SystemTable->ConOut->OutputString(SystemTable->ConOut, (CHAR16*)L"Initializing Tri-Brid OS...\r\n");
    SystemTable->ConOut->OutputString(SystemTable->ConOut, (CHAR16*)L"Press any key to continue...\r\n");

    // Wait for key press
    EFI_INPUT_KEY Key;
    UINTN Index;

    // Wait for the keystroke event
    SystemTable->BootServices->WaitForEvent(1, &SystemTable->ConIn->WaitForKey, &Index);

    // Read the key
    SystemTable->ConIn->ReadKeyStroke(SystemTable->ConIn, &Key);

    // Print exit message
    SystemTable->ConOut->OutputString(SystemTable->ConOut, (CHAR16*)L"Exiting...\r\n");

    return EFI_SUCCESS;
}
