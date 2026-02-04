/*
 * T-OS Minimal Kernel
 * Freestanding C Environment
 */

#define VIDEO_MEMORY 0xb8000
#define WHITE_ON_BLACK 0x0f

void print_string(char* message);

void kernel_main() {
    print_string("Welcome to T-OS Kernel (C Code)");

    // Halt
    while(1);
}

void print_string(char* message) {
    char* video_memory = (char*) VIDEO_MEMORY;
    // We need to offset, but for now let's just write to the second line (offset 160)
    // to avoid overwriting the bootloader message if possible,
    // or just overwrite the beginning if we prefer.
    // Let's write to 0xb8000 + 160 (2nd line).

    int i = 0;
    int offset = 160;

    while (message[i] != 0) {
        video_memory[offset + i*2] = message[i];
        video_memory[offset + i*2 + 1] = WHITE_ON_BLACK;
        i++;
    }
}
