#ifndef SCHED_H
#define SCHED_H

#include <stdint.h>

// ---------------------------------------------------------------------------
// T-OS CPU scheduler
//
// A preemptive multi-level feedback queue (MLFQ) scheduler for kernel threads.
//
//  * SCHED_PRIO_LEVELS round-robin queues, level 0 is the highest priority.
//  * The quantum grows with the level: interactive threads at the top get
//    short slices and low latency, batch threads at the bottom get long
//    slices and fewer context switches.
//  * A thread that burns its whole slice is demoted one level (it is
//    CPU-bound). A thread that gives the CPU back early - by sleeping or
//    blocking - is promoted back towards its base level (it is I/O-bound).
//  * Two mechanisms keep the bottom of the queue alive. Aging promotes any
//    thread that has been runnable but unscheduled for SCHED_STARVE_TICKS by
//    one level, repeatedly, so even a thread with the worst base priority
//    climbs to level 0 and must be dispatched. On top of that, every
//    SCHED_BOOST_INTERVAL ticks every thread is reset to its base level, which
//    undoes the demotion history of a thread that has changed behaviour.
//
// The policy is deliberately free of inline assembly so it can be compiled and
// exercised on the host (see tests/test_sched.c). The one machine-dependent
// piece, the register/stack switch, lives behind sched_arch_switch().
// ---------------------------------------------------------------------------

#define SCHED_PRIO_LEVELS 4
#define SCHED_MAX_THREADS 32
#define SCHED_NAME_LEN 16

// Ticks of CPU time granted at the top level. Level n gets
// SCHED_BASE_QUANTUM << n ticks.
#define SCHED_BASE_QUANTUM 2

// A runnable thread that has waited this long without being dispatched is
// promoted one level. This is what bounds worst-case latency.
#define SCHED_STARVE_TICKS 50

// Every thread is restored to its base priority this often.
#define SCHED_BOOST_INTERVAL 200

// Default kernel stack for a thread, in bytes. Interrupts are taken on the
// running thread's stack, so this has to cover the deepest kernel call chain
// plus an interrupt frame.
#define SCHED_STACK_SIZE 16384

// Written at the low end of every thread stack at creation and verified on
// every tick. An overflow corrupts the heap silently and shows up much later
// as an unrelated fault; this turns it into an immediate, named failure.
#define SCHED_STACK_CANARY 0x5441434B47554152ULL // "TACKGUAR"

typedef enum {
  THREAD_UNUSED = 0, // slot free
  THREAD_READY,      // on a run queue
  THREAD_RUNNING,    // currently on the CPU
  THREAD_SLEEPING,   // waiting for wake_tick
  THREAD_BLOCKED,    // waiting on a lock or event
  THREAD_ZOMBIE      // returned from its entry point
} sched_state_t;

typedef struct sched_thread {
  uint32_t tid;
  char name[SCHED_NAME_LEN];
  sched_state_t state;

  uint8_t base_prio; // level the thread is boosted back to
  uint8_t prio;      // level it currently sits at
  uint32_t slice;    // quantum for the current level, in ticks
  uint32_t slice_left;

  uint64_t wake_tick; // valid while THREAD_SLEEPING

  // Accounting, reported by sched_dump_stats().
  uint64_t cpu_ticks;    // ticks spent on the CPU
  uint64_t ready_ticks;  // ticks spent waiting on a run queue
  uint64_t switches;     // times this thread was dispatched
  uint64_t preemptions;  // times it was preempted with slice exhausted
  uint64_t voluntary;    // times it yielded/slept/blocked early
  uint64_t max_latency;  // longest single ready-to-run wait, in ticks
  uint64_t promotions;   // times aging lifted it a level
  uint64_t enqueue_tick; // tick at which it last entered a run queue
  uint64_t age_tick;     // reset on enqueue and on every aging promotion

  // Saved stack pointer. The register file lives on the thread's own stack.
  uint64_t rsp;

  void (*entry)(void *);
  void *arg;
  void *stack_base;
  uint64_t stack_size;

  struct sched_thread *rq_next; // run-queue link
  struct sched_thread *wait_next; // link for lock wait queues
} sched_thread_t;

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

// Prepare the scheduler. Must be called before any thread is created.
void sched_init(void);

// Create a runnable thread. prio is clamped to [0, SCHED_PRIO_LEVELS-1].
// Returns NULL if the thread table or the heap is exhausted.
sched_thread_t *sched_create_thread(const char *name, void (*entry)(void *),
                                    void *arg, uint8_t prio);

// Turn the current execution context into the idle thread and dispatch the
// first runnable thread. Returns only when sched_stop() is called.
void sched_start(void);

// Ask the scheduler to wind down; the idle loop returns from sched_start().
void sched_stop(void);

// ---------------------------------------------------------------------------
// Thread operations (called from thread context)
// ---------------------------------------------------------------------------

void sched_yield(void);              // give up the rest of the slice
void sched_sleep(uint64_t ticks);    // block until now + ticks
void sched_block(void);              // block until someone calls sched_wake()
void sched_wake(sched_thread_t *t);  // make a blocked thread runnable again
void sched_exit(void);               // terminate the calling thread

sched_thread_t *sched_current(void);
uint64_t sched_ticks(void);

// ---------------------------------------------------------------------------
// Timer integration (called from the IRQ0 handler)
// ---------------------------------------------------------------------------

// Advance time by one tick: wake sleepers, charge the running thread, apply
// the periodic boost, and decide whether a reschedule is due. Returns non-zero
// when the caller should invoke sched_preempt().
int sched_tick(void);

// Perform a preemptive context switch if one is pending. Safe to call from the
// timer interrupt after the EOI has been sent.
void sched_preempt(void);

// ---------------------------------------------------------------------------
// Blocking mutex built on sched_block()/sched_wake()
//
// Ownership is handed directly to the first waiter on unlock, so a released
// lock cannot be stolen by a thread that arrives later.
// ---------------------------------------------------------------------------

typedef struct {
  int locked;
  sched_thread_t *owner;
  sched_thread_t *wait_head;
  sched_thread_t *wait_tail;
} sched_mutex_t;

void sched_mutex_init(sched_mutex_t *m);
void sched_mutex_lock(sched_mutex_t *m);
void sched_mutex_unlock(sched_mutex_t *m);

// ---------------------------------------------------------------------------
// Introspection
// ---------------------------------------------------------------------------

void sched_dump_stats(void);
uint32_t sched_runnable_count(void);
uint64_t sched_total_switches(void);

// Machine-dependent register/stack switch. Implemented in switch.s for the
// kernel and stubbed out for host tests.
void sched_arch_switch(uint64_t *save_rsp, uint64_t load_rsp);

// Build the initial stack image for a new thread so that the first
// sched_arch_switch() into it lands in the thread trampoline.
uint64_t sched_arch_init_stack(sched_thread_t *t);

#endif // SCHED_H
