#include <stdint.h>

// User-space shell for T-OS
void shell_main() {
    const char* prompt = "T-OS > ";
    // Use syscalls to write to CLI and read keyboard
    while (1) {
        // Read line
        // Parse command
        // Execute
    }
}
