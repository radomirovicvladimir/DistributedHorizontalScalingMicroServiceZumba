---
name: reference-grading
description: OS1 grading — points per task, pass conditions, submission rules
metadata:
  type: reference
---

Per PDF §"Način ocenjivanja":

| # | Task | Points | Prereq |
|---|---|---|---|
| 1 | Memory alloc — `mem_alloc`, `mem_free` | 5 | — |
| 2 | Threads — `thread_create`, `thread_exit`, `thread_dispatch`; sync context switch only | 10 | — |
| 3 | Semaphores — `sem_open/close/wait/signal` | 5 | Task 2 |
| 4 | Async ctx switch + time-sharing + `time_sleep` + `getc`/`putc` + `PeriodicThread` | 10 | Task 2 |
| 5 | Predrok bonus | 10 | project defended in early exam period |

**Every task must implement all three layers** (ABI + C API + C++ API). Skipping Task 1 → link `mem.lib`. Skipping Task 4 → link `console.lib` and call `console_handler()` from the trap vector on external IRQ.

**Pass conditions:**
- ≥ **20 pts** from public tests (**failing ANY public test = whole task scored 0**).
- ≥ **15 pts** at defense (private tests + on-the-spot modifications + viva).

**Public vs private:**
- Public tests visible to students in advance.
- Private tests during defense; also performance tests (context-switch overhead, semaphore overhead, IRQ latency, max concurrent threads, etc.).

**Submission:** ZIP with `src/` + `inc/` only. No libs, no binaries, no tests, no git repo. Upload before deadline at rti.etf.bg.ac.rs/domaci.
