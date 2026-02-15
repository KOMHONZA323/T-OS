#ifndef TEXF_H
#define TEXF_H

typedef unsigned char uint8_t;
typedef unsigned int uint32_t;

// T-OS Executable File (.texf) Header
// Structure:
// [Magic: 3 bytes] [Version: 1 byte]
// [Code Offset: 4 bytes] [Code Size: 4 bytes]
// [Data Offset: 4 bytes] [Data Size: 4 bytes]
// [Entry Point: 4 bytes]

#define TEXF_MAGIC_0 'T'
#define TEXF_MAGIC_1 'O'
#define TEXF_MAGIC_2 'S'
#define TEXF_VERSION 1

typedef struct {
    uint8_t magic[3];     // 'T', 'O', 'S'
    uint8_t version;      // Version number
    uint32_t code_offset; // File offset to code segment
    uint32_t code_size;   // Size of code segment in bytes
    uint32_t data_offset; // File offset to data segment
    uint32_t data_size;   // Size of data segment in bytes
    uint32_t entry_point; // Entry point offset (relative to code segment start?)
                          // Let's make it relative to the start of the file for simplicity in loading.
} __attribute__((packed)) texf_header_t;

#endif
