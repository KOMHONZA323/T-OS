#include <efi.h>
#include <efilib.h>

// Standard BootInfo to pass to kernel
typedef struct {
  uint64_t FramebufferBase;
  uint64_t FramebufferSize;
  uint32_t HorizontalResolution;
  uint32_t VerticalResolution;
  uint32_t PixelsPerScanLine;
} KernelBootInfo;

EFI_STATUS
EFIAPI
efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
  InitializeLib(ImageHandle, SystemTable);
  SystemTable->ConOut->ClearScreen(SystemTable->ConOut);
  Print(L"T-OS Bootloader v3.0\n");

  // 1. Setup Graphics (GOP)
  EFI_GUID gopGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
  EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
  EFI_STATUS status =
      uefi_call_wrapper(SystemTable->BootServices->LocateProtocol, 3, &gopGuid,
                        NULL, (void **)&gop);
  if (EFI_ERROR(status)) {
    Print(L"GOP not found!\n");
    return status;
  }

  KernelBootInfo kbi;
  kbi.FramebufferBase = gop->Mode->FrameBufferBase;
  kbi.FramebufferSize = gop->Mode->FrameBufferSize;
  kbi.HorizontalResolution = gop->Mode->Info->HorizontalResolution;
  kbi.VerticalResolution = gop->Mode->Info->VerticalResolution;
  kbi.PixelsPerScanLine = gop->Mode->Info->PixelsPerScanLine;

  // 2. Load Kernel (kernel.bin) from Root
  EFI_LOADED_IMAGE *LoadedImage;
  EFI_FILE_IO_INTERFACE *FileSystem;
  EFI_FILE_HANDLE Root, KernelFile;

  uefi_call_wrapper(SystemTable->BootServices->HandleProtocol, 3, ImageHandle,
                    &LoadedImageProtocol, (void **)&LoadedImage);
  uefi_call_wrapper(SystemTable->BootServices->HandleProtocol, 3,
                    LoadedImage->DeviceHandle, &FileSystemProtocol,
                    (void **)&FileSystem);

  if (!FileSystem)
    LibLocateProtocol(&FileSystemProtocol, (void **)&FileSystem);

  uefi_call_wrapper(FileSystem->OpenVolume, 2, FileSystem, &Root);
  status = uefi_call_wrapper(Root->Open, 5, Root, &KernelFile, L"kernel.bin",
                             EFI_FILE_MODE_READ, 0);

  if (EFI_ERROR(status)) {
    Print(L"kernel.bin not found on USB root!\n");
    while (1)
      ;
  }

  // 3. Allocate Memory for Kernel
  EFI_PHYSICAL_ADDRESS kernelAddr = 0x1000000; // 16MB
  UINTN kernelSize = 0x200000;                 // 2MB max
  uefi_call_wrapper(SystemTable->BootServices->AllocatePages, 4,
                    AllocateAddress, EfiLoaderCode, 512, &kernelAddr);
  uefi_call_wrapper(KernelFile->Read, 3, KernelFile, &kernelSize,
                    (void *)kernelAddr);
  uefi_call_wrapper(KernelFile->Close, 1, KernelFile);

  // 4. Exit Boot Services
  UINTN MapSize = 0, MapKey, DescriptorSize;
  UINT32 DescriptorVersion;
  void *MemoryMap;
  uefi_call_wrapper(SystemTable->BootServices->GetMemoryMap, 5, &MapSize, NULL,
                    &MapKey, &DescriptorSize, &DescriptorVersion);
  MapSize += 4096;
  uefi_call_wrapper(SystemTable->BootServices->AllocatePool, 3, EfiLoaderData,
                    MapSize, &MemoryMap);
  uefi_call_wrapper(SystemTable->BootServices->GetMemoryMap, 5, &MapSize,
                    MemoryMap, &MapKey, &DescriptorSize, &DescriptorVersion);

  status = uefi_call_wrapper(SystemTable->BootServices->ExitBootServices, 2,
                             ImageHandle, MapKey);
  if (EFI_ERROR(status)) {
    uefi_call_wrapper(SystemTable->BootServices->GetMemoryMap, 5, &MapSize,
                      MemoryMap, &MapKey, &DescriptorSize, &DescriptorVersion);
    uefi_call_wrapper(SystemTable->BootServices->ExitBootServices, 2,
                      ImageHandle, MapKey);
  }

  // 5. Jump to Kernel
  void (*kernel_entry)(KernelBootInfo *) =
      (void (*)(KernelBootInfo *))kernelAddr;
  kernel_entry(&kbi);

  return EFI_SUCCESS;
}
