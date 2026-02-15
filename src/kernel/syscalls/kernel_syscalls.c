#include "../isr.h"
#include "../../sdk/syscalls.h"
#include "../../drivers/screen.h"
#include "../../drivers/keyboard.h"
#include "../../drivers/timer.h"
#include "../memory/heap.h"
#include "../../drivers/utils.h"
#include "../task/scheduler.h"
#include "../../drivers/filesystem/fat16.h"
#include "../../sdk/texf.h"

#define MAX_FDS 10

typedef struct {
    int active;
    char* buffer;
    uint32_t size;
    uint32_t pos;
} kernel_fd_t;

kernel_fd_t fds[MAX_FDS];

void init_fds() {
    for (int i=0; i<MAX_FDS; i++) fds[i].active = 0;
}

int spawn_process(char* filename) {
    kprint("Spawning: "); kprint(filename); kprint("\n");

    char* proc_buf = (char*)malloc(65536);
    if (!proc_buf) return -1;

    int bytes_read = fat16_read_file(filename, proc_buf);
    if (bytes_read <= 0) {
        kprint("Error: Could not read file.\n");
        free(proc_buf);
        return -1;
    }

    texf_header_t* header = (texf_header_t*)proc_buf;
    if (header->magic[0] != 'T' || header->magic[1] != 'O' || header->magic[2] != 'S') {
        kprint("Error: Invalid Executable Format.\n");
        free(proc_buf);
        return -1;
    }

    void* load_addr = (void*)0x400000;
    // Copy code
    char* code_src = proc_buf + header->code_offset;
    memory_copy(code_src, (char*)load_addr, header->code_size);

    create_process((void (*)())load_addr);

    free(proc_buf);
    return 1;
}

void syscall_handler_c(registers_t *regs) {
    uint32_t syscall_num = regs->eax;

    switch (syscall_num) {
        case SYS_EXIT:
            kprint("Process exited.\n");
            while(1);
            break;

        case SYS_PRINT:
            kprint((char*)regs->ebx);
            break;

        case SYS_GET_CHAR:
            regs->eax = 0;
            break;

        case SYS_CREATE_WINDOW:
            kprint("Syscall: Create Window\n");
            regs->eax = 1;
            break;

        case SYS_SPAWN_PROCESS:
            regs->eax = spawn_process((char*)regs->ebx);
            break;
// ... rest same as before ...
        case SYS_SLEEP:
            delay(regs->ebx);
            break;

        case SYS_OPEN:
            // Find free FD
            int fd = -1;
            for (int i=0; i<MAX_FDS; i++) {
                if (!fds[i].active) { fd = i; break; }
            }
            if (fd == -1) { regs->eax = -1; break; }

            fds[fd].buffer = (char*)malloc(65536);
            if (!fds[fd].buffer) { regs->eax = -1; break; }

            int sz = fat16_read_file((char*)regs->ebx, fds[fd].buffer);
            if (sz < 0) {
                if (regs->ecx & O_CREAT) {
                    sz = 0; // New file
                } else {
                    free(fds[fd].buffer);
                    fds[fd].active = 0;
                    regs->eax = -1;
                    break;
                }
            }
            fds[fd].size = sz;
            fds[fd].pos = 0;
            fds[fd].active = 1;
            regs->eax = fd;
            break;

        case SYS_READ:
            if (regs->ebx < 0 || regs->ebx >= MAX_FDS || !fds[regs->ebx].active) {
                regs->eax = -1; break;
            }
            kernel_fd_t* rfd = &fds[regs->ebx];

            uint32_t available = rfd->size - rfd->pos;
            uint32_t to_read = regs->edx;
            if (to_read > available) to_read = available;

            memory_copy(rfd->buffer + rfd->pos, (char*)regs->ecx, to_read);
            rfd->pos += to_read;
            regs->eax = to_read;
            break;

        case SYS_WRITE:
            if (regs->ebx == 1) { // STDOUT
                char* buf = (char*)regs->ecx;
                uint32_t count = regs->edx;
                for (uint32_t i = 0; i < count; i++) {
                    char c[2];
                    c[0] = buf[i];
                    c[1] = 0;
                    kprint(c);
                }
                regs->eax = regs->edx;
            } else {
                if (regs->ebx < 0 || regs->ebx >= MAX_FDS || !fds[regs->ebx].active) {
                    regs->eax = -1; break;
                }
                kernel_fd_t* wfd = &fds[regs->ebx];
                if (wfd->pos + regs->edx > 65536) {
                    regs->eax = -1; break;
                }
                memory_copy((char*)regs->ecx, wfd->buffer + wfd->pos, regs->edx);
                wfd->pos += regs->edx;
                if (wfd->pos > wfd->size) wfd->size = wfd->pos;
                regs->eax = regs->edx;
            }
            break;

        case SYS_CLOSE:
            if (regs->ebx >= 0 && regs->ebx < MAX_FDS && fds[regs->ebx].active) {
                free(fds[regs->ebx].buffer);
                fds[regs->ebx].active = 0;
                regs->eax = 0;
            } else {
                regs->eax = -1;
            }
            break;

        case SYS_MALLOC:
            regs->eax = (uint32_t)malloc(regs->ebx);
            break;

        case SYS_FREE:
            free((void*)regs->ebx);
            break;

        default:
            kprint("Unknown Syscall: ");
            char buf[32];
            int_to_ascii(syscall_num, buf);
            kprint(buf);
            kprint("\n");
            break;
    }
}
