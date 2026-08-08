// Host-side test suite for the T-OS CPU scheduler.
//
// sched.c is compiled with -DSCHED_HOSTTEST so the machine-dependent register
// switch is replaced by the stub below. Nothing actually runs on a second
// stack here: the harness *is* the CPU. It advances time one tick at a time
// and, for whichever thread the scheduler picked, applies that thread's
// modelled behaviour (spin, yield, sleep, take a lock). That exercises the
// real policy code - the queues, the quanta, demotion, aging, the periodic
// boost, sleep/wake and the blocking mutex - deterministically and without a
// VM in the loop.
//
// Build and run:  gcc -DSCHED_HOSTTEST -I.. ../sched.c test_sched.c -o t && ./t

#include "../sched.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

// --------------------------------------------------------------------------
// Arch stubs
// --------------------------------------------------------------------------

void sched_arch_switch(uint64_t *save_rsp, uint64_t load_rsp) {
  // No real stack switch on the host: record the outgoing stack pointer so the
  // bookkeeping stays consistent and return to the caller.
  (void)load_rsp;
  if (save_rsp)
    *save_rsp = 0;
}

uint64_t sched_arch_init_stack(sched_thread_t *t) {
  (void)t;
  return 0;
}

void sched_host_bootstrap(void);

// --------------------------------------------------------------------------
// Assertions
// --------------------------------------------------------------------------

static int failures;
static int checks;

static void check(int cond, const char *what) {
  checks++;
  if (cond) {
    printf("  [PASS] %s\n", what);
  } else {
    failures++;
    printf("  [FAIL] %s\n", what);
  }
}

static void checkf(int cond, const char *what, const char *detail_fmt,
                   long a, long b) {
  checks++;
  if (cond) {
    printf("  [PASS] %s\n", what);
  } else {
    failures++;
    printf("  [FAIL] %s (", what);
    printf(detail_fmt, a, b);
    printf(")\n");
  }
}

// --------------------------------------------------------------------------
// Simulation harness
// --------------------------------------------------------------------------

typedef enum {
  BEHAVE_SPIN = 0,  // never gives the CPU up, always preempted
  BEHAVE_YIELD,     // yields after one tick on the CPU
  BEHAVE_SLEEP,     // runs one tick, then sleeps `param` ticks
  BEHAVE_LOCK       // takes the shared mutex, holds it `param` ticks
} behaviour_t;

typedef struct {
  sched_thread_t *t;
  behaviour_t how;
  int param;
  int held; // ticks the lock has been held so far
  int holds_lock;
} actor_t;

#define MAX_ACTORS 8
static actor_t actors[MAX_ACTORS];
static int actor_count;
static sched_mutex_t shared_lock;

// Trace of which tid was on the CPU at each tick.
#define MAX_TRACE 4096
static uint32_t trace[MAX_TRACE];
static int trace_len;

static void reset_world(void) {
  sched_init();
  sched_host_bootstrap();
  sched_mutex_init(&shared_lock);
  actor_count = 0;
  trace_len = 0;
  memset(actors, 0, sizeof(actors));
}

static actor_t *add_actor(const char *name, uint8_t prio, behaviour_t how,
                          int param) {
  actor_t *a = &actors[actor_count++];
  a->t = sched_create_thread(name, (void (*)(void *))0, (void *)0, prio);
  a->how = how;
  a->param = param;
  a->held = 0;
  a->holds_lock = 0;
  return a;
}

static actor_t *actor_of(sched_thread_t *t) {
  for (int i = 0; i < actor_count; i++)
    if (actors[i].t == t)
      return &actors[i];
  return (actor_t *)0;
}

// Advance the simulated CPU by `n` ticks.
static void run_ticks(int n) {
  for (int i = 0; i < n; i++) {
    if (sched_tick())
      sched_preempt();

    sched_thread_t *cur = sched_current();
    if (trace_len < MAX_TRACE)
      trace[trace_len++] = cur->tid;

    actor_t *a = actor_of(cur);
    if (!a)
      continue; // the idle thread

    switch (a->how) {
    case BEHAVE_SPIN:
      break;
    case BEHAVE_YIELD:
      sched_yield();
      break;
    case BEHAVE_SLEEP:
      sched_sleep((uint64_t)a->param);
      break;
    case BEHAVE_LOCK:
      if (!a->holds_lock) {
        sched_mutex_lock(&shared_lock);
        // On the host the lock call returns immediately even when it blocked,
        // so only claim ownership if we really got it.
        if (shared_lock.owner == a->t) {
          a->holds_lock = 1;
          a->held = 0;
        }
      } else if (++a->held >= a->param) {
        sched_mutex_unlock(&shared_lock);
        a->holds_lock = 0;
      }
      break;
    }
  }
}

static int ticks_for(uint32_t tid, int from, int to) {
  int n = 0;
  for (int i = from; i < to && i < trace_len; i++)
    if (trace[i] == tid)
      n++;
  return n;
}

static int first_tick_of(uint32_t tid) {
  for (int i = 0; i < trace_len; i++)
    if (trace[i] == tid)
      return i;
  return -1;
}

// --------------------------------------------------------------------------
// Tests
// --------------------------------------------------------------------------

// Equal-priority CPU hogs should split the CPU evenly.
static void test_round_robin_fairness(void) {
  printf("\nTC-01 round-robin fairness between equal-priority threads\n");
  reset_world();

  actor_t *a = add_actor("rr-a", 1, BEHAVE_SPIN, 0);
  actor_t *b = add_actor("rr-b", 1, BEHAVE_SPIN, 0);
  actor_t *c = add_actor("rr-c", 1, BEHAVE_SPIN, 0);

  run_ticks(600);

  int ta = ticks_for(a->t->tid, 0, trace_len);
  int tb = ticks_for(b->t->tid, 0, trace_len);
  int tc = ticks_for(c->t->tid, 0, trace_len);
  printf("       cpu ticks: a=%d b=%d c=%d\n", ta, tb, tc);

  int lo = ta < tb ? (ta < tc ? ta : tc) : (tb < tc ? tb : tc);
  int hi = ta > tb ? (ta > tc ? ta : tc) : (tb > tc ? tb : tc);

  checkf(hi - lo <= 30, "CPU split within 5% between three hogs",
         "spread %ld ticks over %ld", hi - lo, (long)trace_len);
  check(ta > 150 && tb > 150 && tc > 150, "every thread made progress");
  check(ticks_for(0, 0, trace_len) == 0, "idle never ran while work existed");
}

// A high-priority thread runs before a low-priority one.
static void test_priority_ordering(void) {
  printf("\nTC-02 higher priority runs first\n");
  reset_world();

  actor_t *lo = add_actor("low", 3, BEHAVE_SPIN, 0);
  actor_t *hi = add_actor("high", 0, BEHAVE_SPIN, 0);

  run_ticks(40);

  int first_hi = first_tick_of(hi->t->tid);
  int first_lo = first_tick_of(lo->t->tid);
  printf("       first dispatch: high@%d low@%d\n", first_hi, first_lo);

  check(first_hi == 0, "high-priority thread is dispatched first");
  check(first_lo > first_hi, "low-priority thread waits for it");
}

// A thread that always burns its whole slice is CPU-bound and sinks.
static void test_cpu_bound_demotion(void) {
  printf("\nTC-03 CPU-bound thread is demoted\n");
  reset_world();

  actor_t *hog = add_actor("hog", 0, BEHAVE_SPIN, 0);
  run_ticks(30);

  printf("       level=%u base=%u preemptions=%lu\n", hog->t->prio,
         hog->t->base_prio, (unsigned long)hog->t->preemptions);

  check(hog->t->preemptions > 0, "slice expiry preempted the hog");
  check(hog->t->prio > hog->t->base_prio, "hog sank below its base level");
}

// A thread that sleeps instead of spinning keeps its interactive standing and
// gets more CPU per dispatch than the hog it competes with.
static void test_io_bound_keeps_priority(void) {
  printf("\nTC-04 I/O-bound thread keeps its priority\n");
  reset_world();

  actor_t *hog = add_actor("hog", 1, BEHAVE_SPIN, 0);
  actor_t *io = add_actor("io", 1, BEHAVE_SLEEP, 3);

  run_ticks(400);

  printf("       hog level=%u  io level=%u  io voluntary=%lu\n", hog->t->prio,
         io->t->prio, (unsigned long)io->t->voluntary);

  check(io->t->voluntary > 50, "I/O thread released the CPU voluntarily");
  check(io->t->prio <= hog->t->prio,
        "I/O thread sits at or above the hog's level");
  check(io->t->max_latency <= SCHED_STARVE_TICKS,
        "I/O thread is dispatched promptly after waking");
}

// The point of aging: a thread with the worst base priority still runs, even
// against permanent high-priority load that never burns a full slice.
static void test_no_starvation(void) {
  printf("\nTC-05 anti-starvation aging\n");
  reset_world();

  // Four level-0 threads that yield every tick. They never burn a slice, so
  // demotion alone would never move them out of the way.
  for (int i = 0; i < 4; i++)
    add_actor("greedy", 0, BEHAVE_YIELD, 0);
  actor_t *poor = add_actor("poor", SCHED_PRIO_LEVELS - 1, BEHAVE_SPIN, 0);

  run_ticks(1000);

  printf("       poor: cpu=%lu promotions=%lu maxlat=%lu\n",
         (unsigned long)poor->t->cpu_ticks,
         (unsigned long)poor->t->promotions,
         (unsigned long)poor->t->max_latency);

  check(poor->t->cpu_ticks > 0, "lowest-priority thread got the CPU");
  check(poor->t->promotions > 0, "aging promoted it out of the bottom queue");
  checkf(poor->t->max_latency <= SCHED_STARVE_TICKS * SCHED_PRIO_LEVELS + 20,
         "worst-case wait is bounded by the aging interval",
         "maxlat=%ld bound=%ld", (long)poor->t->max_latency,
         (long)(SCHED_STARVE_TICKS * SCHED_PRIO_LEVELS + 20));
}

// A sleeping thread must not be dispatched before its deadline, and must be
// runnable promptly afterwards.
static void test_sleep_wake(void) {
  printf("\nTC-06 sleep and wake timing\n");
  reset_world();

  actor_t *filler = add_actor("filler", 2, BEHAVE_SPIN, 0);
  actor_t *sleeper = add_actor("sleeper", 0, BEHAVE_SLEEP, 10);

  run_ticks(60);

  // The sleeper runs one tick, sleeps 10, runs again... so it should appear
  // roughly every 11 ticks and never twice in a row.
  int runs = ticks_for(sleeper->t->tid, 0, trace_len);
  int back_to_back = 0;
  for (int i = 1; i < trace_len; i++)
    if (trace[i] == sleeper->t->tid && trace[i - 1] == sleeper->t->tid)
      back_to_back++;

  printf("       sleeper ran %d times in %d ticks, %d back-to-back\n", runs,
         trace_len, back_to_back);

  check(back_to_back == 0, "sleeper never runs two ticks in a row");
  check(runs >= 4 && runs <= 7, "sleeper wakes once per sleep period");
  check(filler->t->cpu_ticks > 45, "filler used the CPU the sleeper gave up");
}

// The blocking mutex must hand ownership to the first waiter, in FIFO order,
// and blocked threads must not be dispatched.
static void test_mutex_handoff(void) {
  printf("\nTC-07 blocking mutex hand-off\n");
  reset_world();

  actor_t *a = add_actor("lock-a", 1, BEHAVE_SPIN, 0);
  actor_t *b = add_actor("lock-b", 1, BEHAVE_SPIN, 0);
  actor_t *c = add_actor("lock-c", 1, BEHAVE_SPIN, 0);

  // Drive the lock explicitly rather than through the behaviour loop so the
  // ordering assertions are unambiguous.
  sched_thread_t *saved = sched_current();
  (void)saved;

  // A acquires.
  sched_preempt();
  check(sched_current() == a->t, "thread A is running");
  sched_mutex_lock(&shared_lock);
  check(shared_lock.owner == a->t, "A owns the lock");

  // B blocks behind A.
  while (sched_current() != b->t)
    run_ticks(1);
  sched_mutex_lock(&shared_lock);
  check(b->t->state == THREAD_BLOCKED, "B blocked on the contended lock");

  // C blocks behind B.
  while (sched_current() != c->t)
    run_ticks(1);
  sched_mutex_lock(&shared_lock);
  check(c->t->state == THREAD_BLOCKED, "C blocked behind B");

  int b_ticks_before = ticks_for(b->t->tid, 0, trace_len);
  run_ticks(20);
  check(ticks_for(b->t->tid, 0, trace_len) == b_ticks_before,
        "blocked thread is never dispatched");

  // A releases: ownership goes straight to B, the first waiter.
  sched_mutex_unlock(&shared_lock);
  check(shared_lock.owner == b->t, "lock handed to the first waiter (B)");
  check(b->t->state == THREAD_READY, "B is runnable again");
  check(c->t->state == THREAD_BLOCKED, "C still waits its turn");

  sched_mutex_unlock(&shared_lock);
  check(shared_lock.owner == c->t, "lock then handed to C");

  sched_mutex_unlock(&shared_lock);
  check(shared_lock.locked == 0, "lock free once the queue drains");
}

// The idle thread only runs when there is genuinely nothing else to do.
static void test_idle(void) {
  printf("\nTC-08 idle thread\n");
  reset_world();

  actor_t *s = add_actor("napper", 1, BEHAVE_SLEEP, 20);
  run_ticks(30);

  int idle_ticks = 0;
  for (int i = 0; i < trace_len; i++)
    if (trace[i] != s->t->tid)
      idle_ticks++;

  printf("       idle ran %d of %d ticks\n", idle_ticks, trace_len);
  check(idle_ticks > 20, "idle covers the gap while the only thread sleeps");
  check(sched_runnable_count() == 0, "run queues empty while it sleeps");
}

// Exiting removes a thread from consideration.
static void test_exit(void) {
  printf("\nTC-09 thread exit\n");
  reset_world();

  actor_t *a = add_actor("short", 1, BEHAVE_SPIN, 0);
  actor_t *b = add_actor("long", 1, BEHAVE_SPIN, 0);

  run_ticks(20);
  while (sched_current() != a->t)
    run_ticks(1);
  sched_exit();

  check(a->t->state == THREAD_ZOMBIE, "exited thread became a zombie");

  int before = ticks_for(a->t->tid, 0, trace_len);
  run_ticks(50);
  check(ticks_for(a->t->tid, 0, trace_len) == before,
        "zombie is never dispatched again");
  check(b->t->cpu_ticks > 20, "survivor keeps running");
}

int main(void) {
  setvbuf(stdout, (char *)0, _IONBF, 0);
  printf("=== T-OS CPU scheduler test suite ===\n");
  printf("levels=%d base quantum=%d aging=%d boost=%d\n", SCHED_PRIO_LEVELS,
         SCHED_BASE_QUANTUM, SCHED_STARVE_TICKS, SCHED_BOOST_INTERVAL);

  test_round_robin_fairness();
  test_priority_ordering();
  test_cpu_bound_demotion();
  test_io_bound_keeps_priority();
  test_no_starvation();
  test_sleep_wake();
  test_mutex_handoff();
  test_idle();
  test_exit();

  printf("\n=== %d checks, %d failures ===\n", checks, failures);
  return failures ? 1 : 0;
}
