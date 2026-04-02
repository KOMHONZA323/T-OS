#include "t_sync.h"
#include "nsas_scheduler.h"
#include <stdbool.h>

extern tcb_t* current_tcb;
extern void nsas_schedule();

void spin_init(spinlock_t* lock) {
    lock->locked = 0;
    lock->holder_cpu = 0xFFFFFFFF;
}

void spin_lock(spinlock_t* lock) {
    while (__sync_lock_test_and_set(&lock->locked, 1)) {
        // Pause instruction for power saving on x86
        __asm__ volatile("pause");
    }
}

void spin_unlock(spinlock_t* lock) {
    __sync_lock_release(&lock->locked);
}

void mutex_init(mutex_t* m) {
    spin_init(&m->wait_lock);
    m->locked = false;
    m->owner = (void*)0;
    m->wait_queue.head = (void*)0;
    m->wait_queue.tail = (void*)0;
}

void mutex_lock(mutex_t* m) {
    spin_lock(&m->wait_lock);

    if (__sync_bool_compare_and_swap(&m->locked, false, true)) {
        m->owner = current_tcb;
        spin_unlock(&m->wait_lock);
        return;
    }

    // Priority Inheritance
    if (current_tcb && m->owner && current_tcb->base_priority < m->owner->base_priority) {
        m->owner->inherited_priority = current_tcb->base_priority;
    }

    // Enqueue current thread and sleep
    current_tcb->state = STATE_BLOCKED;
    if (m->wait_queue.tail) {
        m->wait_queue.tail->wait_next = current_tcb;
    } else {
        m->wait_queue.head = current_tcb;
    }
    m->wait_queue.tail = current_tcb;
    current_tcb->wait_next = (void*)0;

    spin_unlock(&m->wait_lock);
    nsas_schedule(); // Yield to other threads
}

void mutex_unlock(mutex_t* m) {
    spin_lock(&m->wait_lock);

    m->locked = false;
    if (m->owner) m->owner->inherited_priority = -1;
    m->owner = (void*)0;

    if (m->wait_queue.head) {
        tcb_t* next = m->wait_queue.head;
        m->wait_queue.head = next->wait_next;
        if (!m->wait_queue.head) m->wait_queue.tail = (void*)0;

        next->state = STATE_READY;
        m->locked = true;
        m->owner = next;
    }

    spin_unlock(&m->wait_lock);
}
