#ifndef _UEFI_H_
#define _UEFI_H_

#include <stdint.h>
#include <stddef.h>

#define EFIAPI __attribute__((ms_abi))

typedef uint16_t CHAR16;
typedef char CHAR8;
typedef uint64_t UINTN;
typedef uint64_t UINT64;
typedef uint32_t UINT32;
typedef uint16_t UINT16;
typedef uint8_t UINT8;
typedef int64_t INTN;
typedef int32_t INT32;
typedef int16_t INT16;
typedef int8_t INT8;
typedef void *EFI_HANDLE;
typedef UINTN EFI_STATUS;
typedef uint64_t EFI_PHYSICAL_ADDRESS;

#define EFI_SUCCESS 0
#define TRUE 1
#define FALSE 0

typedef struct {
    UINT32 Data1;
    UINT16 Data2;
    UINT16 Data3;
    UINT8 Data4[8];
} EFI_GUID;

typedef struct {
    UINT16 ScanCode;
    CHAR16 UnicodeChar;
} EFI_INPUT_KEY;

typedef struct _EFI_SIMPLE_TEXT_INPUT_PROTOCOL EFI_SIMPLE_TEXT_INPUT_PROTOCOL;
typedef EFI_STATUS (EFIAPI *EFI_INPUT_RESET)(EFI_SIMPLE_TEXT_INPUT_PROTOCOL *This, UINT8 ExtendedVerification);
typedef EFI_STATUS (EFIAPI *EFI_INPUT_READ_KEY)(EFI_SIMPLE_TEXT_INPUT_PROTOCOL *This, EFI_INPUT_KEY *Key);

struct _EFI_SIMPLE_TEXT_INPUT_PROTOCOL {
    EFI_INPUT_RESET Reset;
    EFI_INPUT_READ_KEY ReadKeyStroke;
    void *WaitForKey;
};

typedef struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;
typedef EFI_STATUS (EFIAPI *EFI_TEXT_RESET)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This, UINT8 ExtendedVerification);
typedef EFI_STATUS (EFIAPI *EFI_TEXT_STRING)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This, CHAR16 *String);
typedef EFI_STATUS (EFIAPI *EFI_TEXT_TEST_STRING)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This, CHAR16 *String);
typedef EFI_STATUS (EFIAPI *EFI_TEXT_QUERY_MODE)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This, UINTN ModeNumber, UINTN *Columns, UINTN *Rows);
typedef EFI_STATUS (EFIAPI *EFI_TEXT_SET_MODE)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This, UINTN ModeNumber);
typedef EFI_STATUS (EFIAPI *EFI_TEXT_SET_ATTRIBUTE)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This, UINTN Attribute);
typedef EFI_STATUS (EFIAPI *EFI_TEXT_CLEAR_SCREEN)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This);
typedef EFI_STATUS (EFIAPI *EFI_TEXT_SET_CURSOR_POSITION)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This, UINTN Column, UINTN Row);
typedef EFI_STATUS (EFIAPI *EFI_TEXT_ENABLE_CURSOR)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This, UINT8 Visible);

typedef struct {
    INT32 MaxMode;
    INT32 Mode;
    INT32 Attribute;
    INT32 CursorColumn;
    INT32 CursorRow;
    UINT8 CursorVisible;
} SIMPLE_TEXT_OUTPUT_MODE;

struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL {
    EFI_TEXT_RESET Reset;
    EFI_TEXT_STRING OutputString;
    EFI_TEXT_TEST_STRING TestString;
    EFI_TEXT_QUERY_MODE QueryMode;
    EFI_TEXT_SET_MODE SetMode;
    EFI_TEXT_SET_ATTRIBUTE SetAttribute;
    EFI_TEXT_CLEAR_SCREEN ClearScreen;
    EFI_TEXT_SET_CURSOR_POSITION SetCursorPosition;
    EFI_TEXT_ENABLE_CURSOR EnableCursor;
    SIMPLE_TEXT_OUTPUT_MODE *Mode;
};

// Text Attribute Constants
#define EFI_BLACK         0x00
#define EFI_BLUE          0x01
#define EFI_GREEN         0x02
#define EFI_CYAN          0x03
#define EFI_RED           0x04
#define EFI_MAGENTA       0x05
#define EFI_BROWN         0x06
#define EFI_LIGHTGRAY     0x07
#define EFI_DARKGRAY      0x08
#define EFI_LIGHTBLUE     0x09
#define EFI_LIGHTGREEN    0x0A
#define EFI_LIGHTCYAN     0x0B
#define EFI_LIGHTRED      0x0C
#define EFI_LIGHTMAGENTA  0x0D
#define EFI_YELLOW        0x0E
#define EFI_WHITE         0x0F

#define EFI_TEXT_ATTRIBUTE(Foreground, Background) ((Foreground) | ((Background) << 4))

// Graphics Output Protocol
typedef struct _EFI_GRAPHICS_OUTPUT_PROTOCOL EFI_GRAPHICS_OUTPUT_PROTOCOL;

typedef enum {
  PixelRedGreenBlueReserved8BitPerColor,
  PixelBlueGreenRedReserved8BitPerColor,
  PixelBitMask,
  PixelBltOnly,
  PixelFormatMax
} EFI_GRAPHICS_PIXEL_FORMAT;

typedef struct {
  UINT32 RedMask;
  UINT32 GreenMask;
  UINT32 BlueMask;
  UINT32 ReservedMask;
} EFI_PIXEL_BITMASK;

typedef struct {
  UINT32                     Version;
  UINT32                     HorizontalResolution;
  UINT32                     VerticalResolution;
  EFI_GRAPHICS_PIXEL_FORMAT  PixelFormat;
  EFI_PIXEL_BITMASK          PixelInformation;
  UINT32                     PixelsPerScanLine;
} EFI_GRAPHICS_OUTPUT_MODE_INFORMATION;

typedef struct {
    UINT32 MaxMode;
    UINT32 Mode;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *Info;
    UINTN SizeOfInfo;
    UINTN FrameBufferBase;
    UINTN FrameBufferSize;
} EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE;

// Function pointer typedefs must come before the struct that uses them
typedef EFI_STATUS (EFIAPI *EFI_GRAPHICS_OUTPUT_PROTOCOL_QUERY_MODE) (
    EFI_GRAPHICS_OUTPUT_PROTOCOL  *This,
    UINT32                                ModeNumber,
    UINTN                                 *SizeOfInfo,
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  **Info
);

typedef EFI_STATUS (EFIAPI *EFI_GRAPHICS_OUTPUT_PROTOCOL_SET_MODE) (
    EFI_GRAPHICS_OUTPUT_PROTOCOL  *This,
    UINT32                                ModeNumber
);

typedef EFI_STATUS (EFIAPI *EFI_GRAPHICS_OUTPUT_PROTOCOL_BLT) (
    EFI_GRAPHICS_OUTPUT_PROTOCOL *This,
    void                                 *BltBuffer,
    UINTN                                BltOperation,
    UINTN                                SourceX,
    UINTN                                SourceY,
    UINTN                                DestinationX,
    UINTN                                DestinationY,
    UINTN                                Width,
    UINTN                                Height,
    UINTN                                Delta
);

struct _EFI_GRAPHICS_OUTPUT_PROTOCOL { // Use struct here for the actual definition
  EFI_GRAPHICS_OUTPUT_PROTOCOL_QUERY_MODE  QueryMode;
  EFI_GRAPHICS_OUTPUT_PROTOCOL_SET_MODE    SetMode;
  EFI_GRAPHICS_OUTPUT_PROTOCOL_BLT         Blt;
  EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE        *Mode;
};

#define EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID \
  { \
    0x9042a9de, 0x23dc, 0x4a38, {0x96, 0xfb, 0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a } \
  }

typedef struct {
    UINT64 Signature;
    UINT32 Revision;
    UINT32 HeaderSize;
    UINT32 CRC32;
    UINT32 Reserved;
} EFI_TABLE_HEADER;

typedef enum {
    EfiResetCold,
    EfiResetWarm,
    EfiResetShutdown,
    EfiResetPlatformSpecific
} EFI_RESET_TYPE;

typedef EFI_STATUS (EFIAPI *EFI_SET_VIRTUAL_ADDRESS_MAP)(UINTN MemoryMapSize, UINTN DescriptorSize, UINT32 DescriptorVersion, void *VirtualMap);
typedef EFI_STATUS (EFIAPI *EFI_CONVERT_POINTER)(UINTN DebugDisposition, void **Address);
typedef EFI_STATUS (EFIAPI *EFI_GET_TIME)(void *Time, void *Capabilities);
typedef EFI_STATUS (EFIAPI *EFI_SET_TIME)(void *Time);
typedef EFI_STATUS (EFIAPI *EFI_GET_WAKEUP_TIME)(UINT8 *Enabled, UINT8 *Pending, void *Time);
typedef EFI_STATUS (EFIAPI *EFI_SET_WAKEUP_TIME)(UINT8 Enable, void *Time);
typedef EFI_STATUS (EFIAPI *EFI_GET_VARIABLE)(CHAR16 *VariableName, EFI_GUID *VendorGuid, UINT32 *Attributes, UINTN *DataSize, void *Data);
typedef EFI_STATUS (EFIAPI *EFI_GET_NEXT_VARIABLE_NAME)(UINTN *VariableNameSize, CHAR16 *VariableName, EFI_GUID *VendorGuid);
typedef EFI_STATUS (EFIAPI *EFI_SET_VARIABLE)(CHAR16 *VariableName, EFI_GUID *VendorGuid, UINT32 Attributes, UINTN DataSize, void *Data);
typedef EFI_STATUS (EFIAPI *EFI_GET_NEXT_HIGH_MONO_COUNT)(UINT32 *HighCount);
typedef void (EFIAPI *EFI_RESET_SYSTEM)(EFI_RESET_TYPE ResetType, EFI_STATUS ResetStatus, UINTN DataSize, void *ResetData);

typedef struct {
    EFI_TABLE_HEADER Hdr;
    EFI_GET_TIME GetTime;
    EFI_SET_TIME SetTime;
    EFI_GET_WAKEUP_TIME GetWakeupTime;
    EFI_SET_WAKEUP_TIME SetWakeupTime;
    EFI_SET_VIRTUAL_ADDRESS_MAP SetVirtualAddressMap;
    EFI_CONVERT_POINTER ConvertPointer;
    EFI_GET_VARIABLE GetVariable;
    EFI_GET_NEXT_VARIABLE_NAME GetNextVariableName;
    EFI_SET_VARIABLE SetVariable;
    EFI_GET_NEXT_HIGH_MONO_COUNT GetNextHighMonotonicCount;
    EFI_RESET_SYSTEM ResetSystem;
} EFI_RUNTIME_SERVICES;

typedef EFI_STATUS (EFIAPI *EFI_WAIT_FOR_EVENT)(UINTN NumberOfEvents, void **Event, UINTN *Index);
typedef EFI_STATUS (EFIAPI *EFI_HANDLE_PROTOCOL)(EFI_HANDLE Handle, EFI_GUID *Protocol, void **Interface);
typedef EFI_STATUS (EFIAPI *EFI_LOCATE_PROTOCOL)(EFI_GUID *Protocol, void *Registration, void **Interface);

typedef EFI_STATUS (EFIAPI *EFI_ALLOCATE_POOL)(UINT32 PoolType, UINTN Size, void **Buffer);
typedef EFI_STATUS (EFIAPI *EFI_FREE_POOL)(void *Buffer);
typedef EFI_STATUS (EFIAPI *EFI_ALLOCATE_PAGES)(UINT32 Type, UINT32 MemoryType, UINTN Pages, EFI_PHYSICAL_ADDRESS *Memory);

typedef void (EFIAPI *EFI_COPY_MEM)(void *Destination, void *Source, UINTN Length);
typedef void (EFIAPI *EFI_SET_MEM)(void *Buffer, UINTN Size, UINT8 Value);

typedef struct {
    EFI_TABLE_HEADER Hdr;
    void *RaiseTPL;
    void *RestoreTPL;
    EFI_ALLOCATE_PAGES AllocatePages;
    void *FreePages;
    void *GetMemoryMap;
    EFI_ALLOCATE_POOL AllocatePool;
    EFI_FREE_POOL FreePool;
    void *CreateEvent;

    void *SetTimer;
    EFI_WAIT_FOR_EVENT WaitForEvent;
    void *SignalEvent;
    void *CloseEvent;
    void *CheckEvent;
    void *InstallProtocolInterface;
    void *ReinstallProtocolInterface;
    void *UninstallProtocolInterface;
    EFI_HANDLE_PROTOCOL HandleProtocol;
    void *Reserved;
    void *RegisterProtocolNotify;
    void *LocateHandle;
    void *LocateDevicePath;
    void *InstallConfigurationTable;
    void *LoadImage;
    void *StartImage;
    void *Exit;
    void *UnloadImage;
    void *ExitBootServices;
    void *GetNextMonotonicCount;
    EFI_STATUS (EFIAPI *Stall)(UINTN Microseconds);
    void *SetWatchdogTimer;
    void *ConnectController;
    void *DisconnectController;
    void *OpenProtocol;
    void *CloseProtocol;
    void *OpenProtocolInformation;
    void *ProtocolsPerHandle;
    void *LocateHandleBuffer;
    EFI_LOCATE_PROTOCOL LocateProtocol;
    void *InstallMultipleProtocolInterfaces;
    void *UninstallMultipleProtocolInterfaces;
    void *CalculateCrc32;
    EFI_COPY_MEM CopyMem;
    EFI_SET_MEM SetMem;
    void *CreateEventEx;
} EFI_BOOT_SERVICES;

typedef struct {
    EFI_TABLE_HEADER Hdr;
    CHAR16 *FirmwareVendor;
    UINT32 FirmwareRevision;
    EFI_HANDLE ConsoleInHandle;
    EFI_SIMPLE_TEXT_INPUT_PROTOCOL *ConIn;
    EFI_HANDLE ConsoleOutHandle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *ConOut;
    EFI_HANDLE StandardErrorHandle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *StdErr;
    EFI_RUNTIME_SERVICES *RuntimeServices;
    EFI_BOOT_SERVICES *BootServices;
} EFI_SYSTEM_TABLE;

// Forward declaration for EFI_FILE_PROTOCOL
typedef struct _EFI_FILE_PROTOCOL EFI_FILE_PROTOCOL;

typedef EFI_STATUS (EFIAPI *EFI_FILE_OPEN)(EFI_FILE_PROTOCOL *This, EFI_FILE_PROTOCOL **NewHandle, CHAR16 *FileName, UINT64 OpenMode, UINT64 Attributes);
typedef EFI_STATUS (EFIAPI *EFI_FILE_CLOSE)(EFI_FILE_PROTOCOL *This);
typedef EFI_STATUS (EFIAPI *EFI_FILE_DELETE)(EFI_FILE_PROTOCOL *This);
typedef EFI_STATUS (EFIAPI *EFI_FILE_READ)(EFI_FILE_PROTOCOL *This, UINTN *BufferSize, void *Buffer);
typedef EFI_STATUS (EFIAPI *EFI_FILE_WRITE)(EFI_FILE_PROTOCOL *This, UINTN *BufferSize, void *Buffer);
typedef EFI_STATUS (EFIAPI *EFI_FILE_SET_POSITION)(EFI_FILE_PROTOCOL *This, UINT64 Position);
typedef EFI_STATUS (EFIAPI *EFI_FILE_GET_POSITION)(EFI_FILE_PROTOCOL *This, UINT64 *Position);
typedef EFI_STATUS (EFIAPI *EFI_FILE_GET_INFO)(EFI_FILE_PROTOCOL *This, EFI_GUID *InformationType, UINTN *BufferSize, void *Buffer);
typedef EFI_STATUS (EFIAPI *EFI_FILE_FLUSH)(EFI_FILE_PROTOCOL *This);

struct _EFI_FILE_PROTOCOL {
    UINT64 Revision;
    EFI_FILE_OPEN Open;
    EFI_FILE_CLOSE Close;
    EFI_FILE_DELETE Delete;
    EFI_FILE_READ Read;
    EFI_FILE_WRITE Write;
    EFI_FILE_GET_POSITION GetPosition;
    EFI_FILE_SET_POSITION SetPosition;
    EFI_FILE_GET_INFO GetInfo;
    EFI_FILE_FLUSH Flush;
};

typedef struct _EFI_SIMPLE_FILE_SYSTEM_PROTOCOL EFI_SIMPLE_FILE_SYSTEM_PROTOCOL;
struct _EFI_SIMPLE_FILE_SYSTEM_PROTOCOL {
    UINT64 Revision;
    EFI_STATUS (EFIAPI *OpenVolume)(EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *This, EFI_FILE_PROTOCOL **Root);
};

typedef struct {
    UINT32 Revision;
    EFI_HANDLE ParentHandle;
    EFI_SYSTEM_TABLE *SystemTable;
    EFI_HANDLE DeviceHandle;
    void *FilePath;
    void *Reserved;
    UINT32 LoadOptionsSize;
    void *LoadOptions;
    void *ImageBase;
    UINT64 ImageSize;
} EFI_LOADED_IMAGE_PROTOCOL;

#define EFI_FILE_INFO_ID \
    {0x09576E92, 0x6D3F, 0x11D2, {0x8E, 0x39, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B}}

typedef struct {
    UINT16  Year;
    UINT8   Month;
    UINT8   Day;
    UINT8   Hour;
    UINT8   Minute;
    UINT8   Second;
    UINT8   Pad1;
    UINT32  Nanosecond;
    INT16   TimeZone;
    UINT8   Daylight;
    UINT8   Pad2;
} EFI_TIME;

typedef struct {
    UINT64 Size;
    UINT64 FileSize;
    UINT64 PhysicalSize;
    EFI_TIME CreateTime;
    EFI_TIME LastAccessTime;
    EFI_TIME ModificationTime;
    UINT64 Attribute;
    CHAR16 FileName[1]; 
} EFI_FILE_INFO;

#define EFI_FILE_READ_ONLY   0x0000000000000001
#define EFI_FILE_HIDDEN      0x0000000000000002
#define EFI_FILE_SYSTEM      0x0000000000000004
#define EFI_FILE_DIRECTORY   0x0000000000000010

#define SCAN_UP    0x01
#define SCAN_DOWN  0x02
#define SCAN_RIGHT 0x03
#define SCAN_LEFT  0x04
#define SCAN_ESC   0x17

#define EFI_FILE_MODE_READ   0x0000000000000001
#define EFI_FILE_MODE_WRITE  0x0000000000000002
#define EFI_FILE_MODE_CREATE 0x8000000000000000

#define EFI_LOADED_IMAGE_PROTOCOL_GUID \
    {0x5B1B31A1, 0x9562, 0x11D2, {0x8E, 0x3F, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B}}
#define EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID \
    {0x964E5B22, 0x6459, 0x11D2, {0x8E, 0x39, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B}}

typedef UINT8 BOOLEAN;

typedef struct {
    INT32 RelativeMovementX;
    INT32 RelativeMovementY;
    INT32 RelativeMovementZ;
    BOOLEAN LeftButton;
    BOOLEAN RightButton;
} EFI_SIMPLE_POINTER_STATE;

typedef struct _EFI_SIMPLE_POINTER_PROTOCOL EFI_SIMPLE_POINTER_PROTOCOL;

typedef EFI_STATUS (EFIAPI *EFI_SIMPLE_POINTER_RESET)(
    EFI_SIMPLE_POINTER_PROTOCOL *This,
    BOOLEAN ExtendedVerification
);

typedef EFI_STATUS (EFIAPI *EFI_SIMPLE_POINTER_GET_STATE)(
    EFI_SIMPLE_POINTER_PROTOCOL *This,
    EFI_SIMPLE_POINTER_STATE *State
);

typedef struct {
    UINT64 ResolutionX;
    UINT64 ResolutionY;
    UINT64 ResolutionZ;
    BOOLEAN LeftButton;
    BOOLEAN RightButton;
} EFI_SIMPLE_POINTER_MODE;

struct _EFI_SIMPLE_POINTER_PROTOCOL {
    EFI_SIMPLE_POINTER_RESET Reset;
    EFI_SIMPLE_POINTER_GET_STATE GetState;
    void *WaitForInput;
    EFI_SIMPLE_POINTER_MODE *Mode;
};

#define EFI_SIMPLE_POINTER_PROTOCOL_GUID \
    {0x31878c87, 0x0b75, 0x11d5, {0x9a, 0x4f, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d}}

#endif