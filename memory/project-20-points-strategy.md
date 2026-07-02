---
name: project-20-points-strategy
description: Task selection — Task 1+2+3 for 20 points, skip Task 4, use console.lib as the fallback for getc/putc
metadata:
  type: project
---

Vlado's plan: hit exactly the 20-point threshold to pass. Passing requires "≥20 pts public tests" and "≥15 pts at defense".

| Task | Doing? | Points | Notes |
|---|---|---|---|
| 1 — Memory alloc | ✅ done | 5 | See [[project-task1-done]] |
| 2 — Threads (create/exit/dispatch) + sync context switch | 🔨 next | 10 | This is the current work — [[project-task2-plan]] |
| 3 — Semaphores | plan | 5 | After Task 2 |
| 4 — Async preemption / time_sleep / getc/putc / PeriodicThread | ❌ **skip** | 10 | Use console.lib instead |
| 5 — Predrok bonus | n/a | 10 | |

**Task 4 replacement:** `lib/console.lib` provides `__putc`, `__getc`, and `console_handler`. Per PDF §"Način ocenjivanja": if we skip Task 4 we still must implement all three interface layers for `putc`/`getc`; the kernel body is the pre-built `__putc`/`__getc` from console.lib. See [[reference-console-lib]] for the wiring.

**What we still must implement even while skipping Task 4:**
- C API `putc`/`getc` shims — trivial calls through ecall to the kernel-side implementations, which delegate to `__putc`/`__getc`.
- C++ API `Console::getc`/`Console::putc`.
- The trap handler must call `console_handler()` on external interrupt (`scause == (1<<63)|9`) so console.lib's own ISR path runs.
- **No** timer handling, **no** `time_sleep`, **no** `PeriodicThread` body — those cost the 10 points but aren't blocking.

**Why:** the 10 Task 4 points are the priciest to implement (async register save of all 31 GPRs, timer subsystem, sleep list, producer/consumer buffers, periodic thread scheduling). Trading them for 0 pts and shipping a clean 20 is the right cost/benefit.
