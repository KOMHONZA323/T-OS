#include "uefi.h"

EFI_STATUS __attribute__((ms_abi)) efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *ConOut = SystemTable->ConOut;
    EFI_SIMPLE_TEXT_INPUT_PROTOCOL *ConIn = SystemTable->ConIn;

    // Set Colors: Windows XP Style (White on Blue)
    // Using EFI_BACKGROUND_BLUE (0x10) | EFI_WHITE (0x0F) = 0x1F
    ConOut->SetAttribute(ConOut, EFI_BACKGROUND_BLUE | EFI_WHITE);
    ConOut->ClearScreen(ConOut);

    // Print Initialization Message
    ConOut->OutputString(ConOut, L"Initializing Tri-Brid OS...\r\n\r\n");
    ConOut->OutputString(ConOut, L" [ OK ] Bootloader: UEFI Initialized\r\n");
    ConOut->OutputString(ConOut, L" [ OK ] Kernel: Custom x86_64 Higher-Half\r\n");
    ConOut->OutputString(ConOut, L" [ OK ] Graphics: UEFI GOP Active\r\n\r\n");

    ConOut->OutputString(ConOut, L"Press any key to continue sequence...\r\n");

    // Reset Input
    ConIn->Reset(ConIn, 0);

    // Wait for Key Press
    EFI_INPUT_KEY Key;
    while ((ConIn->ReadKeyStroke(ConIn, &Key)) != EFI_SUCCESS) {
        __asm__ __volatile__("pause");
    }

    // Clear Screen before exit
    ConOut->ClearScreen(ConOut);

    return EFI_SUCCESS;
}
