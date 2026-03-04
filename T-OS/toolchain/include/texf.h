#ifndef _TEXF_H_
#define _TEXF_H_

#include <stdint.h>

// T-OS Executable Format (TEXF) Header (64 bytes)
typedef struct {
    uint32_t magic;      // 0x46584554 ("TEXF")
    uint32_t version;    // 0x00000001
    uint64_t entry;      // Virtual Address (default: 0x401000)
    uint64_t flags;      // Bit 0: C; Bit 1: HolyC; Bit 2: Static; Bit 3: Kernel
    uint64_t ph_off;     // Program Header Table offset
    uint16_t ph_count;   // Number of Program Headers
    uint64_t sh_off;     // Section Header Table offset
    uint8_t  pad[14];    // Pad to 64 bytes
} texf_header_t;

#define TEXF_MAGIC 0x46584554
#define TEXF_FLAG_C      (1 << 0)
#define TEXF_FLAG_HOLYC  (1 << 1)
#define TEXF_FLAG_STATIC (1 << 2)
#define TEXF_FLAG_KERNEL (1 << 3)

// TEXF Program Header
typedef struct {
    uint32_t type;       // Segment Type
    uint32_t flags;      // R/W/X Permissions
    uint64_t offset;     // Offset in file
    uint64_t vaddr;      // Virtual Address in memory
    uint64_t filesz;     // Size in file
    uint64_t memsz;      // Size in memory
    uint64_t align;      // Memory Alignment
} texf_phdr_t;

#define TEXF_PT_LOAD_CODE 1
#define TEXF_PT_LOAD_DATA 2

#define TEXF_PF_X (1 << 0)
#define TEXF_PF_W (1 << 1)
#define TEXF_PF_R (1 << 2)

#endif
