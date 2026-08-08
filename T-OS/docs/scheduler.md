# The T-OS CPU scheduler

T-OS runs kernel threads under a preemptive multi-level feedback queue (MLFQ)
scheduler. The timer interrupt drives it, a real x86-64 register/stack switch
implements it, and `kmain` itself becomes the idle thread once it starts.

## Files

| File | Contents |
|---|---|
| `sched.h` | Public API, tunables, the TCB, and the blocking mutex |
| `sched.c` | Policy: run queues, quanta, demotion, aging, boost, sleep/block/wake |
| `sched_arch.c` | Builds the initial stack image for a new thread |
| `switch.s` | `sched_arch_switch()` and the new-thread trampoline |
| `sched_demo.c` | The demo workload the kernel starts at boot |
| `tests/test_sched.c` | Host-side test suite for the policy |
| `exceptions.s`, `idt.c` | Fault reporting, so a scheduler bug is diagnosable |

## Policy

Four priority levels, level 0 highest. Each level is a FIFO run queue; picking
the next thread means taking the head of the highest non-empty level, which is
round robin within a level.

**Quantum grows with the level.** Level *n* gets `SCHED_BASE_QUANTUM << n`
ticks (2, 4, 8, 16 at a 1 kHz tick). Threads near the top get short slices and
low dispatch latency; threads near the bottom get long slices and fewer context
switches.

**Demotion.** A thread that burns a whole slice is CPU-bound, and drops one
level. That is what keeps a spinning thread from monopolising the top queue.

**Promotion on voluntary release.** A thread that gives the CPU back early - by
sleeping, blocking, or yielding - moves one level back towards its base
priority. Interactive and I/O-bound work therefore settles near the top on its
own, without anyone declaring it "interactive".

**Aging bounds the worst case.** Any thread that has been runnable but
unscheduled for `SCHED_STARVE_TICKS` is promoted one level, repeatedly. Even a
thread created at the worst base priority climbs to level 0 in bounded time and
must then be dispatched. This is what makes the policy starvation-free under
permanent high-priority load - demotion alone does not, because a high-priority
thread that always yields early is never demoted.

**Periodic boost.** Every `SCHED_BOOST_INTERVAL` ticks every thread is reset to
its base level, which discards stale demotion history when a thread's behaviour
changes.

Tunables live at the top of `sched.h`.

## Mechanism

`sched_arch_switch(&prev->rsp, next->rsp)` pushes the System V callee-saved
registers plus RFLAGS onto the outgoing thread's stack, saves RSP into the
outgoing TCB, loads the incoming RSP and pops the mirror image. Everything else
is already caller-saved across the call, and the IRQ0 stub in `entry.s` saves
the caller-saved set.

A new thread's stack is pre-loaded with that exact image by
`sched_arch_init_stack()`, with the return address pointing at
`sched_thread_trampoline`, which calls the entry point and then `sched_exit()`.
So the first dispatch of a new thread and the resumption of an old one are the
same code path.

Preemption happens in `timer_handler()` (`kernel.c`):

```c
int resched = sched_tick();
outb(0x20, 0x20);          // EOI before the switch, not after
if (resched) sched_preempt();
```

The EOI ordering matters: once `sched_preempt()` switches stacks, control does
not return to that instruction until the outgoing thread is scheduled again,
and the PIC would stay masked until then.

Thread-context entry points (`sched_yield`, `sched_sleep`, `sched_block`,
`sched_exit`, the mutex) mask interrupts around the queue manipulation, since
the timer interrupt mutates the same queues.

## Safety nets

* Every thread stack carries a guard word pair checked on each tick. An
  overflow reports `*** STACK OVERFLOW in thread N ***` and halts, instead of
  corrupting the heap and faulting somewhere unrelated later.
* Vectors 0-31 point at `exceptions.s`, which prints the vector, error code,
  RIP, RSP, CR2, the register file, and which thread was running.
* Vectors 32-255 get a stub that acknowledges the PICs, so a spurious IRQ7 or
  IRQ15 cannot escalate into a triple fault and a silent reboot.

## Testing

`tests/test_sched.c` compiles `sched.c` for the host with `-DSCHED_HOSTTEST`,
which swaps the assembly switch for a stub. The harness then acts as the CPU:
it advances one tick at a time and applies the modelled behaviour of whichever
thread the scheduler picked. That exercises the real queue and policy code
deterministically. Nine cases cover round-robin fairness, priority ordering,
CPU-bound demotion, I/O-bound promotion, anti-starvation aging, sleep/wake
timing, mutex hand-off, the idle thread, and thread exit.

```bash
./tests/run_tests.sh
```

## Seeing it run

```bash
mkdir -p build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain-x86_64.cmake .. && make image
cd ..
./scripts/run_qemu.sh --headless --timeout 60 --serial-log sched.log
```

`sched_demo.c` starts a reporter, two I/O-bound threads, two workers contending
on a mutex, a CPU hog, and a lowest-priority batch job. Every three seconds the
reporter dumps the scheduler table to COM1:

```
[sched] tick=81029 switches=92357 runnable=7
  tid name          state  lvl base    cpu   wait   disp  preempt  volunt  promo  maxlat
    1 reporter      run      0    0     28     28     30        2      27      0       1
    2 io-disk       ready    0    0      2   4077   4050        0    4050      0       2
    6 hog           ready    2    2  30491  50538  11903        0       0      0      13
    7 batch         ready    3    3  23478  57551   9012        0       0    260      62
    8 desktop       ready    2    1  27026  45258  10356      772     583      0       7
    9 idle          ready    3    3      0      0      0        0       0      0       0
  guarded counter=52958  worker progress=26479/26479
```

Reading that: the I/O threads keep `maxlat` at 2-4 ticks because they always
release the CPU early; `batch` has the worst base priority and never sleeps, yet
`promo=260` shows aging repeatedly lifting it out of the bottom queue and it
still gets a quarter of the CPU; and the guarded counter exactly equals the sum
of the two workers' progress, so the blocking mutex held under real preemption.
