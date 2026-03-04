#include <stdio.h>
#include <string.h>
#include <stdint.h>

void assemble_line(const char* line) {
    if (strstr(line, "syscall")) {
        // Emit 0x0F 0x05
    } else if (strstr(line, "nop")) {
        // Emit 0x90
    }
}

int main(int argc, char** argv) {
    // TASM - T-OS Assembler
    return 0;
}
