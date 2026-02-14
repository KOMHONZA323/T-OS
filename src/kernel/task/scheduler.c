#include "scheduler.h"
#include "process.h"
#include "../memory/heap.h"
#include "../../drivers/utils.h"

Process* current_process = 0;
Process* process_list = 0;
uint32_t next_pid = 1;

void init_scheduler() {
    current_process = (Process*)malloc(sizeof(Process));
    current_process->pid = 0;
    current_process->esp = 0;
    current_process->next = 0;
    process_list = current_process;
}

void create_process(void (*entry)()) {
    Process* p = (Process*)malloc(sizeof(Process));
    p->pid = next_pid++;

    uint32_t* stack = (uint32_t*)malloc(4096);
    uint32_t* top = stack + 1024;

    // IRET Frame
    *--top = 0x202; // EFLAGS
    *--top = 0x08;  // CS
    *--top = (uint32_t)entry; // EIP

    // Error Code, Int No
    *--top = 0;
    *--top = 32;

    // PUSHA
    *--top = 0; // EAX
    *--top = 0; // ECX
    *--top = 0; // EDX
    *--top = 0; // EBX
    *--top = 0; // ESP
    *--top = 0; // EBP
    *--top = 0; // ESI
    *--top = 0; // EDI

    // DS
    *--top = 0x10;

    p->esp = (uint32_t)top;
    p->next = 0;

    Process* temp = process_list;
    while (temp->next) temp = temp->next;
    temp->next = p;
}

registers_t* schedule(registers_t *regs) {
    if (!current_process) return regs;

    current_process->esp = (uint32_t)regs;

    if (current_process->next) {
        current_process = current_process->next;
    } else {
        current_process = process_list;
    }

    return (registers_t*)current_process->esp;
}
