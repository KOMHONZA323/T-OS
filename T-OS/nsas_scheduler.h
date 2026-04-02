#ifndef NSAS_SCHEDULER_H
#define NSAS_SCHEDULER_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    STATE_READY,
    STATE_RUNNING,
    STATE_BLOCKED,
    STATE_NPU_PENDING,
    STATE_NPU_BURST,
    STATE_EXITED
} thread_state_t;

typedef struct {
    uint64_t model_id;
    uint32_t vram_usage_mb;
    float avg_burst_ms;
} ai_burst_stats_t;

typedef struct {
    uint64_t rax, rbx, rcx, rdx, rsi, rdi, rbp, r8, r9, r10, r11, r12, r13, r14, r15;
    uint64_t rip, rflags, rsp;
} cpu_context_t;

// Process Control Block (PCB) - Resource Container
typedef struct Process {
    uint32_t pid;
    uint64_t cr3; // Page Directory (Address Space)
    struct Thread* main_thread;
    struct Process* next;
} pcb_t;

// Thread Control Block (TCB) - Execution Unit
typedef struct Thread {
    uint32_t tid;
    thread_state_t state;
    pcb_t* parent;
    
    int base_priority;
    int inherited_priority; // For Priority Inheritance
    
    // NSAS Specific Fields (Inherited from T-OS)
    ai_burst_stats_t ai_stats;
    float npu_affinity_score;
    uint64_t last_npu_tick;
    uint64_t burst_start_tick;
    
    cpu_context_t context;
    struct Thread* next;      // Global runqueue
    struct Thread* wait_next; // Wait queue for locks
} tcb_t;

typedef struct {
    tcb_t* head;
    uint64_t active_model_id;
    bool is_locked;
    uint64_t ticks_since_switch;
} npu_affinity_queue_t;

#define MAX_THREADS 256
#define NPU_AFFINITY_WINDOW 50

void nsas_init();
void nsas_schedule();
tcb_t* create_thread(pcb_t* parent, void (*entry_point)());
pcb_t* create_process(uint64_t cr3);

#endif
