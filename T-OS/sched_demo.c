// Demo workload for the T-OS CPU scheduler.
//
// These are real kernel threads on their own stacks, switched by switch.s and
// preempted from the timer interrupt. Between them they cover every path the
// policy has: threads that never give the CPU up, threads that block on I/O
// (modelled with sched_sleep), a thread that only ever wants short bursts, a
// pair contending for a mutex, and a low-priority batch job that only makes
// progress because of the aging rule.

#include "sched.h"
#include "serial.h"

static sched_mutex_t demo_lock;

// Shared counter guarded by demo_lock; the reporter checks it for tearing.
static volatile uint64_t guarded_counter;
static volatile uint64_t worker_progress[2];

// Burn roughly a millisecond of CPU without touching the clock, so the timer
// interrupt is what ends the slice.
static void burn(uint32_t units) {
  for (volatile uint32_t i = 0; i < units * 20000u; i++)
    __asm__ volatile("");
}

// Pure CPU hog: never yields. The scheduler has to preempt it.
static void thread_cpu_hog(void *arg) {
  uint64_t n = 0;
  (void)arg;
  for (;;) {
    burn(1);
    n++;
    if ((n % 400) == 0)
      kprintf("[hog] still grinding, iteration %lu\n", (unsigned long)n);
  }
}

// I/O-bound: a short burst of work, then a long wait. Should keep its high
// priority and be dispatched quickly every time it wakes.
static void thread_io(void *arg) {
  const char *name = (const char *)arg;
  uint64_t n = 0;
  for (;;) {
    burn(1);
    n++;
    if ((n % 25) == 0)
      kprintf("[%s] serviced %lu requests at tick %lu\n", name,
              (unsigned long)n, (unsigned long)sched_ticks());
    sched_sleep(20);
  }
}

// Two threads hammering the same mutex. Exercises block, direct hand-off and
// wake through the scheduler.
static void thread_worker(void *arg) {
  uint64_t id = (uint64_t)arg;
  for (;;) {
    sched_mutex_lock(&demo_lock);
    uint64_t v = guarded_counter;
    burn(1); // hold the lock across a preemption point on purpose
    guarded_counter = v + 1;
    sched_mutex_unlock(&demo_lock);

    worker_progress[id]++;
    sched_sleep(3);
  }
}

// Lowest possible base priority and never sleeps: it only ever runs because
// aging promotes it out of the bottom queue.
static void thread_batch(void *arg) {
  uint64_t n = 0;
  (void)arg;
  for (;;) {
    burn(1);
    n++;
    if ((n % 50) == 0)
      kprintf("[batch] chunk %lu done at tick %lu\n", (unsigned long)n,
              (unsigned long)sched_ticks());
  }
}

// Prints the scheduler table every few seconds so the run is observable from
// the serial console.
static void thread_reporter(void *arg) {
  (void)arg;
  for (int round = 1;; round++) {
    sched_sleep(3000); // ~3 s at 1 kHz

    kprintf("\n===== T-OS scheduler report #%d =====\n", round);
    sched_dump_stats();
    kprintf("  guarded counter=%lu  worker progress=%lu/%lu\n",
            (unsigned long)guarded_counter,
            (unsigned long)worker_progress[0],
            (unsigned long)worker_progress[1]);
    kprintf("=====================================\n\n");
  }
}

void sched_demo_start(void) {
  sched_mutex_init(&demo_lock);
  guarded_counter = 0;
  worker_progress[0] = worker_progress[1] = 0;

  //                name          entry            arg          base priority
  sched_create_thread("reporter", thread_reporter, (void *)0, 0);
  sched_create_thread("io-disk", thread_io, (void *)"io-disk", 0);
  sched_create_thread("io-net", thread_io, (void *)"io-net", 1);
  sched_create_thread("worker0", thread_worker, (void *)0, 1);
  sched_create_thread("worker1", thread_worker, (void *)1, 1);
  sched_create_thread("hog", thread_cpu_hog, (void *)0, 2);
  sched_create_thread("batch", thread_batch, (void *)0, SCHED_PRIO_LEVELS - 1);

  kprintf("[sched] %u threads queued, handing the CPU to the scheduler\n",
          sched_runnable_count());
}
