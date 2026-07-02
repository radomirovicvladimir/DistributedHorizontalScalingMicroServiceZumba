---
name: project-task3-plan
description: Task 3 (Semaphores) — KSemaphore kernel class, FIFO blocked queue, wait_n/signal_n, wired through all 3 layers
metadata:
  type: project
---

**Task 3 = 5 points, IMPLEMENTED.** Deliverables: `sem_open`, `sem_close`, `sem_wait`, `sem_signal`, `sem_wait_n`, `sem_signal_n` at all three interface layers.

## Design

**Kernel-side class name: `KSemaphore`.** The user-visible `Semaphore` in [[project-task2-plan]]'s `syscall_cpp.hpp` clashes with the kernel one, so kernel uses `KSemaphore` (matches OS12026's convention). `sem_t` typedef stays as `void*` (opaque handle) — kernel casts to `KSemaphore*`.

**Value convention: strictly non-negative.**
- `wait(n)`: if `value >= n`, decrement and return. Else block (do NOT touch value).
- `signal(n)`: add n to value; then loop `while blocked_head && value >= head.wait_n`: decrement value by head.wait_n and wake head.
- `close()`: wake all remaining waiters with return -1.

This differs from OS12026's signed-value convention (which subtly mis-handles `wait_n > 1`). Ours handles heterogeneous wait_n correctly.

## Files

- `h/Semaphore.hpp` — KSemaphore class + inline placement-new declaration.
- `src/Semaphore.cpp` — wait/signal/close + FIFO blocked queue (head+tail).
- `src/trap.cpp` — added cases 0x21-0x26. Blocking wait uses `Scheduler::switch_to_next()` after setting `TCB::state = BLOCKED`.
- `src/syscall.cpp` — added `ecall2` helper and 6 C API wrappers.
- `src/syscall_cpp.cpp` — replaced Semaphore stubs with real impls that call sem_open/close/wait/signal.
- `h/TCB.hpp` — added `BLOCKED` state, `trap_frame` (void*), `wait_n`, `sem_result` fields.
- `src/TCB.cpp` — zero-init new fields in create/init.
- `src/Scheduler.cpp` — `switch_to_next` handles BLOCKED state (doesn't re-queue).

## Wake mechanism

When a waiter blocks in `sem_wait`, the trap dispatcher stashes the trap frame pointer on `TCB::trap_frame` (via `c_trap_handler`'s prologue) BEFORE calling into KSemaphore. On wake, KSemaphore::signal writes the return value (0 or -1) directly into `frame->a0` (index 8 in the frame layout). When the woken thread is scheduled and eventually returns through trap_entry's epilogue, sret restores a0 to that value — user code sees `sem_wait` returning 0 (or -1 for close).

## Tests

Enabled `LEVEL_3_IMPLEMENTED = 1` in `src/userMain.cpp`. Restored from stash:
- `tests/ConsumerProducer_C_API_test.{cpp,hpp}` (Test 3)
- `tests/ConsumerProducer_CPP_Sync_API_test.{cpp,hpp}` (Test 4)
- `tests/buffer.{cpp,hpp}` (C API bounded buffer)
- `tests/buffer_CPP_API.{cpp,hpp}` (C++ API bounded buffer)

Still stashed in `../task3_stash_temp/`:
- Task 4 tests: `ConsumerProducer_CPP_API_test.*`, `ThreadSleep_C_API_test.*`

## Placement-new plumbing

Kernel needs `new (raw) KSemaphore(...)` in trap.cpp to construct into allocator memory (see [[feedback-kernel-no-new]]). Added `inline void* operator new(size_t, void*) noexcept { return p; }` to `h/Semaphore.hpp` — pulled in wherever the syntax is used. `cpp_runtime.cpp` no longer needs a global definition.
