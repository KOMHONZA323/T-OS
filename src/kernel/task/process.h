#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>

typedef struct Process {
    uint32_t pid;
    uint32_t esp;
    struct Process* next;
} Process;

void create_process(void (*entry)());

#endif
