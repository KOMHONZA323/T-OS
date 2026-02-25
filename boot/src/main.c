#include <efi.h>
#include <efilib.h>

EFI_STATUS
EFIAPI
efi_main (EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
    EFI_STATUS Status;

    // Memory Map variables
    UINTN MapSize = 16384, MapKey, DescriptorSize;
    UINT32 DescriptorVersion;
    // Use a large static buffer to avoid allocation complexity during map retrieval
    // Align to 16 bytes using UINT64 array
    static UINT64 MemoryMapBuffer[2048];
    EFI_MEMORY_DESCRIPTOR *MemoryMap = (EFI_MEMORY_DESCRIPTOR *)MemoryMapBuffer;

    InitializeLib(ImageHandle, SystemTable);

    Print(L"T-OS Bootloader Initializing...\n");

    // 1. Get Memory Map
    Status = uefi_call_wrapper(SystemTable->BootServices->GetMemoryMap, 5, &MapSize, MemoryMap, &MapKey, &DescriptorSize, &DescriptorVersion);
    if (EFI_ERROR(Status)) {
        Print(L"Error getting memory map: %r\n", Status);
        return Status;
    }

    Print(L"Memory Map Retrieved. MapKey: %d\n", MapKey);
    Print(L"Exiting Boot Services...\n");

    // 3. Exit Boot Services
    // We loop here because ExitBootServices might fail if the memory map changes (e.g. by timers)
    // between GetMemoryMap and ExitBootServices.
    UINTN RetryCount = 0;
    while (RetryCount < 5) {
        // Reset MapSize to buffer capacity
        MapSize = sizeof(MemoryMapBuffer);

        Status = uefi_call_wrapper(SystemTable->BootServices->GetMemoryMap, 5, &MapSize, MemoryMap, &MapKey, &DescriptorSize, &DescriptorVersion);
        if (EFI_ERROR(Status)) {
            Print(L"Error getting fresh memory map key: %r\n", Status);
            return Status;
        }

        Status = uefi_call_wrapper(SystemTable->BootServices->ExitBootServices, 2, ImageHandle, MapKey);
        if (Status == EFI_SUCCESS) {
            break;
        }
        RetryCount++;
    }

    if (EFI_ERROR(Status)) {
        Print(L"Error exiting boot services: %r\n", Status);
        return Status;
    }

    // Success! We are now in control of the machine.
    // Infinite loop to halt
    while (1) {
        __asm__ __volatile__("hlt");
    }

    return EFI_SUCCESS;
}
