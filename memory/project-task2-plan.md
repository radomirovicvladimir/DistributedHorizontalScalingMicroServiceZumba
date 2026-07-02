---
name: project-task2-plan
description: Task 2 (Threads) implementation plan — non-preemptive kernel, per-thread kernel stack, cooperative yield()
metadata:
  type: project
---

**Task 2 = 10 points.** Deliverables: `thread_create`, `thread_exit`, `thread_dispatch` at all three interface layers (ABI, C API, C++ API), plus a working synchronous context switch. **No timer/preemption** — that's Task 4 and we're skipping it (see [[project-20-points-strategy]]).

## Architectural choices

**Non-preemptive kernel.** Interrupts are masked whenever we're above the trap line. Hardware clears `sstatus.SIE` on trap entry; we simply don't re-enable it. This makes every kernel path a critical section — no scheduler locks, no atomicity worries around the ready queue.

**Per-thread kernel stack.** Each TCB owns one stack, on which BOTH the user body and any nested kernel work (traps, syscalls) run. This is the "višenitni program" model from PDF §"Stek na kom se izvršava kôd jezgra". `yield` swaps stacks between TCBs. Nothing lives on a shared kernel stack.

**Cooperative context switch only.** `yield(oldCtx*, newCtx*)` saves the 14 RISC-V callee-saved regs (`ra, sp, s0-s11`) into `oldCtx` and loads from `newCtx`. Caller-saved regs live on the stack per the C ABI so we don't touch them. This works precisely because switches only happen at well-defined synchronous points (inside `thread_dispatch`, `thread_exit`, and later `sem_wait`).

**Idle thread.** Always-runnable safety net so `Scheduler::get()` never returns nullptr while user threads exist. Its body is `for (;;) asm("wfi");` — `wfi` halts the core until an interrupt would fire, which is fine even in our non-preemptive design (we don't rely on the wake, and there are no timer IRQs firing).

## Data structures

`TCB` (each ~112B context + 4KB stack):
- `uint64 context[14]` — saved callee-saved regs: `[0]=ra`, `[1]=sp`, `[2..13]=s0..s11`
- `void* stack_bottom` — for `free()`; points at the low end
- `void (*body)(void*)` + `void* arg` — for the wrapper
- `TCB* next` — ready-queue link
- `enum State { READY, RUNNING, FINISHED } state`
- Statics: `running`, `mainTCB` (for the pseudo-TCB representing whoever called `main`), `idleTCB`, plus a monotonic id counter for debug.

`Scheduler` — FIFO ready queue; `put(TCB*)`, `TCB* get()`. Idle thread NOT in ready queue — it's the last resort.

## New syscall dispatch (added to trap.cpp)
- 0x11 `SYS_THREAD_CREATE`: `a1=handle**`, `a2=start`, `a3=arg`, `a4=stack_top` → `TCB::create(...)`
- 0x12 `SYS_THREAD_EXIT`: mark running→FINISHED, call `Scheduler::yield()` to next
- 0x13 `SYS_THREAD_DISPATCH`: if ready queue non-empty, put running back + switch

## New C API in syscall.cpp
- `thread_create` allocates stack via `mem_alloc(DEFAULT_STACK_SIZE)`, then `ecall(0x11, handle, start, arg, stack_top)`.
- `thread_exit`, `thread_dispatch` — thin `ecall0`/`ecall`-with-zero-args wrappers.

## New C++ API in syscall_cpp.hpp/.cpp
- `Thread(body, arg)` ctor — stores; `start()` calls `thread_create`. `~Thread` — no-op for now.
- `Thread::dispatch` static → `thread_dispatch`.
- Protected `Thread()` ctor (for derived classes overriding `run`) — sets `body = &Thread::runWrapper`, `arg = this`.
- `Thread::runWrapper` (private static) `static_cast<Thread*>(arg)->run()`.

**Ignore `run()` override if function pointer is set** — PDF §"C++ API" p.10-11 explicitly requires this. So `Thread(fn, arg)` always uses `fn`, never derived `run()`.

## Wrapper function

`extern "C" void thread_body_wrapper() { TCB* self = TCB::running; self->body(self->arg); thread_exit(); }`. Wrapper's address goes into `context[CTX_RA]`. On first switch to a fresh TCB, `yield` loads `ra = &wrapper`, `sp = stack_top`, `s0..s11 = 0`, then `ret` — falling into the wrapper.

## Tests to add to main.cpp
1. Two threads increment shared counters, `thread_dispatch` in a loop → interleave verified.
2. N=8 threads, FIFO order verified by output sequence.
3. Thread exits mid-flight; ready queue drains cleanly; kernel returns to main.
4. Thread creates another thread — nested creation OK.
5. `thread_exit` from within a wrapper (implicit — body returns).

**Do NOT change trap_entry.S save set** for Task 2 — 16 caller-saved regs is enough because `yield` is synchronous (invoked from the C dispatcher, which itself is invoked from asm that saved all caller-saved regs). The C compiler's own prologue in `c_trap_handler`/`Scheduler::yield` handles callee-saved. Only Task 4 (async preemption) needs the full 31-GPR save.
