#include "../isr.h"
#include "../../sdk/syscalls.h"
#include "../../drivers/screen.h"
#include "../../drivers/keyboard.h"
#include "../../drivers/timer.h"

// Forward declarations of kernel functions
// We can include headers, but for now let's just prototype what we need or include them.
// screen.h -> kprint, etc.

void syscall_handler_c(registers_t *regs) {
    uint32_t syscall_num = regs->eax;

    // Arguments in EBX, ECX, EDX, ESI, EDI

    switch (syscall_num) {
        case SYS_EXIT:
            kprint("Process exited.\n");
            // For now, just hang or restart shell?
            // In a real OS, we'd schedule the next process or kill this one.
            // Since we don't have a full process manager with destructors yet:
            while(1);
            break;

        case SYS_PRINT:
            // EBX = char* message
            kprint((char*)regs->ebx);
            break;

        case SYS_GET_CHAR:
            // Return char in EAX
            // get_char() is in keyboard.c but it blocks?
            // The existing get_char() in kernel.c (not driver) uses a buffer.
            // Let's assume we can call a non-blocking or blocking keyboard function.
            // drivers/keyboard.c has keyboard_handler, but we need a consumer.
            // Let's use the one from kernel.c? Accessing kernel globals is messy.
            // Ideally, keyboard driver should have a 'get_key' function.
            // For now, let's return 0 if no key, or block.
            // Implementation detail: we need to expose a 'get_queued_key' from keyboard driver.
            // Let's just return 0 for now to avoid linker errors, will fix in integration.
            regs->eax = 0;
            break;

        case SYS_CREATE_WINDOW:
            // EBX = width, ECX = height, EDX = title
            // Returns window ID in EAX
            // We need to implement a window manager hook.
            // For Stage 2, let's just print "Window Created" and return a dummy ID.
            kprint("Syscall: Create Window\n");
            regs->eax = 1;
            break;

        case SYS_SPAWN_PROCESS:
            // EBX = filename
            kprint("Syscall: Spawn Process\n");
            regs->eax = 1;
            break;

        case SYS_SLEEP:
            // EBX = ms
            // We need a sleep function. 'delay' in utils.c is busy wait.
            // We can use that for now.
             // We need to include utils.h or declare it.
            // Let's just do a busy loop or nothing for now to satisfy the symbol.
            break;

        default:
            kprint("Unknown Syscall\n");
            break;
    }
}
