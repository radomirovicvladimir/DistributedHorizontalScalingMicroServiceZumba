---
name: reference-os12026-tests
description: OS12026 public tests we've integrated — what each one exercises and which APIs it requires
metadata:
  type: reference
---

The grader ships a canonical test suite with the OS1 assignment (`tests/uputstvo.txt` confirms this). We copied the version from `C:\Users\I770069\Desktop\OS12026\tests\` into our `tests/` folder. The harness is user-side: `src/userMain.cpp` reads a test number from stdin and dispatches to one of the tests.

## Live tests (compiled + linked)

| # | File | Task | Requires |
|---|---|---|---|
| 1 | `Threads_C_API_test.cpp` | 2 | `thread_create`, `thread_dispatch`, `thread_exit`, `putc` (via printString) |
| 2 | `Threads_CPP_API_test.cpp` | 2 | Same + `Thread` subclass with `run()` override, `operator new/delete`, `Thread::start()`, `Thread::dispatch()` |
| 7 | `System_Mode_test.cpp` | 2 | Same as Test 1 + `csrr sepc` (privileged) — expected to NOT complete regularly |

## Stashed until Task 3 (in `../task3_stash_temp`, outside project tree)

| # | File | Task | Requires |
|---|---|---|---|
| 3 | `ConsumerProducer_C_API_test.cpp` | 3 | `sem_open`, `sem_close`, `sem_wait`, `sem_signal`, `getc`, `putc`, `mem_alloc/free` |
| 4 | `ConsumerProducer_CPP_Sync_API_test.cpp` | 3 | + `Semaphore` C++ class, `Console::getc/putc` |
| 5 | `ThreadSleep_C_API_test.cpp` | 4 | `time_sleep` — **won't build without Task 4** |
| 6 | `ConsumerProducer_CPP_API_test.cpp` | 4 | `Thread::sleep`, async preemption — **won't build without Task 4** |

Also stashed: `buffer.{cpp,hpp}` and `buffer_CPP_API.{cpp,hpp}` (bounded-buffer helpers used by tests 3/4/6).

## Test 7 caveat

Per `tests/uputstvo.txt`: *"Očekivano ponašanje za test 7 jeste da ne dolazi do regularnog završetka procesa"* — process should NOT terminate normally. The test runs `csrr t6, sepc` at iteration `i==10` inside workerBodyB. In a proper U-mode design that faults with scause=2 (illegal instruction). In our all-S-mode design it succeeds silently. See [[project-task2-plan]] for the workaround (catches all non-ecall exceptions but not this specific case).

## Harness rules (from uputstvo.txt)

1. Set `LEVEL_N_IMPLEMENTED` macros in `userMain.cpp` to 1 for finished tasks.
2. `userMain()` must be called from `main()` after kernel init.
3. Tests dispatch by user typing a digit on the console.
4. Copy test files into project, adjust `#include` paths — done.

## Shared test infrastructure

- `printing.cpp` — `printString`, `printInt`, `getString`, `stringToInt`. Uses a spinlock over `putc`/`getc` so concurrent-thread prints don't interleave mid-string.
- `lock.S` — `copy_and_swap(uint64* lock, uint64 expected, uint64 desired)` using `lr.w`/`sc.w` (RISC-V A extension). Returns 0 on success. Our `-march=rv64ima` includes A.
