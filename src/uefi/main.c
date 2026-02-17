#include "uefi.h"

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    (void)ImageHandle;

    // Reset console
    SystemTable->ConOut->Reset(SystemTable->ConOut, 0);

    // Set Color: Bright Blue text on Black background
    // Mimicking a "futuristic" or "XP Blue" vibe.
    SystemTable->ConOut->SetAttribute(SystemTable->ConOut, EFI_LIGHTBLUE | EFI_BACKGROUND_BLACK);

    // Clear screen
    SystemTable->ConOut->ClearScreen(SystemTable->ConOut);

    // Print Message
    SystemTable->ConOut->OutputString(SystemTable->ConOut, L"Initializing Tri-Brid OS...\r\n");
    SystemTable->ConOut->OutputString(SystemTable->ConOut, L"\r\n"); // Spacing
    SystemTable->ConOut->OutputString(SystemTable->ConOut, L"Press any key to exit...\r\n");

    // Wait for key
    EFI_INPUT_KEY Key;
    EFI_STATUS Status;

    // Simple polling loop
    while (1) {
        Status = SystemTable->ConIn->ReadKeyStroke(SystemTable->ConIn, &Key);
        if (Status == EFI_SUCCESS) {
            break;
        }
        // Ideally we would use BootServices->WaitForEvent or Stall here to save CPU,
        // but for a minimal Hello World without full BootServices definitions,
        // a tight loop is acceptable.
    }

    // Reset attribute before exit
    SystemTable->ConOut->SetAttribute(SystemTable->ConOut, EFI_LIGHTGRAY | EFI_BACKGROUND_BLACK);
    SystemTable->ConOut->ClearScreen(SystemTable->ConOut);

    return EFI_SUCCESS;
}
