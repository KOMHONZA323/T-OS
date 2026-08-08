#include "sched.h"

#ifdef SCHED_HOSTTEST
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define sched_log(...) printf(__VA_ARGS__)
static void *sched_alloc(uint64_t n) { return malloc((size_t)n); }
static inline unsigned long sched_irq_save(void) { return 0; }
static inline void sched_irq_restore(unsigned long f) { (void)f; }
static inline void sched_arch_halt(void) {}
#else
#include "serial.h"
extern void *kmalloc(uint64_t size);
#define sched_log(...) kprintf(__VA_ARGS__)
static void *sched_alloc(uint64_t n) { return kmalloc(n); }

// Thread-context scheduler entry points touch the run queues, which the timer
// interrupt also mutates. Mask interrupts for those critical sections.
static inline unsigned long sched_irq_save(void) {
  unsigned long flags;
  __asm__ volatile("pushfq; popq %0; cli" : "=r"(flags)::"memory");
  return flags;
}
static inline void sched_irq_restore(unsigned long flags) {
  __asm__ volatile("pushq %0; popfq" ::"r"(flags) : "memory", "cc");
}
static inline void sched_arch_halt(void) { __asm__ volatile("hlt"); }
#endif

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------

static sched_thread_t threads[SCHED_MAX_THREADS];
static sched_thread_t *rq_head[SCHED_PRIO_LEVELS];
static sched_thread_t *rq_tail[SCHED_PRIO_LEVELS];

static sched_thread_t *current_thread;
static sched_thread_t *idle_thread;

static uint64_t sched_tick_count;
static uint64_t last_boost_tick;
static uint64_t total_switches;
static uint32_t next_tid = 1;
static int need_resched;
static int sched_running;

static const char *state_name(sched_state_t s) {
  switch (s) {
  case THREAD_UNUSED:
    return "unused";
  case THREAD_READY:
    return "ready";
  case THREAD_RUNNING:
    return "run";
  case THREAD_SLEEPING:
    return "sleep";
  case THREAD_BLOCKED:
    return "block";
  case THREAD_ZOMBIE:
    return "zombie";
  }
  return "?";
}

static uint32_t quantum_for(uint8_t prio) {
  return (uint32_t)SCHED_BASE_QUANTUM << prio;
}

// Returns non-zero if the thread's stack guard is intact.
static int stack_ok(sched_thread_t *t) {
  if (!t || !t->stack_base)
    return 1; // the idle thread runs on the boot stack
  const uint64_t *guard = (const uint64_t *)t->stack_base;
  return guard[0] == SCHED_STACK_CANARY && guard[1] == SCHED_STACK_CANARY;
}

static void stack_overflow_panic(sched_thread_t *t) {
#ifndef SCHED_HOSTTEST
  __asm__ volatile("cli");
#endif
  sched_log("\n*** STACK OVERFLOW in thread %u '%s' ***\n", t->tid, t->name);
  sched_log("    stack %p size %lu - raise SCHED_STACK_SIZE\n", t->stack_base,
            (unsigned long)t->stack_size);
#ifndef SCHED_HOSTTEST
  for (;;)
    sched_arch_halt();
#endif
}

// ---------------------------------------------------------------------------
// Run queues
// ---------------------------------------------------------------------------

// Append to the tail of the queue for the thread's current level. Round robin
// within a level falls out of FIFO order plus the quantum expiry in
// sched_tick().
static void rq_push_at(sched_thread_t *t, int stamp) {
  t->state = THREAD_READY;
  t->rq_next = (sched_thread_t *)0;
  if (stamp)
    t->enqueue_tick = sched_tick_count;
  t->age_tick = sched_tick_count;

  uint8_t p = t->prio;
  if (rq_tail[p])
    rq_tail[p]->rq_next = t;
  else
    rq_head[p] = t;
  rq_tail[p] = t;
}

static void rq_push(sched_thread_t *t) { rq_push_at(t, 1); }

// Unlink a thread from whichever level it currently sits on. Used by aging,
// which has to move a thread between queues.
static void rq_remove(sched_thread_t *t) {
  uint8_t p = t->prio;
  sched_thread_t *prev = (sched_thread_t *)0;
  sched_thread_t *it = rq_head[p];

  while (it && it != t) {
    prev = it;
    it = it->rq_next;
  }
  if (!it)
    return;

  if (prev)
    prev->rq_next = it->rq_next;
  else
    rq_head[p] = it->rq_next;
  if (rq_tail[p] == it)
    rq_tail[p] = prev;
  it->rq_next = (sched_thread_t *)0;
}

static sched_thread_t *rq_pop(void) {
  for (int p = 0; p < SCHED_PRIO_LEVELS; p++) {
    sched_thread_t *t = rq_head[p];
    if (t) {
      rq_head[p] = t->rq_next;
      if (!rq_head[p])
        rq_tail[p] = (sched_thread_t *)0;
      t->rq_next = (sched_thread_t *)0;
      return t;
    }
  }
  return (sched_thread_t *)0;
}

// Highest-priority level with a runnable thread, or SCHED_PRIO_LEVELS if the
// run queues are empty.
static int rq_best_level(void) {
  for (int p = 0; p < SCHED_PRIO_LEVELS; p++)
    if (rq_head[p])
      return p;
  return SCHED_PRIO_LEVELS;
}

// Periodic anti-starvation boost: everything runnable goes back to its base
// level. Relative order is preserved so a boosted thread does not jump ahead
// of a thread that has been waiting longer at the same base level.
static void rq_boost_all(void) {
  sched_thread_t *list = (sched_thread_t *)0, *tail = (sched_thread_t *)0;

  for (int p = 0; p < SCHED_PRIO_LEVELS; p++) {
    sched_thread_t *t = rq_head[p];
    while (t) {
      sched_thread_t *next = t->rq_next;
      t->rq_next = (sched_thread_t *)0;
      if (tail)
        tail->rq_next = t;
      else
        list = t;
      tail = t;
      t = next;
    }
    rq_head[p] = rq_tail[p] = (sched_thread_t *)0;
  }

  while (list) {
    sched_thread_t *next = list->rq_next;
    list->prio = list->base_prio;
    // Keep the original enqueue stamp so latency accounting stays honest.
    rq_push_at(list, 0);
    list = next;
  }

  // Threads that are asleep or blocked are boosted too, so they come back at
  // their base level rather than wherever they were demoted to.
  for (int i = 0; i < SCHED_MAX_THREADS; i++) {
    sched_thread_t *t = &threads[i];
    if (t->state == THREAD_SLEEPING || t->state == THREAD_BLOCKED)
      t->prio = t->base_prio;
  }
}

// ---------------------------------------------------------------------------
// Core dispatch
// ---------------------------------------------------------------------------

// Pick the next thread and switch to it.
//
// The caller must already have moved `current_thread` into the state it wants:
// THREAD_READY to be requeued, or SLEEPING/BLOCKED/ZOMBIE to be left off the
// run queues. Interrupts must be masked.
static void sched_reschedule(void) {
  sched_thread_t *prev = current_thread;

  if (prev->state == THREAD_READY && prev != idle_thread)
    rq_push(prev);

  sched_thread_t *next = rq_pop();
  if (!next)
    next = idle_thread;

  need_resched = 0;

  if (next == prev) {
    // Still the best candidate: keep running with a fresh slice.
    prev->state = THREAD_RUNNING;
    prev->slice = quantum_for(prev->prio);
    prev->slice_left = prev->slice;
    return;
  }

  if (next != idle_thread) {
    uint64_t waited = sched_tick_count - next->enqueue_tick;
    if (waited > next->max_latency)
      next->max_latency = waited;
  }

  next->state = THREAD_RUNNING;
  next->slice = quantum_for(next->prio);
  next->slice_left = next->slice;
  next->switches++;
  total_switches++;

  current_thread = next;
  sched_arch_switch(&prev->rsp, next->rsp);
  // Execution resumes here when `prev` is scheduled again.
}

// A thread handing the CPU back before its slice expired is behaving like an
// I/O-bound thread, so it earns a step back towards its base level.
static void credit_voluntary(sched_thread_t *t) {
  t->voluntary++;
  if (t->prio > t->base_prio)
    t->prio--;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void sched_init(void) {
  for (int i = 0; i < SCHED_MAX_THREADS; i++) {
    threads[i].state = THREAD_UNUSED;
    threads[i].rq_next = (sched_thread_t *)0;
    threads[i].wait_next = (sched_thread_t *)0;
  }
  for (int p = 0; p < SCHED_PRIO_LEVELS; p++)
    rq_head[p] = rq_tail[p] = (sched_thread_t *)0;

  current_thread = (sched_thread_t *)0;
  idle_thread = (sched_thread_t *)0;
  sched_tick_count = 0;
  last_boost_tick = 0;
  total_switches = 0;
  next_tid = 1;
  need_resched = 0;
  sched_running = 0;
}

static sched_thread_t *alloc_slot(void) {
  for (int i = 0; i < SCHED_MAX_THREADS; i++)
    if (threads[i].state == THREAD_UNUSED)
      return &threads[i];
  return (sched_thread_t *)0;
}

static void set_name(sched_thread_t *t, const char *name) {
  int i = 0;
  if (name)
    for (; name[i] && i < SCHED_NAME_LEN - 1; i++)
      t->name[i] = name[i];
  t->name[i] = 0;
}

static void init_common(sched_thread_t *t, const char *name, uint8_t prio) {
  if (prio >= SCHED_PRIO_LEVELS)
    prio = SCHED_PRIO_LEVELS - 1;

  t->tid = next_tid++;
  set_name(t, name);
  t->base_prio = prio;
  t->prio = prio;
  t->slice = quantum_for(prio);
  t->slice_left = t->slice;
  t->wake_tick = 0;
  t->cpu_ticks = 0;
  t->ready_ticks = 0;
  t->switches = 0;
  t->preemptions = 0;
  t->voluntary = 0;
  t->max_latency = 0;
  t->promotions = 0;
  t->enqueue_tick = sched_tick_count;
  t->age_tick = sched_tick_count;
  t->rsp = 0;
  t->entry = (void (*)(void *))0;
  t->arg = (void *)0;
  t->stack_base = (void *)0;
  t->stack_size = 0;
  t->rq_next = (sched_thread_t *)0;
  t->wait_next = (sched_thread_t *)0;
}

sched_thread_t *sched_create_thread(const char *name, void (*entry)(void *),
                                    void *arg, uint8_t prio) {
  unsigned long flags = sched_irq_save();

  sched_thread_t *t = alloc_slot();
  if (!t) {
    sched_irq_restore(flags);
    return (sched_thread_t *)0;
  }

  init_common(t, name, prio);
  t->entry = entry;
  t->arg = arg;
  t->stack_size = SCHED_STACK_SIZE;
  t->stack_base = sched_alloc(SCHED_STACK_SIZE);
  if (!t->stack_base) {
    t->state = THREAD_UNUSED;
    sched_irq_restore(flags);
    return (sched_thread_t *)0;
  }

  // Guard words at the low end of the stack; sched_tick() checks them.
  uint64_t *guard = (uint64_t *)t->stack_base;
  guard[0] = SCHED_STACK_CANARY;
  guard[1] = SCHED_STACK_CANARY;

  t->rsp = sched_arch_init_stack(t);
  rq_push(t);

  if (current_thread && t->prio < current_thread->prio)
    need_resched = 1;

  sched_irq_restore(flags);
  return t;
}

// Split out of sched_start() so host tests can bring the scheduler up without
// entering the idle loop.
static void sched_bootstrap(void) {
  sched_thread_t *idle = alloc_slot();
  init_common(idle, "idle", SCHED_PRIO_LEVELS - 1);
  idle->state = THREAD_RUNNING;

  idle_thread = idle;
  current_thread = idle;
  sched_running = 1;
  last_boost_tick = sched_tick_count;
}

#ifdef SCHED_HOSTTEST
void sched_host_bootstrap(void) { sched_bootstrap(); }
#endif

void sched_start(void) {
  sched_bootstrap();

  // The boot context becomes the idle thread: it runs only when every other
  // thread is asleep or blocked.
  while (sched_running) {
    unsigned long flags = sched_irq_save();
    if (rq_best_level() < SCHED_PRIO_LEVELS) {
      current_thread->state = THREAD_READY;
      sched_reschedule();
      sched_irq_restore(flags);
    } else {
      sched_irq_restore(flags);
      sched_arch_halt();
    }
  }
}

void sched_stop(void) { sched_running = 0; }

void sched_yield(void) {
  unsigned long flags = sched_irq_save();
  credit_voluntary(current_thread);
  current_thread->state = THREAD_READY;
  sched_reschedule();
  sched_irq_restore(flags);
}

void sched_sleep(uint64_t ticks) {
  if (ticks == 0) {
    sched_yield();
    return;
  }
  unsigned long flags = sched_irq_save();
  credit_voluntary(current_thread);
  current_thread->wake_tick = sched_tick_count + ticks;
  current_thread->state = THREAD_SLEEPING;
  sched_reschedule();
  sched_irq_restore(flags);
}

void sched_block(void) {
  unsigned long flags = sched_irq_save();
  credit_voluntary(current_thread);
  current_thread->state = THREAD_BLOCKED;
  sched_reschedule();
  sched_irq_restore(flags);
}

void sched_wake(sched_thread_t *t) {
  if (!t)
    return;
  unsigned long flags = sched_irq_save();
  if (t->state == THREAD_BLOCKED || t->state == THREAD_SLEEPING) {
    rq_push(t);
    if (current_thread && t->prio < current_thread->prio)
      need_resched = 1;
  }
  sched_irq_restore(flags);
}

void sched_exit(void) {
  unsigned long flags = sched_irq_save();
  sched_log("[sched] thread %u '%s' exited after %lu ticks of CPU\n",
            current_thread->tid, current_thread->name,
            (unsigned long)current_thread->cpu_ticks);
  current_thread->state = THREAD_ZOMBIE;
  sched_reschedule();
  sched_irq_restore(flags);

#ifndef SCHED_HOSTTEST
  // A zombie is never put back on a run queue, so control never gets here on
  // real hardware. (The host harness has no real stack switch, so there
  // sched_reschedule() does return and the caller carries on.)
  for (;;)
    sched_arch_halt();
#endif
}

sched_thread_t *sched_current(void) { return current_thread; }

uint64_t sched_ticks(void) { return sched_tick_count; }

uint32_t sched_runnable_count(void) {
  uint32_t n = 0;
  for (int p = 0; p < SCHED_PRIO_LEVELS; p++)
    for (sched_thread_t *t = rq_head[p]; t; t = t->rq_next)
      n++;
  return n;
}

uint64_t sched_total_switches(void) { return total_switches; }

// ---------------------------------------------------------------------------
// Timer tick
// ---------------------------------------------------------------------------

int sched_tick(void) {
  sched_tick_count++;

  // 1. Wake any sleeper whose deadline has passed.
  for (int i = 0; i < SCHED_MAX_THREADS; i++) {
    sched_thread_t *t = &threads[i];
    if (t->state == THREAD_SLEEPING && t->wake_tick <= sched_tick_count)
      rq_push(t);
  }

  // 2. Charge waiting time to everything sitting on a run queue, and age the
  //    ones that have been waiting too long. Repeated promotion guarantees any
  //    runnable thread reaches level 0 in bounded time, so nothing starves no
  //    matter how much high-priority load there is.
  for (int i = 0; i < SCHED_MAX_THREADS; i++) {
    sched_thread_t *t = &threads[i];
    if (t->state != THREAD_READY || t == idle_thread)
      continue;

    t->ready_ticks++;
    if (t->prio > 0 && sched_tick_count - t->age_tick >= SCHED_STARVE_TICKS) {
      rq_remove(t);
      t->prio--;
      t->promotions++;
      rq_push_at(t, 0); // keep the original enqueue stamp for max_latency
      need_resched = 1;
    }
  }

  // 3. Charge CPU time to the running thread and expire its slice.
  if (current_thread) {
    if (!stack_ok(current_thread))
      stack_overflow_panic(current_thread);

    current_thread->cpu_ticks++;
    if (current_thread != idle_thread) {
      if (current_thread->slice_left > 0)
        current_thread->slice_left--;
      if (current_thread->slice_left == 0) {
        // Burnt a whole slice: treat as CPU-bound and demote a level.
        current_thread->preemptions++;
        if (current_thread->prio + 1 < SCHED_PRIO_LEVELS)
          current_thread->prio++;
        need_resched = 1;
      }
    }
  }

  // 4. A thread more important than the running one became runnable.
  if (current_thread) {
    int best = rq_best_level();
    if (best < SCHED_PRIO_LEVELS &&
        (current_thread == idle_thread || best < current_thread->prio))
      need_resched = 1;
  }

  // 5. Periodic boost keeps the low levels from starving.
  if (sched_tick_count - last_boost_tick >= SCHED_BOOST_INTERVAL) {
    last_boost_tick = sched_tick_count;
    rq_boost_all();
    need_resched = 1;
  }

  return need_resched;
}

void sched_preempt(void) {
  if (!need_resched || !current_thread)
    return;
  unsigned long flags = sched_irq_save();
  if (current_thread->state == THREAD_RUNNING)
    current_thread->state = THREAD_READY;
  sched_reschedule();
  sched_irq_restore(flags);
}

// ---------------------------------------------------------------------------
// Blocking mutex
// ---------------------------------------------------------------------------

void sched_mutex_init(sched_mutex_t *m) {
  m->locked = 0;
  m->owner = (sched_thread_t *)0;
  m->wait_head = (sched_thread_t *)0;
  m->wait_tail = (sched_thread_t *)0;
}

void sched_mutex_lock(sched_mutex_t *m) {
  unsigned long flags = sched_irq_save();

  if (!m->locked) {
    m->locked = 1;
    m->owner = current_thread;
    sched_irq_restore(flags);
    return;
  }

  // Contended: queue up and block. sched_mutex_unlock() hands the lock over
  // before waking us, so there is no re-acquire race on the way back.
  sched_thread_t *self = current_thread;
  self->wait_next = (sched_thread_t *)0;
  if (m->wait_tail)
    m->wait_tail->wait_next = self;
  else
    m->wait_head = self;
  m->wait_tail = self;

  credit_voluntary(self);
  self->state = THREAD_BLOCKED;
  sched_reschedule();
  sched_irq_restore(flags);
}

void sched_mutex_unlock(sched_mutex_t *m) {
  unsigned long flags = sched_irq_save();

  sched_thread_t *next = m->wait_head;
  if (next) {
    m->wait_head = next->wait_next;
    if (!m->wait_head)
      m->wait_tail = (sched_thread_t *)0;
    next->wait_next = (sched_thread_t *)0;

    m->owner = next; // direct hand-off, the lock stays held
    if (next->state == THREAD_BLOCKED) {
      rq_push(next);
      if (current_thread && next->prio < current_thread->prio)
        need_resched = 1;
    }
  } else {
    m->locked = 0;
    m->owner = (sched_thread_t *)0;
  }

  sched_irq_restore(flags);
}

// ---------------------------------------------------------------------------
// Introspection
// ---------------------------------------------------------------------------

void sched_dump_stats(void) {
  sched_log("\n[sched] tick=%lu switches=%lu runnable=%u\n",
            (unsigned long)sched_tick_count, (unsigned long)total_switches,
            sched_runnable_count());
  sched_log("  tid name          state  lvl base    cpu   wait   disp  preempt "
            " volunt  promo  maxlat\n");
  for (int i = 0; i < SCHED_MAX_THREADS; i++) {
    sched_thread_t *t = &threads[i];
    if (t->state == THREAD_UNUSED)
      continue;
    sched_log("  %3u %-13s %-6s %3u %4u %6lu %6lu %6lu %8lu %7lu %6lu %7lu\n",
              t->tid, t->name, state_name(t->state), t->prio, t->base_prio,
              (unsigned long)t->cpu_ticks, (unsigned long)t->ready_ticks,
              (unsigned long)t->switches, (unsigned long)t->preemptions,
              (unsigned long)t->voluntary, (unsigned long)t->promotions,
              (unsigned long)t->max_latency);
  }
}
