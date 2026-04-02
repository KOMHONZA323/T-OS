#include "../nsas_scheduler.h"
#include "../t_sync.h"
#include "../compositor.h"
#include <stdint.h>

extern tcb_t* global_runqueue;
extern volatile uint64_t timer_ticks;
extern void nsas_schedule();
extern tcb_t* current_tcb;

#define TEST_ASSERT(cond, msg) \
    if (!(cond)) { \
        compositor_print("[FAIL] ", 0xFF0000); \
        compositor_print(msg, 0xFF0000); \
        compositor_print("\n", 0xFF0000); \
        return; \
    } else { \
        compositor_print("[PASS] ", 0x00FF00); \
        compositor_print(msg, 0x00FF00); \
        compositor_print("\n", 0x00FF00); \
    }

void run_nsas_tests() {
    compositor_print("--- NSAS SMP Threading & Sync Test Suite ---\n", 0xFFFF00);

    pcb_t* p1 = create_process(0x1000000);
    tcb_t* t1 = create_thread(p1, (void*)0x1000);
    
    // TC-01: Thread Initialization
    TEST_ASSERT(t1 != 0, "TCB Allocation");
    TEST_ASSERT(t1->parent == p1, "PCB/TCB Linkage");
    TEST_ASSERT(t1->state == STATE_READY, "Initial State");

    // TC-02: Mutex Basic Lock
    mutex_t m1;
    mutex_init(&m1);
    current_tcb = t1;
    t1->state = STATE_RUNNING;
    
    mutex_lock(&m1);
    TEST_ASSERT(m1.locked == true, "Mutex Lock Acquire");
    TEST_ASSERT(m1.owner == t1, "Mutex Ownership");

    // TC-03: Mutex Contention & Blocking
    tcb_t* t2 = create_thread(p1, (void*)0x2000);
    t2->base_priority = 5; // Higher priority (lower number)
    
    // Manually simulate T2 trying to acquire lock while T1 holds it
    tcb_t* original_tcb = current_tcb;
    current_tcb = t2;
    // We expect this to block T2
    mutex_lock(&m1); 
    
    TEST_ASSERT(t2->state == STATE_BLOCKED, "Thread Blocking on Mutex");
    TEST_ASSERT(t1->inherited_priority == 5, "Priority Inheritance (T1 inherits T2's priority)");

    // TC-04: Mutex Release & Wakeup
    current_tcb = original_tcb;
    mutex_unlock(&m1);
    
    TEST_ASSERT(m1.locked == true, "Mutex Re-locked by Waiting Thread");
    TEST_ASSERT(m1.owner == t2, "Mutex Hand-off to T2");
    TEST_ASSERT(t2->state == STATE_READY, "Blocked Thread Woken Up");

    compositor_print("--- NSAS SMP Sync Tests Complete ---\n", 0xFFFF00);
}
