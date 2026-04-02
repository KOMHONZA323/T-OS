#include "nsas_scheduler.h"
#include "compositor.h"
#include "t_sync.h"
#include <stdint.h>

extern void* kmalloc(uint64_t size);
extern volatile uint64_t timer_ticks;

tcb_t* global_runqueue = 0;
npu_affinity_queue_t npu_queue = {0, 0, false, 0};
tcb_t* current_tcb = 0;

static uint32_t next_pid = 1;
static uint32_t next_tid = 1;

pcb_t* create_process(uint64_t cr3) {
    pcb_t* p = (pcb_t*)kmalloc(sizeof(pcb_t));
    if (!p) return 0;
    p->pid = next_pid++;
    p->cr3 = cr3;
    p->main_thread = 0;
    p->next = 0;
    return p;
}

tcb_t* create_thread(pcb_t* parent, void (*entry_point)()) {
    tcb_t* t = (tcb_t*)kmalloc(sizeof(tcb_t));
    if (!t) return 0;
    
    t->tid = next_tid++;
    t->state = STATE_READY;
    t->parent = parent;
    t->base_priority = 10;
    t->inherited_priority = -1;
    t->npu_affinity_score = 0.0f;
    t->ai_stats.model_id = 0;
    t->last_npu_tick = timer_ticks;
    
    t->context.rip = (uint64_t)entry_point;
    t->context.rsp = (uint64_t)kmalloc(4096) + 4096;
    t->context.rflags = 0x202;
    
    // Add to global runqueue
    t->next = global_runqueue;
    global_runqueue = t;
    
    if (parent && !parent->main_thread) {
        parent->main_thread = t;
    }
    
    return t;
}

void nsas_init() {
    compositor_print("NSAS SMP Multi-threading Initialized\n", 0x00FFFF);
}

tcb_t* select_next_cpu_thread() {
    tcb_t* t = global_runqueue;
    while (t) {
        if (t->state == STATE_READY) return t;
        t = t->next;
    }
    return 0;
}

tcb_t* select_next_npu_thread() {
    tcb_t* t = global_runqueue;
    // Simple affinity check
    while (t) {
        if (t->state == STATE_NPU_PENDING && t->ai_stats.model_id == npu_queue.active_model_id) {
            return t;
        }
        t = t->next;
    }
    
    if (npu_queue.ticks_since_switch > NPU_AFFINITY_WINDOW) {
        t = global_runqueue;
        while (t) {
            if (t->state == STATE_NPU_PENDING) return t;
            t = t->next;
        }
    }
    return 0;
}

void nsas_schedule() {
    if (npu_queue.is_locked) return;
    
    // NPU Logic
    tcb_t* next_npu = select_next_npu_thread();
    if (next_npu) {
        if (next_npu->ai_stats.model_id != npu_queue.active_model_id) {
            npu_queue.active_model_id = next_npu->ai_stats.model_id;
            npu_queue.ticks_since_switch = 0;
            compositor_print("[NSAS] NPU Context Switch (SMP Mode)\n", 0xFFAA00);
        }
        next_npu->state = STATE_NPU_BURST;
    }

    // CPU Logic
    tcb_t* next_cpu = select_next_cpu_thread();
    if (next_cpu && next_cpu != current_tcb) {
        if (current_tcb && current_tcb->state == STATE_RUNNING) {
            current_tcb->state = STATE_READY;
        }
        
        // Context Switch Logic: CR3 Reload optimization
        if (current_tcb && current_tcb->parent != next_cpu->parent) {
            // In a real SMP kernel, we'd reload CR3 here:
            // load_cr3(next_cpu->parent->cr3);
        }
        
        current_tcb = next_cpu;
        current_tcb->state = STATE_RUNNING;
    }
    
    npu_queue.ticks_since_switch++;
}
