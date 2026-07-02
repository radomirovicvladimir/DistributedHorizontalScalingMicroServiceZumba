---
name: project-task2-plan
description: Task 2 (Threads) implementation status — non-preemptive kernel, per-thread kernel stack, cooperative yield(), wired to OS12026 public tests
metadata:
  type: project
---

**Task 2 = 10 points, IMPLEMENTED.** Deliverables: `thread_create`, `thread_exit`, `thread_dispatch` at all three interface layers (ABI, C API, C++ API), plus a working synchronous context switch. **No timer/preemption** — that's Task 4 and we're skipping it (see [[project-20-points-strategy]]).

## Architectural choices

**Non-preemptive kernel.** Interrupts are masked whenever we're above the trap line. Hardware clears `sstatus.SIE` on trap entry; we don't re-enable it. Every kernel path is one big critical section.

**Per-thread stack, shared with kernel.** Each TCB owns one 4KB stack. User body runs on it in U-mode; when a trap fires, the trap frame gets pushed onto the SAME stack (no separate kernel stack). Works because there's no MMU/PMP — U-mode has full physical access.

**User threads run in U-mode.** TCB::create for user threads lays down a synthetic trap-return frame on the new thread's stack with `sstatus.SPP=0, SPIE=1, sepc=&body_wrapper`. Fresh-thread first entry:
1. `context_switch` loads `ra = &trap_return_tail` and `sp = &synthetic_frame`, does `ret`.
2. `trap_return_tail` (label at the tail of `src/trap_entry.S`) restores CSRs and GPRs from the frame, `sret` → U-mode at `body_wrapper`.
3. body_wrapper runs user body in U-mode. On return, calls `thread_exit()` (C API, `ecall`) rather than kernel-side `TCB::exit()` — proper syscall boundary.

**Kernel threads (idle, mainTCB) stay in S-mode.** They use the plain body_wrapper entry — `context_switch` `ret`s straight into it in S-mode.

**Cooperative context switch only.** `context_switch(old_ctx*, new_ctx*)` saves the 14 RISC-V callee-saved regs (`ra, sp, s0-s11`) into `old_ctx` and loads from `new_ctx`. Caller-saved regs live on the stack per the C ABI.

**Test 7 handling.** `csrr sepc` in U-mode raises scause=2 (illegal instruction). Our trap handler catches this (non-ecall exception), kills the offending thread, and yields. The busy-wait loop in `System_Mode_test` never sees `finishedB=true`, so the test never completes regularly — exactly what tests/uputstvo.txt requires.

**Idle thread.** Always-runnable safety net so `Scheduler::switch_to_next()` never returns nullptr. Its body is `for (;;) asm("wfi");` — S-mode instruction.

## Data structures

- `TCB` — 14-reg context, stack_bottom, body/arg, next-link, state (READY/RUNNING/FINISHED), is_kernel flag.
- `Scheduler` — head+tail linked queue, O(1) put/get.
- **Graveyard** — deferred-free list for FINISHED TCBs; reaped by the next thread's return from `context_switch` (safe because they're on a different stack).

## Files

- `h/TCB.hpp`, `src/TCB.cpp` — TCB with `create`/`exit`/`dispatch`/`init`, body_wrapper trampoline, idle thread.
- `h/Scheduler.hpp`, `src/Scheduler.cpp` — FIFO ready queue, switch_to_next(), graveyard reaper.
- `src/context_switch.S` — saves/restores 14 callee-saved regs.
- `src/trap.cpp` — dispatcher for SYS_THREAD_CREATE (0x11), SYS_THREAD_EXIT (0x12), SYS_THREAD_DISPATCH (0x13), plus mem/putc/getc. **Also catches non-ecall exceptions and kills the offending thread.**
- `src/trap_entry.S` — 144-byte frame: 16 caller-saved regs + sepc + sstatus. Sepc/sstatus stored per-thread (on the trapping thread's stack) so intermediate context switches never lose a thread's return PC.
- `src/syscall.cpp` — C API: `thread_create` (allocates stack via mem_alloc then ecall4), `thread_exit`, `thread_dispatch`, plus `putc`/`getc`.
- `src/syscall_cpp.cpp` + `h/syscall_cpp.hpp` — C++ API: `Thread`, `Semaphore` (stub for Task 3), `PeriodicThread` (stub for Task 4), `Console`. Private-static `run_trampoline` handles the `Thread()` default-ctor + virtual `run()` pattern (PDF p.11 says function-pointer wins if both are set).

## Test wiring — OS12026 public tests

Public test suite copied from `OS12026/tests/` into `tests/`:
- `Threads_C_API_test.{cpp,hpp}` — 4-thread cooperative interleave (Test 1)
- `Threads_CPP_API_test.{cpp,hpp}` — same via C++ API (Test 2)
- `System_Mode_test.{cpp,hpp}` — includes `csrr t6, sepc` to check U-mode (Test 7)
- `printing.{cpp,hpp}` — putc/getc-based print + int formatting with CAS spinlock
- `lock.S` — atomic compare_and_swap using `lr.w`/`sc.w`
- `uputstvo.txt` — test harness instructions

**Entry glue:** `src/userMain.cpp` implements the standard OS12026 harness — reads test number via getc, dispatches to selected test, guarded by `LEVEL_N_IMPLEMENTED` macros. Currently `LEVEL_2 = 1`, `LEVEL_3 = 0`, `LEVEL_4 = 0`.

**Main flow:** `main()` inits allocator + traps + threads, then spawns `userMain_wrapper` as a user thread and drains the ready queue via cooperative `thread_dispatch` until everyone finishes, then `khalt()`.

**Stashed** in `../task3_stash_temp` outside the project tree (so Makefile's `find` doesn't pick them up until they compile):
`ConsumerProducer_*_test.*` (Tests 3, 4, 6), `ThreadSleep_C_API_test.*` (Test 5), `buffer.*`, `buffer_CPP_API.*`. Restore when Task 3 is done.

## Known gap: Test 7 System_Mode — FIXED

Fixed by U-mode retrofit (see above). Fresh user TCBs get a synthetic trap-return frame with `sstatus.SPP=0`; first switch-in goes through `trap_return_tail` and `sret` into U-mode. When Test 7's worker B hits `csrr t6, sepc`, hardware raises illegal-instruction (scause=2), our trap handler kills the thread, the busy-wait loop in the test never sees `finishedB`, test never completes.
