#ifndef T_SYNC_H
#define T_SYNC_H

#include <stdint.h>
#include <stdbool.h>

// Forward declaration of TCB for the wait queue
struct Thread;

typedef struct {
    volatile int locked;
    uint32_t holder_cpu;
} spinlock_t;

typedef struct {
    struct Thread* head;
    struct Thread* tail;
} tcb_wait_queue_t;

typedef struct {
    spinlock_t wait_lock;
    volatile bool locked;
    struct Thread* owner;
    tcb_wait_queue_t wait_queue;
} mutex_t;

// Spinlock Functions
void spin_init(spinlock_t* lock);
void spin_lock(spinlock_t* lock);
void spin_unlock(spinlock_t* lock);

// Mutex Functions
void mutex_init(mutex_t* m);
void mutex_lock(mutex_t* m);
void mutex_unlock(mutex_t* m);

#endif
