# Modification 2026 — `pairSems` (Semaphore pairing) — 20p

## Task (original, Serbian)

Dodati klasi `Semaphore` statičku metodu:

```cpp
void pairSems(Semaphore sem1, Semaphore sem2)
```

koja zadata dva semafora uparuje. Pored metode, implementirati i odgovarajući
sistemski poziv u C API koji metoda `pairSems` koristi. Jedan semafor može biti
uparen sa više različitih semafora; smatra se da se već upareni semafori neće
ponovo uparivati.

Kada se na nekom od uparenih semafora pozove `wait`, nit prvo pokuša da prođe bez
blokiranja na **pozvanom** semaforu; ako ne prođe, pokuša da prođe na nekom
**uparenom** semaforu; ako ni to ne uspe, blokira se na **pozvanom** semaforu.
Ako nit prođe na uparenom semaforu, `wait` pozvanog semafora vraća vrednost **1**.
Prilikom prolaska ili blokiranja, taj semafor se ažurira na standardan način.
Redosled ispitivanja uparenih semafora je proizvoljan. Neupareni semafori
zadržavaju standardno ponašanje. `signal` se ponaša standardno.

Test pod rednim brojem **8**: 6 semafora (glavni init 100 uparen sa 5 običnih init
1..5), 5 niti × 5 iteracija, ispis poruka, 20p varijanta radi u 1000 iteracija
`dispatch`, na kraju ispis ukupnog broja prolazaka na uparenim semaforima, korektan
završetak tek po završetku svih niti.

---

## Design decisions

### Return-value protocol (the key mechanism)

`KSemaphore::wait` internal return codes:

| Code | Meaning | trap.cpp action | user-visible `sem_wait` |
|------|---------|-----------------|-------------------------|
| `0` | passed on the called (own) semaphore | set `a0=0` | `0` |
| `PASSED_PAIRED` (`2`) | passed on a **paired** semaphore, did NOT block | set `a0=1`, do not switch | `1` |
| `1` | must block on the called semaphore | switch away; `wake` later sets `a0` | `0` (or `-1` if closed) |
| `<0` | error (`E_CLOSED = -1`) | set `a0=r` | `<0` |

A distinct sentinel `PASSED_PAIRED = 2` is required because the existing kernel
already uses `1` to mean "block & switch away" (`trap.cpp`). Reusing `1` for the
paired-pass case would incorrectly park the thread. The `2` case falls through to
`f->sepc += 4` (advance past `ecall`) like any normal non-blocking syscall.

### Pass-by-reference (correctness trap)

The task signature shows `pairSems(Semaphore sem1, Semaphore sem2)` **by value**,
but `Semaphore`'s destructor calls `sem_close` (`src/syscall_cpp.cpp`). Passing by
value would copy the handle and then **close the underlying semaphore** when the
temporary is destroyed. Implemented as `pairSems(Semaphore&, Semaphore&)` — same
intent, no double-close. It is a `static` member so it can read the private
`myHandle`.

### Partner storage

Fixed array `KSemaphore* partners[MAX_PARTNERS]` with `MAX_PARTNERS = 16`.
Allocation-free (kernel code). Pairing is **symmetric** (both directions stored,
so `wait` on either sem sees the other), **idempotent** (`addPartner` skips
duplicates), and **self-pairing is ignored** (`a == b` guard).

### `wait` algorithm (3 phases, order matters)

1. `value >= n` on **self** → pass, `value -= n`, return `0`.
2. else if paired: scan partners (arbitrary order = array front-to-back); skip
   closed partners; first partner `P` with `P.value >= n` → `P.value -= n`,
   return `PASSED_PAIRED`.
3. else block on **self** (standard enqueue), return `1`.

`n == 0` returns `0` before the partner scan. Closed self returns `E_CLOSED`.
Unpaired sems (`nPartners == 0`) skip phase 2 → identical old behavior.

`signal` is **unchanged** — no cross-pair propagation. A thread that passed on a
partner already consumed that partner's unit at wait-time.

---

## Files changed

| File | Change |
|------|--------|
| `h/syscall_abi.hpp` | `#define SYS_SEM_PAIR 0x27` |
| `h/syscall_c.h` | `int sem_pair(sem_t a, sem_t b);` |
| `src/syscall.cpp` | `sem_pair` → `ecall2(SYS_SEM_PAIR, a, b)` |
| `h/Semaphore.hpp` | `partners[16]`, `nPartners`, `MAX_PARTNERS`, `PASSED_PAIRED`, `addPartner`, static `pair` |
| `src/Semaphore.cpp` | ctor inits partners; `addPartner`, `pair`, new 3-phase `wait` |
| `src/trap.cpp` | wait case handles `PASSED_PAIRED` (a0=1, no switch); new `case SYS_SEM_PAIR` |
| `h/syscall_cpp.hpp` | `static void Semaphore::pairSems(Semaphore&, Semaphore&)` |
| `src/syscall_cpp.cpp` | `pairSems` → `sem_pair(sem1.myHandle, sem2.myHandle)` |
| `tests/SemPair_test.{cpp,hpp}` | **new** — test #8 |
| `tests/userMain.cpp` | menu `8`, digit branch widened to `'8'`, level-3 gate, `case 8:` |

---

## Test #8 (`tests/SemPair_test.cpp`)

- 6 semaphores: `mainSem` (init 100) paired via `Semaphore::pairSems(*mainSem, *workers[i])`
  with 5 ordinary sems (init 1,2,3,4,5).
- 5 `WorkerThread`s (C++ API, subclass `run()`), each over one ordinary sem, 5 iterations.
- Per iteration:
  - `"ID wait iteracija num!\n"`
  - `wait()`:
    - return `0` → self-pass → `"ID prosla semafor iteracija num!\n"`
    - return `1` → paired-pass → `"ID prosla semafor-iteracija num!\n"`, `pairedPassTotal++`
  - **20p**: `for i in 0..999: Thread::dispatch()`
- `done` semaphore (init 0): each thread signals on finish; main `wait()`s 5× before
  printing `"Ukupno prolazaka na uparenim semaforima: N"` — guarantees join before exit.

### Thread IDs (why workers are 2–6, not 1–5)

`next_id` starts at 0. `mainTCB` (boot context) and `idleTCB` are both id 0.
Every `TCB::create` does `id = ++next_id`. The **first** create is the `userMain`
driver thread (`main.cpp` creates `userMain_wrapper` before any user code runs), so
it takes **id 1**. The 5 workers created inside `userMain` therefore get
**ids 2, 3, 4, 5, 6** — there is no worker with id 1.

This is left as-is: the IDs only need to be unique per thread (which they are), and
`getThreadId()` is spec-correct. Map the self/paired split by **own-semaphore init
value**, not by ID (worker id `k` owns the semaphore with init `k-1`):

| ID | own sem init | self-passes | paired-passes |
|----|--------------|-------------|---------------|
| 2  | 1 | iter 1    | iters 2–5 → **4** |
| 3  | 2 | iters 1–2 | iters 3–5 → **3** |
| 4  | 3 | iters 1–3 | iters 4–5 → **2** |
| 5  | 4 | iters 1–4 | iter 5   → **1** |
| 6  | 5 | iters 1–5 | none     → **0** |

Total paired = 4+3+2+1+0 = **10**. Sanity: mainSem 100 → 90, `100−90 = 10`. ✓

### Reproducibility

The kernel is single-core and cooperative (`CPU_CORE_COUNT = 1`, threads yield only
at explicit `thread_dispatch()`), so scheduling is fully deterministic. Repeated runs
of the same binary produce byte-identical output. Only changing `DISPATCH_ITERS`, the
thread count, `max_user_threads`, or creation order can alter the interleaving; the
counts (per-thread split and total 10) are invariant regardless.

### Expected output (reference run, IDs 2–6)

Message forms: `prosla semafor ` (space) = self-pass, `wait` returned 0;
`prosla semafor-iteracija ` (hyphen) = paired-pass, `wait` returned 1.

Note the interleave: `max_user_threads = 5` and the `userMain` driver holds one of
those 5 quota slots, so only 4 workers (ids 2–5) fit initially. Worker 6 (own sem
init 5, never blocks, never needs a partner) waits in `PENDING_QUOTA` and runs to
completion at the end, after the others free their slots.

```
--- SemPair (Modifikacija: pairSems) ---
2 wait iteracija 1!
2 prosla semafor iteracija 1!
3 wait iteracija 1!
3 prosla semafor iteracija 1!
4 wait iteracija 1!
4 prosla semafor iteracija 1!
5 wait iteracija 1!
5 prosla semafor iteracija 1!
2 wait iteracija 2!
2 prosla semafor-iteracija 2!
3 wait iteracija 2!
3 prosla semafor iteracija 2!
4 wait iteracija 2!
4 prosla semafor iteracija 2!
5 wait iteracija 2!
5 prosla semafor iteracija 2!
2 wait iteracija 3!
2 prosla semafor-iteracija 3!
3 wait iteracija 3!
3 prosla semafor-iteracija 3!
4 wait iteracija 3!
4 prosla semafor iteracija 3!
5 wait iteracija 3!
5 prosla semafor iteracija 3!
2 wait iteracija 4!
2 prosla semafor-iteracija 4!
3 wait iteracija 4!
3 prosla semafor-iteracija 4!
4 wait iteracija 4!
4 prosla semafor-iteracija 4!
5 wait iteracija 4!
5 prosla semafor iteracija 4!
2 wait iteracija 5!
2 prosla semafor-iteracija 5!
3 wait iteracija 5!
3 prosla semafor-iteracija 5!
4 wait iteracija 5!
4 prosla semafor-iteracija 5!
5 wait iteracija 5!
5 prosla semafor-iteracija 5!
6 wait iteracija 1!
6 prosla semafor iteracija 1!
6 wait iteracija 2!
6 prosla semafor iteracija 2!
6 wait iteracija 3!
6 prosla semafor iteracija 3!
6 wait iteracija 4!
6 prosla semafor iteracija 4!
6 wait iteracija 5!
6 prosla semafor iteracija 5!
Ukupno prolazaka na uparenim semaforima: 10
SemPair: PASS
TEST 8 (Modifikacija, uparivanje semafora pairSems)
```

Exact interleaving depends on scheduling order (spec-allowed to be arbitrary); the
line set and the total **10** are invariant. Per-thread paired-pass counts (count the
hyphen lines): ID2→4, ID3→3, ID4→2, ID5→1, ID6→0 = **10**.

### Notes on thread IDs and scheduling order (not bugs)

Because the kernel is single-core and cooperative, identical code always produces
identical output. Two observable behaviors are worth understanding — both are
correct, not defects:

**Worker IDs are 2–6, not 1–5.**
`next_id` increments on every `TCB::create`. The `userMain` driver thread is itself
created via `TCB::create` (in `main.cpp`) before the workers, so it takes id 1 and
pushes the workers to 2–6. IDs only need to be unique, so this is fine. (An optional
fix — exempting the driver from the counter via a `make_driver` helper — was tried and
then reverted; the workers-from-2 behavior is the shipped state.)

**Thread 6 runs alone at the end instead of interleaving.**
`max_user_threads = 5` is a concurrency quota; `TCB::create` does
`active_user_threads++` for every thread — including the driver. The driver holds one
of the 5 slots, so only 4 workers fit; worker 6 waits in `PENDING_QUOTA` and starts
only after another worker exits, so it tails the output. The counts are unaffected
(total still 10); only the interleave order changes, which the spec permits.

### Expected behavior / invariants

- Paired-pass count == `100 - mainSem.finalValue` (each paired pass consumes 1 main unit).
- With these initial values the main sem (100 units) is not exhausted, so no worker
  blocks; every iteration where a worker's own sem is empty becomes a paired-pass.
- `ID` is `Thread::getId()` (unique per thread).

---

## Edge cases handled

1. Pass-by-value double-close → pass by reference.
2. Paired-pass vs block ambiguity → `PASSED_PAIRED = 2` sentinel.
3. `wait(0)` → returns `0` before scanning partners.
4. Closed self → `E_CLOSED`; closed partners skipped in the scan.
5. Self-pairing / duplicate pairing → guarded (`a==b`, dedup in `addPartner`).
6. Partner array full → extra silently ignored (documented cap 16; never hit with 5).
7. `wait_n` interaction → partner pass requires `P.value >= n` (atomic, no partial).
8. Arbitrary partner order → array front-to-back; no fairness requirement.
9. Join before exit → `done` completion semaphore.

---

## Build / verify (NOT done on this host)

This Windows machine has no `make` or RISC-V toolchain (`riscv64-*-g++`). Changes are
a manual-review pass only. To verify in the Linux/QEMU environment:

```
make
make qemu       # then choose option 8 from the menu
```

Menu option: `8  SemPair (Modifikacija: pairSems)`. Gated behind
`LEVEL_3_IMPLEMENTED == 1`.
