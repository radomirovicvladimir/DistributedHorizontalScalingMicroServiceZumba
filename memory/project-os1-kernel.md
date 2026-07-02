---
name: project-os1-kernel
description: OS1 RISC-V multithreaded kernel project — tiny embedded-style kernel booting under QEMU with hw.lib as its host
metadata:
  type: project
---

Small multithreaded kernel written in C++/RISC-V asm, running on `qemu-system-riscv64 -machine virt`. The kernel and user app are one statically-linked binary at 0x80000000. The host runtime `hw.lib` (stripped-down xv6) handles boot, UART/PLIC/timer init, then calls into a `main()` symbol we provide.

**Boot chain:** `_entry` (hw.lib) → `start` (hw.lib, M→S mode) → `system_main` (hw.lib, inits hardware) → **our `main()`** → creates `userMain` thread.

**Interface layering (each syscall goes through all three layers, per PDF):**
```
User app (userMain) ─► C++ OO API (Thread, Semaphore, Console, PeriodicThread)
                    ─► C  API      (thread_create, mem_alloc, sem_wait, …)
                    ─► ABI         (ecall + register-packed args)
                    ─►────────── user/kernel line ──────────
                    ─► Kernel      (MemoryAllocator, Scheduler, TCB, SCB)
                    ─► HW access   (hw.lib — provided)
```

**Key decisions (see [[project-task2-plan]] for details):**
- Non-preemptive kernel: whole kernel is one critical section, interrupts stay masked (hardware clears `sstatus.SIE` on trap entry).
- Per-thread kernel stack: kernel code runs as nested calls in the current thread's context.
- Cooperative `yield(old*, new*)` for context switch — saves 14 callee-saved regs (ra, sp, s0-s11).
- FIFO scheduler with an always-runnable idle thread as fallback.
- ~~Preemption/timer/PeriodicThread~~ — skipped, see [[project-20-points-strategy]].

**Grading:** [[reference-grading]] (20 points target).
**ABI numbers:** [[reference-abi-syscalls]].
**hw.lib symbols we depend on:** [[reference-hwlib-symbols]].
**Where files live:** [[project-dir-layout]].
