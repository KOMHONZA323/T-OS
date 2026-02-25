#include <uefi.h>

// Globals
EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop = NULL;
UINT32 *BackBuffer = NULL;
UINT32 ScreenWidth = 0;
UINT32 ScreenHeight = 0;
UINT32 PixelsPerScanLine = 0;

// Simple memcpy
void *memcpy(void *dest, const void *src, UINTN n) {
    char *d = (char *)dest;
    const char *s = (const char *)src;
    while (n--) {
        *d++ = *s++;
    }
    return dest;
}

// Helpers
void print(EFI_SYSTEM_TABLE *SystemTable, CHAR16 *str) {
    SystemTable->ConOut->OutputString(SystemTable->ConOut, str);
}

void itoa_hex(UINT64 value, CHAR16 *str) {
    const CHAR16 *digits = (CHAR16 *)L"0123456789ABCDEF";
    int i = 0;
    int j = 0;
    CHAR16 temp[20];

    if (value == 0) {
        temp[i++] = (CHAR16)'0';
    } else {
        while (value > 0) {
            temp[i++] = digits[value % 16];
            value /= 16;
        }
    }

    str[j++] = (CHAR16)'0';
    str[j++] = (CHAR16)'x';

    while (i > 0) {
        str[j++] = temp[--i];
    }
    str[j] = 0;
}

void print_hex(EFI_SYSTEM_TABLE *SystemTable, UINT64 value) {
    CHAR16 buffer[32];
    itoa_hex(value, buffer);
    print(SystemTable, buffer);
}

// Memory Map Function (Phase 1A Requirement)
EFI_STATUS get_memory_map(EFI_SYSTEM_TABLE *SystemTable) {
    UINTN MemoryMapSize = 0;
    EFI_MEMORY_DESCRIPTOR *MemoryMap = NULL;
    UINTN MapKey;
    UINTN DescriptorSize;
    UINT32 DescriptorVersion;
    EFI_STATUS Status;

    // Get size needed
    Status = SystemTable->BootServices->GetMemoryMap(&MemoryMapSize, NULL, &MapKey, &DescriptorSize, &DescriptorVersion);
    if (Status != EFI_BUFFER_TOO_SMALL) {
        // It might succeed if size happens to be 0 (unlikely) or fail otherwise
        if (Status != EFI_SUCCESS) return Status;
    }

    // Allocate pool (add extra space to avoid reallocation causing map change)
    MemoryMapSize += 4096;

    Status = SystemTable->BootServices->AllocatePool(EfiLoaderData, MemoryMapSize, (void**)&MemoryMap);
    if (EFI_ERROR(Status)) {
        print(SystemTable, (CHAR16 *)L"Error: Could not allocate memory map buffer\r\n");
        return Status;
    }

    // Get Map
    Status = SystemTable->BootServices->GetMemoryMap(&MemoryMapSize, MemoryMap, &MapKey, &DescriptorSize, &DescriptorVersion);
    if (EFI_ERROR(Status)) {
        print(SystemTable, (CHAR16 *)L"Error: Could not get memory map\r\n");
        SystemTable->BootServices->FreePool(MemoryMap);
        return Status;
    }

    print(SystemTable, (CHAR16 *)L"Memory Map Retrieved successfully. Map Size: ");
    print_hex(SystemTable, MemoryMapSize);
    print(SystemTable, (CHAR16 *)L"\r\n");

    SystemTable->BootServices->FreePool(MemoryMap);
    return EFI_SUCCESS;
}

// Graphics Functions
EFI_STATUS init_graphics(EFI_SYSTEM_TABLE *SystemTable) {
    EFI_GUID GopGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    EFI_STATUS Status;

    Status = SystemTable->BootServices->LocateProtocol(&GopGuid, NULL, (void**)&Gop);
    if (EFI_ERROR(Status)) {
        print(SystemTable, (CHAR16 *)L"Error: Could not locate GOP\r\n");
        return Status;
    }

    ScreenWidth = Gop->Mode->Info->HorizontalResolution;
    ScreenHeight = Gop->Mode->Info->VerticalResolution;
    PixelsPerScanLine = Gop->Mode->Info->PixelsPerScanLine;

    // Allocate BackBuffer
    UINTN buffer_size = ScreenHeight * PixelsPerScanLine * 4;

    Status = SystemTable->BootServices->AllocatePool(EfiLoaderData, buffer_size, (void**)&BackBuffer);
    if (EFI_ERROR(Status)) {
        print(SystemTable, (CHAR16 *)L"Error: Could not allocate back buffer\r\n");
        return Status;
    }

    // Clear BackBuffer
    for (UINTN i = 0; i < ScreenHeight * PixelsPerScanLine; i++) {
        BackBuffer[i] = 0xFF000000; // Black
    }

    return EFI_SUCCESS;
}

void draw_pixel(UINTN x, UINTN y, UINT32 color) {
    if (x >= ScreenWidth || y >= ScreenHeight) return;
    BackBuffer[y * PixelsPerScanLine + x] = color;
}

void draw_rect(UINTN x, UINTN y, UINTN w, UINTN h, UINT32 color) {
    for (UINTN j = 0; j < h; j++) {
        for (UINTN i = 0; i < w; i++) {
            draw_pixel(x + i, y + j, color);
        }
    }
}

void swap_buffers() {
    if (!Gop || !BackBuffer) return;

    // Copy BackBuffer to Video Memory
    UINT32 *video_mem = (UINT32 *)Gop->Mode->FrameBufferBase;
    UINTN total_pixels = ScreenHeight * PixelsPerScanLine;

    for (UINTN i = 0; i < total_pixels; i++) {
        video_mem[i] = BackBuffer[i];
    }
}

EFI_STATUS
EFIAPI
efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
    (void)ImageHandle;
    EFI_STATUS Status;

    SystemTable->ConOut->ClearScreen(SystemTable->ConOut);
    print(SystemTable, (CHAR16 *)L"Initializing T-OS...\r\n");

    // Phase 1A: Get Memory Map
    Status = get_memory_map(SystemTable);
    if (EFI_ERROR(Status)) {
        // Continue anyway? No, report it.
        print(SystemTable, (CHAR16 *)L"Warning: Memory Map failed.\r\n");
    }

    print(SystemTable, (CHAR16 *)L"Initializing Graphics...\r\n");

    Status = init_graphics(SystemTable);
    if (EFI_ERROR(Status)) {
        // Wait for key before exit so user can read error
        EFI_INPUT_KEY Key;
        SystemTable->ConIn->Reset(SystemTable->ConIn, 0);
        while ((Status = SystemTable->ConIn->ReadKeyStroke(SystemTable->ConIn, &Key)) == EFI_NOT_READY);
        return Status;
    }

    // Validation Test: Draw Gradient and T-OS Logo

    // Draw Background Gradient (Blue-ish)
    for (UINTN y = 0; y < ScreenHeight; y++) {
        for (UINTN x = 0; x < ScreenWidth; x++) {
            // Gradient based on X and Y
            UINT8 r = (x * 255) / ScreenWidth;
            UINT8 g = (y * 255) / ScreenHeight;
            UINT8 b = 128;
            UINT32 color = 0xFF000000 | (r << 16) | (g << 8) | b; // 0xAARRGGBB
            draw_pixel(x, y, color);
        }
    }

    // Draw T-OS Logo (simple rectangles)
    // T
    UINTN cx = ScreenWidth / 2;
    UINTN cy = ScreenHeight / 2;

    // T - Top bar
    draw_rect(cx - 150, cy - 100, 300, 40, 0xFFFFFFFF);
    // T - Vertical bar
    draw_rect(cx - 20, cy - 100, 40, 200, 0xFFFFFFFF);

    // Swap buffers to display
    swap_buffers();

    // Halt CPU
    // We disable interrupts and halt
    // Note: In UEFI, disabling interrupts is risky if we plan to return, but here we halt forever.
    // However, specs say "Halt the CPU and display...".
    // We should ideally loop halt.

    while (1) {
        __asm__ volatile ("hlt");
    }

    return EFI_SUCCESS;
}
