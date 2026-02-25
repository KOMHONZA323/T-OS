#ifndef UEFI_H
#define UEFI_H

#include <stdint.h>

// Basic Types
typedef void* EFI_HANDLE;
typedef uint64_t EFI_STATUS;
typedef uint64_t UINTN;
typedef uint16_t CHAR16;
typedef uint16_t UINT16;
typedef uint32_t UINT32;
typedef uint64_t UINT64;
typedef uint8_t  UINT8;
typedef unsigned char BOOLEAN;

#define EFI_SUCCESS 0
#define NULL (void*)0

// Console Colors
#define EFI_BLACK        0x00
#define EFI_BLUE         0x01
#define EFI_GREEN        0x02
#define EFI_CYAN         0x03
#define EFI_RED          0x04
#define EFI_MAGENTA      0x05
#define EFI_BROWN        0x06
#define EFI_LIGHTGRAY    0x07
#define EFI_DARKGRAY     0x08
#define EFI_LIGHTBLUE    0x09
#define EFI_LIGHTGREEN   0x0A
#define EFI_LIGHTCYAN    0x0B
#define EFI_LIGHTRED     0x0C
#define EFI_LIGHTMAGENTA 0x0D
#define EFI_YELLOW       0x0E
#define EFI_WHITE        0x0F

#define EFI_BACKGROUND_BLACK     0x00
#define EFI_BACKGROUND_BLUE      0x10
#define EFI_BACKGROUND_GREEN     0x20
#define EFI_BACKGROUND_CYAN      0x30
#define EFI_BACKGROUND_RED       0x40
#define EFI_BACKGROUND_MAGENTA   0x50
#define EFI_BACKGROUND_BROWN     0x60
#define EFI_BACKGROUND_LIGHTGRAY 0x70

// EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL
typedef struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL {
    void* Reset;
    EFI_STATUS (__attribute__((ms_abi)) *OutputString)(struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This, CHAR16 *String);
    void* TestString;
    void* QueryMode;
    EFI_STATUS (__attribute__((ms_abi)) *SetAttribute)(struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This, UINTN Attribute);
    EFI_STATUS (__attribute__((ms_abi)) *ClearScreen)(struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This);
    void* SetCursorPosition;
    void* EnableCursor;
    void* Mode;
} EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;

// EFI_INPUT_KEY
typedef struct {
    UINT16 ScanCode;
    CHAR16 UnicodeChar;
} EFI_INPUT_KEY;

// EFI_SIMPLE_TEXT_INPUT_PROTOCOL
typedef struct EFI_SIMPLE_TEXT_INPUT_PROTOCOL {
    void* Reset;
    EFI_STATUS (__attribute__((ms_abi)) *ReadKeyStroke)(struct EFI_SIMPLE_TEXT_INPUT_PROTOCOL *This, EFI_INPUT_KEY *Key);
    void* WaitForKey;
} EFI_SIMPLE_TEXT_INPUT_PROTOCOL;

// EFI_BOOT_SERVICES
typedef void* EFI_EVENT;

typedef struct EFI_BOOT_SERVICES {
    char Header[24];
    void* RaiseTPL;
    void* RestoreTPL;
    void* AllocatePages;
    void* FreePages;
    void* GetMemoryMap;
    void* AllocatePool;
    void* FreePool;
    void* CreateEvent;
    void* SetTimer;
    EFI_STATUS (__attribute__((ms_abi)) *WaitForEvent)(UINTN NumberOfEvents, EFI_EVENT *Event, UINTN *Index);
    void* SignalEvent;
    void* CloseEvent;
    void* CheckEvent;
} EFI_BOOT_SERVICES;

// EFI_SYSTEM_TABLE
typedef struct EFI_SYSTEM_TABLE {
    char Header[24];
    CHAR16 *FirmwareVendor;
    UINT32 FirmwareRevision;
    EFI_HANDLE ConsoleInHandle;
    EFI_SIMPLE_TEXT_INPUT_PROTOCOL *ConIn;
    EFI_HANDLE ConsoleOutHandle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *ConOut;
    EFI_HANDLE StandardErrorHandle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *StdErr;
    void *RuntimeServices;
    EFI_BOOT_SERVICES *BootServices;
    UINTN NumberOfTableEntries;
    void *ConfigurationTable;
} EFI_SYSTEM_TABLE;

#endif
