#ifndef TEXF_H
#define TEXF_H

#include <stdint.h>

/* T-OS Executable File (TEXF) Header Definition */

#define TEXF_MAGIC_0 0x54  // 'T'
#define TEXF_MAGIC_1 0x4F  // 'O'
#define TEXF_MAGIC_2 0x53  // 'S'

typedef struct __attribute__((packed)) {
    uint8_t magic[3];       // Magic Number: 0x54, 0x4F, 0x53
    uint32_t entry_point;   // Entry Point Address (Offset from load address)
    uint32_t code_size;     // Size of Code Segment in bytes
    uint32_t data_size;     // Size of Data Segment in bytes
} texf_header_t;

#endif
