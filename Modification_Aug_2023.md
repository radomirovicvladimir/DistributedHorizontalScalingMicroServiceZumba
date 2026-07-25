# Modification August 2023 — `getThreadId` + `SetMaximumThreads` + FIFO quota — 20p

## Task (original, Serbian)

Implementirati sistemski poziv `int getThreadId()` koji dohvata jedinstveni
identifikator tekuće niti. `getThreadId()` takođe treba da obavi **promenu konteksta**
(v. modifikacija jul 2022).

Proširiti klasu `Thread`:
- Statička metoda `SetMaximumThreads(int num_of_threads)` — poziva se na početku
  programa, postavlja ograničenje na maksimalan broj korisničkih niti.
  **Podrazumevana vrednost je 5** ako korisnik ništa ne unese.
- Nakon postavljanja maksimuma, moguće je kreirati dati broj niti; sve novokreirane
  niti preko limita se **blokiraju**. Kada jedna korisnička nit završi izvršavanje,
  ona **odblokira jednu** od prethodno blokiranih niti, **u redosledu kojim su
  blokirane** (FIFO).

**Test:** postaviti maksimum na 3, kreirati **20 niti**. Svaka nit ispisuje
`Hello! Thread::myId` ciklično **5 puta**, pa simulira spavanje (semafor ili busy-wait)
u **linearnoj korelaciji sa ID-om** date niti.

---

## Status: already implemented in the base project

This modification is **already satisfied by the existing kernel** (it corresponds to a
prior task in this codebase). No new code was written; this document records where each
required piece lives and how it meets the spec.

### 1. `getThreadId()` with context switch

- **C API** `getThreadId()` → `ecall0(SYS_THREAD_GET_ID)` (`src/syscall.cpp`).
- **C++ API** `Thread::getId()` → `getThreadId()` (`src/syscall_cpp.cpp`).
- **Kernel** (`src/trap.cpp`, `case SYS_THREAD_GET_ID`):
  ```cpp
  f->a0 = (uint64)TCB::running->id;   // fetch current thread's unique id
  f->sepc += 4;
  Scheduler::switch_to_next();        // <-- performs the context switch
  return;
  ```
  It returns the running thread's id **and** yields the CPU — exactly the "takođe treba
  da obavi promenu konteksta" requirement.

### 2. `SetMaximumThreads`, default 5

- `TCB::max_user_threads = 5` — the default (`src/TCB.cpp`).
- `Thread::SetMaximumThreads(n)` → `setMaximumThreads(n)` → kernel
  `TCB::set_maximum_threads(n)` (`src/TCB.cpp`), which clamps `n >= 1`, sets the limit,
  and admits any pending threads that now fit.

### 3. FIFO quota blocking / unblocking

- **On create** (`TCB::create`): if `active_user_threads < max_user_threads`, the thread
  is admitted (`Scheduler::put`); otherwise it is parked in state `PENDING_QUOTA` on a
  FIFO list (`pending_head`/`pending_tail`).
- **On exit** (`TCB::exit`): the finishing thread decrements `active_user_threads` and
  calls `admit_from_pending()`, which dequeues the **oldest** pending thread
  (`pending_head`) and admits it — i.e. unblocks one blocked thread **in the order they
  were blocked** (FIFO), exactly as required.

### 4. Unique thread identifier

- `TCB::next_id` is incremented per `TCB::create` (`t->id = ++next_id`), giving each
  thread a unique id. `getThreadId()` returns it.

---

## Test (existing `Modification_test`, menu option `m`)

`tests/Modification_test.cpp` already matches the Aug 2023 test spec:

- `Thread::SetMaximumThreads(3);`
- creates **20** `HelloThread`s;
- each thread: prints `"Hello! " <id>` **5 times** (`Thread::getId()` for the id);
- then busy-waits an amount **proportional to `(id + 1)`** — a linear correlation with
  the thread's ID (the spec's "spavanje ... u linearnoj korelaciji sa ID-om");
- main joins all 20 via a `done` semaphore before finishing.

> The only cosmetic difference from the literal wording is the print format: the test
> prints `Hello! ` followed by the numeric id (same content as `Hello! Thread::myId`).

### Expected output (structure / invariants)

Because scheduling is cooperative and deterministic, the run is reproducible, but the
exact interleaving of the 20 threads' lines is intricate. The guarantees to check:

```
--- Modification (getThreadId + SetMaximumThreads) ---
SetMaximumThreads(3), spawning 20 threads.
Hello! <id>            (x5 per thread, 20 threads = 100 Hello lines total, interleaved)
...
Modification: PASS (all 20 threads finished under quota of 3)
```

- **100 `Hello!` lines total** — each of the 20 threads prints exactly 5.
- **At most 3 user threads run concurrently** at any moment (the quota); the other 17
  start blocked in `PENDING_QUOTA` and are admitted one-by-one as running threads exit,
  in **FIFO order**.
- **All 20 threads eventually finish** despite the quota of 3 — this is the core proof
  that quota blocking + FIFO unblock works. The final line confirms it:
  `Modification: PASS (all 20 threads finished under quota of 3)`.
- Each thread's own 5 `Hello!` lines carry its unique id (from `getThreadId()`), and the
  per-thread busy-wait scales with that id.

---

## Files

No files were changed for this modification — it is already implemented. The relevant
existing locations:

| Piece | Location |
|-------|----------|
| `getThreadId` syscall + context switch | `src/trap.cpp` (`case SYS_THREAD_GET_ID`), `src/syscall.cpp` |
| `SetMaximumThreads` (default 5) | `src/TCB.cpp` (`max_user_threads=5`, `set_maximum_threads`), `src/syscall_cpp.cpp` |
| FIFO quota block/unblock | `src/TCB.cpp` (`create`, `exit`, `admit_from_pending`, `pending_head/tail`) |
| unique id | `src/TCB.cpp` (`next_id`, `t->id = ++next_id`) |
| test | `tests/Modification_test.cpp` (menu option `m`) |

## Verify

```
make
make qemu       # choose option 'm'
```

Expected: 20 threads each print 5 `Hello!` lines and the run ends with
`Modification: PASS (all 20 threads finished under quota of 3)`.
