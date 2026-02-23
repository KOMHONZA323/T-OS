#include "uefi.h"

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    // Reset Console
    // Using 0 (False) for ExtendedVerification
    SystemTable->ConOut->Reset(SystemTable->ConOut, 0);

    // Set Colors: White Text on Blue Background
    // Mimicking Windows XP Blue Screen / Setup style
    // EFI_WHITE | EFI_BACKGROUND_BLUE
    SystemTable->ConOut->SetAttribute(SystemTable->ConOut, EFI_WHITE | EFI_BACKGROUND_BLUE);

    // Clear Screen
    SystemTable->ConOut->ClearScreen(SystemTable->ConOut);

    // Print Message
    // Note: UEFI uses wide characters (CHAR16)
    CHAR16 *msg = (CHAR16*)L"Initializing Tri-Brid OS...\r\n\r\nPress any key to exit...";
    SystemTable->ConOut->OutputString(SystemTable->ConOut, msg);

    // Wait for Key Press
    // We wait for the WaitForKey event in the ConIn protocol
    UINTN index;
    SystemTable->BootServices->WaitForEvent(1, &SystemTable->ConIn->WaitForKey, &index);

    // Clear Input Buffer (Optional, to consume the key)
    EFI_INPUT_KEY key;
    SystemTable->ConIn->ReadKeyStroke(SystemTable->ConIn, &key);

    return EFI_SUCCESS;
}
