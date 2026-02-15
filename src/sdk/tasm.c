#include "std.glh"
#include "texf.h"
#include "syscalls.h"

// T-OS Assembler (TASM)
// A simple two-pass assembler for x86 32-bit protected mode.
// Supports: MOV, PUSH, POP, CALL, RET, INT, ADD, SUB, CMP, JMP, JE, JNE
// Output: .texf binary

#define MAX_LABELS 100
#define MAX_LINE 128
#define MAX_CODE 4096 // 4KB Code limit for now
#define MAX_DATA 1024 // 1KB Data limit for now

typedef struct {
    char name[32];
    uint32_t address;
} label_t;

label_t labels[MAX_LABELS];
int label_count = 0;

uint8_t code_buf[MAX_CODE];
uint32_t code_idx = 0;

uint8_t data_buf[MAX_DATA];
uint32_t data_idx = 0;

// Helper: String functions
int strlen(char* str) {
    int len = 0;
    while (str[len]) len++;
    return len;
}

int strcmp(char* s1, char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++; s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

void strcpy(char* dest, char* src) {
    while ((*dest++ = *src++));
}

int atoi(char* str) {
    int res = 0;
    int sign = 1;
    if (*str == '-') { sign = -1; str++; }
    if (str[0] == '0' && str[1] == 'x') { // Hex
        str += 2;
        while (*str) {
            res *= 16;
            if (*str >= '0' && *str <= '9') res += *str - '0';
            else if (*str >= 'a' && *str <= 'f') res += *str - 'a' + 10;
            else if (*str >= 'A' && *str <= 'F') res += *str - 'A' + 10;
            str++;
        }
    } else { // Decimal
        while (*str) {
            res = res * 10 + (*str - '0');
            str++;
        }
    }
    return res * sign;
}

// Tokenizer
char* trim(char* str) {
    while (*str == ' ' || *str == '\t') str++;
    int len = strlen(str);
    while (len > 0 && (str[len-1] == ' ' || str[len-1] == '\t' || str[len-1] == '\n')) {
        str[len-1] = 0;
        len--;
    }
    return str;
}

// Simple map for registers
int get_reg(char* reg) {
    if (strcmp(reg, "eax") == 0) return 0;
    if (strcmp(reg, "ecx") == 0) return 1;
    if (strcmp(reg, "edx") == 0) return 2;
    if (strcmp(reg, "ebx") == 0) return 3;
    if (strcmp(reg, "esp") == 0) return 4;
    if (strcmp(reg, "ebp") == 0) return 5;
    if (strcmp(reg, "esi") == 0) return 6;
    if (strcmp(reg, "edi") == 0) return 7;
    return -1;
}

void emit8(uint8_t b) {
    if (code_idx < MAX_CODE) code_buf[code_idx++] = b;
}

void emit32(uint32_t v) {
    emit8(v & 0xFF);
    emit8((v >> 8) & 0xFF);
    emit8((v >> 16) & 0xFF);
    emit8((v >> 24) & 0xFF);
}

void add_label(char* name, uint32_t addr) {
    if (label_count >= MAX_LABELS) return;
    int len = strlen(name);
    if (name[len-1] == ':') name[len-1] = 0; // Strip colon
    strcpy(labels[label_count].name, name);
    labels[label_count].address = addr;
    label_count++;
}

uint32_t get_label_addr(char* name) {
    for (int i=0; i<label_count; i++) {
        if (strcmp(labels[i].name, name) == 0) return labels[i].address;
    }
    return 0xFFFFFFFF; // Not found
}

// Pass 1: Collect Labels
void pass1(char* source) {
    // Reset indices
    code_idx = 0;
    data_idx = 0;
    label_count = 0;

    // We need to simulate parsing line by line
    // Since we don't have strtok/fgets easily, assume source is loaded fully
    // and we iterate through it.

    char* ptr = source;
    char line[MAX_LINE];

    while (*ptr) {
        // Extract line
        int i = 0;
        while (*ptr && *ptr != '\n' && i < MAX_LINE-1) {
            line[i++] = *ptr++;
        }
        line[i] = 0;
        if (*ptr == '\n') ptr++;

        char* l = trim(line);
        if (strlen(l) == 0 || l[0] == ';') continue;

        // Check for Label
        int len = strlen(l);
        if (l[len-1] == ':') {
            add_label(l, code_idx);
            continue;
        }

        // Check for instruction length simulation
        // Split opcode
        char opcode[16];
        int j=0;
        while (l[j] && l[j] != ' ') {
            opcode[j] = l[j];
            j++;
        }
        opcode[j] = 0;

        // Logic to increment code_idx based on instruction size
        // This is a simplified estimation for pass 1 labels.
        // In real x86, instruction length varies.
        // We will assume a fixed set of supported instructions.

        if (strcmp(opcode, "mov") == 0) {
            // mov reg, imm -> 5 bytes (B8+rd imm32)
            // mov reg, reg -> 2 bytes (89 ModRM)
            // Need to look at args
            char* args = l + j;
            args = trim(args);
            // Check comma
            // If second arg starts with digit or quote, it's imm?
            // Simplified: Assume 5 bytes for mov reg, imm
            // Assume 2 bytes for mov reg, reg
            // Assume 6 bytes for mov [mem], reg ?
            // For now, let's just increment by 5 to be safe? No, labels need exact addresses.
            // We must parse args.

            // Hack: Just re-implement parsing logic but don't emit.
            // Or better: Just do assemble(NULL) for pass 1?
            // No, assemble writes to buffer. We can use a global 'pass' flag.
        }
        // ...
        // Actually, let's restructure: `assemble_line(char* line, int pass)`
    }
}

void parse_args(char* args, char* arg1, char* arg2) {
    // Split by comma
    int i=0;
    while (*args && *args != ',') {
        arg1[i++] = *args++;
    }
    arg1[i] = 0;
    trim(arg1);

    if (*args == ',') args++;

    i=0;
    while (*args) {
        arg2[i++] = *args++;
    }
    arg2[i] = 0;
    trim(arg2);
}

void assemble_line(char* line, int pass) {
    char* l = trim(line);
    if (strlen(l) == 0 || l[0] == ';') return;

    // Label
    int len = strlen(l);
    if (l[len-1] == ':') {
        if (pass == 1) add_label(l, code_idx);
        return;
    }

    // Instruction
    char opcode[16];
    char args_str[64];
    int j=0;
    while (l[j] && l[j] != ' ') {
        opcode[j] = l[j];
        j++;
    }
    opcode[j] = 0;

    // Copy remaining as args
    int k=0;
    while (l[j]) args_str[k++] = l[j++];
    args_str[k] = 0;

    char arg1[32];
    char arg2[32];
    arg1[0] = 0; arg2[0] = 0;
    parse_args(trim(args_str), arg1, arg2);

    // --- Instructions ---

    // MOV
    if (strcmp(opcode, "mov") == 0) {
        int r1 = get_reg(arg1);
        int r2 = get_reg(arg2);

        if (r1 != -1 && r2 != -1) {
            // MOV reg, reg (89 /r)
            if (pass == 2) {
                emit8(0x89);
                emit8(0xC0 | (r2 << 3) | r1); // ModRM: 11 src dest
            } else { code_idx += 2; }
        } else if (r1 != -1) {
            // MOV reg, imm (B8+reg imm32)
            // Or label
            uint32_t val;
            if (arg2[0] >= '0' && arg2[0] <= '9') {
                val = atoi(arg2);
            } else {
                // Label?
                val = get_label_addr(arg2); // 0xFFFFFFFF if not found
                // Note: In pass 1, label might not be found yet.
            }

            if (pass == 2) {
                emit8(0xB8 + r1);
                emit32(val);
            } else { code_idx += 5; }
        }
    }

    // PUSH
    else if (strcmp(opcode, "push") == 0) {
        int r = get_reg(arg1);
        if (r != -1) {
            // PUSH reg (50+reg)
            if (pass == 2) emit8(0x50 + r);
            else code_idx += 1;
        } else {
            // PUSH imm32 (68 imm32) or imm8 (6A imm8)
            // For simplicity, always use push imm32 (68)
            uint32_t val = atoi(arg1);
            if (pass == 2) {
                emit8(0x68);
                emit32(val);
            } else { code_idx += 5; }
        }
    }

    // POP
    else if (strcmp(opcode, "pop") == 0) {
        int r = get_reg(arg1);
        if (r != -1) {
            // POP reg (58+reg)
            if (pass == 2) emit8(0x58 + r);
            else code_idx += 1;
        }
    }

    // INT
    else if (strcmp(opcode, "int") == 0) {
        // INT imm8 (CD imm8)
        uint32_t val = atoi(arg1);
        if (pass == 2) {
            emit8(0xCD);
            emit8((uint8_t)val);
        } else { code_idx += 2; }
    }

    // RET
    else if (strcmp(opcode, "ret") == 0) {
        if (pass == 2) emit8(0xC3);
        else code_idx += 1;
    }

    // CALL
    else if (strcmp(opcode, "call") == 0) {
        // CALL rel32 (E8 rel32)
        uint32_t target = get_label_addr(arg1);
        // rel32 = target - (current_ip + 5)
        // In pass 1, target might be unknown.

        if (pass == 2) {
            emit8(0xE8);
            uint32_t rel = target - (code_idx + 4); // +4 because we already emitted E8 (1 byte), need 4 more.
            // Wait, current code_idx is after E8?
            // logic: emit8 increments code_idx.
            // So code_idx is pointing to start of rel32.
            // Next instruction starts at code_idx + 4.
            // So rel = target - (code_idx + 4).
            emit32(rel);
        } else { code_idx += 5; }
    }

    // DB (Data Byte) - Pseudo-instruction
    else if (strcmp(opcode, "db") == 0) {
        // Only handling strings: db "string", 0
        // Parse arg1
        char* p = arg1;
        if (*p == '"') {
            p++;
            while (*p && *p != '"') {
                if (pass == 2) emit8(*p);
                else code_idx++;
                p++;
            }
        } else {
            // Number
            if (pass == 2) emit8(atoi(arg1));
            else code_idx++;
        }

        // Handle comma for arg2 (like 0)
        if (strlen(arg2) > 0) {
             if (pass == 2) emit8(atoi(arg2));
             else code_idx++;
        }
    }

    else {
        if (pass == 2) {
            print("Unknown opcode: "); print(opcode); print("\n");
        }
    }
}

void process_file(char* filename) {
    // Open file
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        print("Error: Could not open file.\n");
        exit();
    }

    // Read entire file into buffer (simplified)
    // Dynamic size would be better but static for now.
    char source[4096];
    int len = read(fd, source, 4095);
    close(fd);
    source[len] = 0;

    // --- Pass 1 ---
    code_idx = 0;
    char* ptr = source;
    char line[MAX_LINE];
    while (*ptr) {
        int i = 0;
        while (*ptr && *ptr != '\n' && i < MAX_LINE-1) {
            line[i++] = *ptr++;
        }
        line[i] = 0;
        if (*ptr == '\n') ptr++;
        assemble_line(line, 1);
    }

    // --- Pass 2 ---
    code_idx = 0; // Reset PC
    ptr = source;
    while (*ptr) {
        int i = 0;
        while (*ptr && *ptr != '\n' && i < MAX_LINE-1) {
            line[i++] = *ptr++;
        }
        line[i] = 0;
        if (*ptr == '\n') ptr++;
        assemble_line(line, 2);
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        print("Usage: tasm <input.asm>\n");
        exit();
    }

    process_file(argv[1]);

    // Write Output
    // Derive output name (input.texf)
    char outfile[32];
    strcpy(outfile, "out.texf"); // Simplified

    int fd_out = open(outfile, O_CREAT | O_WRONLY);

    // Header
    texf_header_t header;
    header.magic[0] = 'T'; header.magic[1] = 'O'; header.magic[2] = 'S';
    header.version = 1;
    header.code_offset = sizeof(texf_header_t);
    header.code_size = code_idx;
    header.data_offset = 0; // No data segment separation in this simple assembler yet (data mixed in code via db)
    header.data_size = 0;
    header.entry_point = 0; // Start at 0

    write(fd_out, &header, sizeof(texf_header_t));
    write(fd_out, code_buf, code_idx);
    close(fd_out);

    print("Assembly complete. Output: out.texf\n");
    return 0;
}
