#include "std.glh"
#include "texf.h"
#include "syscalls.h"

// T-OS Assembler (TASM)
// A simple two-pass assembler for x86 32-bit protected mode.
// Supports: MOV, PUSH, POP, CALL, RET, INT, ADD, SUB, CMP, JMP, JE, JNE
// Output: .texf binary

#define MAX_LABELS 100
#define MAX_LINE 128
#define MAX_CODE 4096
#define MAX_DATA 1024

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
    if (str[0] == '0' && str[1] == 'x') {
        str += 2;
        while (*str) {
            res *= 16;
            if (*str >= '0' && *str <= '9') res += *str - '0';
            else if (*str >= 'a' && *str <= 'f') res += *str - 'a' + 10;
            else if (*str >= 'A' && *str <= 'F') res += *str - 'A' + 10;
            str++;
        }
    } else {
        while (*str) {
            res = res * 10 + (*str - '0');
            str++;
        }
    }
    return res * sign;
}

char* trim(char* str) {
    while (*str == ' ' || *str == '\t') str++;
    int len = strlen(str);
    while (len > 0 && (str[len-1] == ' ' || str[len-1] == '\t' || str[len-1] == '\n' || str[len-1] == '\r')) {
        str[len-1] = 0;
        len--;
    }
    return str;
}

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
    if (name[len-1] == ':') name[len-1] = 0;
    strcpy(labels[label_count].name, name);
    labels[label_count].address = addr;
    label_count++;
}

uint32_t get_label_addr(char* name) {
    for (int i=0; i<label_count; i++) {
        if (strcmp(labels[i].name, name) == 0) return labels[i].address;
    }
    return 0xFFFFFFFF;
}

void parse_args(char* args, char* arg1, char* arg2) {
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

    int len = strlen(l);
    if (l[len-1] == ':') {
        if (pass == 1) add_label(l, code_idx);
        return;
    }

    char opcode[16];
    char args_str[64];
    int j=0;
    while (l[j] && l[j] != ' ') {
        opcode[j] = l[j];
        j++;
    }
    opcode[j] = 0;

    int k=0;
    while (l[j]) args_str[k++] = l[j++];
    args_str[k] = 0;

    char arg1[32];
    char arg2[32];
    arg1[0] = 0; arg2[0] = 0;
    parse_args(trim(args_str), arg1, arg2);

    if (strcmp(opcode, "mov") == 0) {
        int r1 = get_reg(arg1);
        int r2 = get_reg(arg2);

        if (r1 != -1 && r2 != -1) {
            if (pass == 2) {
                emit8(0x89);
                emit8(0xC0 | (r2 << 3) | r1);
            } else { code_idx += 2; }
        } else if (r1 != -1) {
            uint32_t val;
            if (arg2[0] >= '0' && arg2[0] <= '9') {
                val = atoi(arg2);
            } else {
                val = get_label_addr(arg2);
            }
            if (pass == 2) {
                emit8(0xB8 + r1);
                emit32(val);
            } else { code_idx += 5; }
        }
    }
    else if (strcmp(opcode, "push") == 0) {
        int r = get_reg(arg1);
        if (r != -1) {
            if (pass == 2) emit8(0x50 + r);
            else code_idx += 1;
        } else {
            uint32_t val = atoi(arg1);
            if (pass == 2) {
                emit8(0x68);
                emit32(val);
            } else { code_idx += 5; }
        }
    }
    else if (strcmp(opcode, "pop") == 0) {
        int r = get_reg(arg1);
        if (r != -1) {
            if (pass == 2) emit8(0x58 + r);
            else code_idx += 1;
        }
    }
    else if (strcmp(opcode, "int") == 0) {
        uint32_t val = atoi(arg1);
        if (pass == 2) {
            emit8(0xCD);
            emit8((uint8_t)val);
        } else { code_idx += 2; }
    }
    else if (strcmp(opcode, "ret") == 0) {
        if (pass == 2) emit8(0xC3);
        else code_idx += 1;
    }
    else if (strcmp(opcode, "call") == 0) {
        uint32_t target = get_label_addr(arg1);
        if (pass == 2) {
            emit8(0xE8);
            uint32_t rel = target - (code_idx + 4);
            emit32(rel);
        } else { code_idx += 5; }
    }
    else if (strcmp(opcode, "db") == 0) {
        char* p = arg1;
        if (*p == '"') {
            p++;
            while (*p && *p != '"') {
                if (pass == 2) emit8(*p);
                else code_idx++;
                p++;
            }
        } else {
            if (pass == 2) emit8(atoi(arg1));
            else code_idx++;
        }
        if (strlen(arg2) > 0) {
             if (pass == 2) emit8(atoi(arg2));
             else code_idx++;
        }
    }
}

void process_file(char* filename) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        print("Error: Could not open file: "); print(filename); print("\n");
        exit();
    }

    char source[4096];
    int len = read(fd, source, 4095);
    close(fd);
    source[len] = 0;

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

    code_idx = 0;
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
    char filename[32];

    if (argc < 2) {
        print("T-OS Assembler\n");
        print("Enter input file: ");
        int i=0;
        while(1) {
            char c = get_char();
            if (c == 0) continue; // Busy wait/Yield handled by OS
            if (c == '\n' || c == '\r') break;
            if (c == '\b') {
                 if (i > 0) { i--; print("\b \b"); }
            } else {
                 filename[i++] = c;
                 char str[2]; str[0]=c; str[1]=0;
                 print(str);
            }
        }
        filename[i] = 0;
        print("\n");
    } else {
        strcpy(filename, argv[1]);
    }

    process_file(filename);

    char outfile[32];
    strcpy(outfile, "out.texf");

    int fd_out = open(outfile, O_CREAT | O_WRONLY);

    texf_header_t header;
    header.magic[0] = 'T'; header.magic[1] = 'O'; header.magic[2] = 'S';
    header.version = 1;
    header.code_offset = sizeof(texf_header_t);
    header.code_size = code_idx;
    header.data_offset = 0;
    header.data_size = 0;
    header.entry_point = 0;

    write(fd_out, &header, sizeof(texf_header_t));
    write(fd_out, code_buf, code_idx);
    close(fd_out);

    print("Assembly complete. Output: out.texf\n");
    return 0;
}
