#include "power.h"
#include "../drivers/ports.h"

void shutdown() {
    // 1. QEMU specific (older)
    port_word_out(0x604, 0x2000);

    // 2. VirtualBox / QEMU (newer ACPI)
    // We don't have ACPI, but usually 0x604 works for QEMU.
    // Bochs/QEMU poweroff via port 0xB004
    port_word_out(0xB004, 0x2000);

    // 3. Fallback: Halt loop
    while(1) {
        __asm__ volatile("cli; hlt");
    }
}

void reboot() {
    // 1. 8042 Keyboard Controller Pulse
    unsigned char temp;

    // Clear buffer
    do {
        temp = port_byte_in(0x64);
        if (temp & 1) port_byte_in(0x60);
    } while (temp & 2);

    // Reset command
    port_byte_out(0x64, 0xFE);

    // 2. Fallback (Triple Fault)
    // Load empty IDT
    __asm__ volatile ("lidt 0");
    __asm__ volatile ("int3");
}
