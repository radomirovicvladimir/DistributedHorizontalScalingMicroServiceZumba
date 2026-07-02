# Memory index

- [User: Vlado](user-vlado.md) — student implementing OS1 RISC-V kernel, targets 20/30 points
- [Project: OS1 RISC-V kernel](project-os1-kernel.md) — tiny multithreaded kernel on emulated RISC-V under QEMU
- [Project: 20-point goal, skip Task 4](project-20-points-strategy.md) — implements Task 1/2/3, uses console.lib for I/O
- [Project: Task 1 done](project-task1-done.md) — MemoryAllocator + syscalls + all three layers complete and tested
- [Project: Directory layout](project-dir-layout.md) — where sources, headers, libs, and build outputs live
- [Reference: Grading breakdown](reference-grading.md) — points per task per PDF §"Način ocenjivanja"
- [Reference: ABI syscall numbers](reference-abi-syscalls.md) — canonical codes from PDF §"Interfejs jezgra"
- [Reference: hw.lib provided symbols](reference-hwlib-symbols.md) — what boot/console/heap constants we can rely on
- [Reference: console.lib API](reference-console-lib.md) — __putc/__getc/console_handler and how to wire it
- [Feedback: Two-entry allocator API](feedback-alloc-two-entry.md) — kernel needs direct alloc back door, not just ecall
- [Feedback: 16-byte alignment via header](feedback-16b-align.md) — Node sizeof==16 keeps payload SP-aligned
- [Feedback: Kernel must never use `new`](feedback-kernel-no-new.md) — kernel `new` would re-enter ecall path
- [Project: Task 2 plan — cooperative threads](project-task2-plan.md) — non-preemptive kernel, per-thread kernel stack, yield()
- [Reference: OS12026 public tests](reference-os12026-tests.md) — the canonical test suite integrated into our tests/
