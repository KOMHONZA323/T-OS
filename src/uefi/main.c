#include "uefi.h"

EFI_STATUS __attribute__((ms_abi)) efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    // Reset the console
    SystemTable->ConOut->Reset(SystemTable->ConOut, 0);

    // Set text attribute: White text on Blue background (XP Style)
    SystemTable->ConOut->SetAttribute(SystemTable->ConOut, EFI_BACKGROUND_BLUE | EFI_WHITE);

    // Clear the screen to apply the background color
    SystemTable->ConOut->ClearScreen(SystemTable->ConOut);

    // Print the welcome message
    SystemTable->ConOut->OutputString(SystemTable->ConOut, (CHAR16*)L"Initializing Tri-Brid OS...\r\n\r\nPress any key to exit...");

    // Wait for a key press
    UINTN Index;
    EFI_STATUS Status = SystemTable->BootServices->WaitForEvent(1, &SystemTable->ConIn->WaitForKey, &Index);

    return Status;
}
