# Modification September 2024 — thread pairing + `sync()` rendezvous — 20p

## Task (original)

Implementirati sve potrebne metode i sistemske pozive za sinhronizaciju niti
uparivanjem.

**C++ API:** `static void Thread::pair(Thread* t1, Thread* t2)` — uparuje dve niti.
Pretpostaviti da je svaka nit uparena sa tačno jednom niti.

**C++ API:** `void Thread::sync()` — sinhronizacija:
- Ako je jedna nit pozvala `sync`, a druga ne, tekuća nit se **blokira**.
- Kada druga (uparena) nit pozove `sync`, obe niti nastavljaju izvršavanje.
- Metoda se može pozvati **proizvoljan broj puta**.

Takođe: svaka korisnička nit ima **jedinstveni identifikator, počev od 1**.

**Test:** dodati **8. test** koji testira samo C++ API — dve niti se sinhronizuju u
`for` petlji sa 3 iteracije.

---

## Design

### Pair-based rendezvous (2-thread barrier)

`sync()` synchronizes a thread with the ONE thread it was paired with via
`Thread::pair`. It is a reusable 2-thread barrier.

### TCB state
```cpp
TCB* sync_partner;   // the paired thread (set by pair)
bool sync_waiting;   // true while this thread is blocked at the barrier
```

### `sync_pair(a, b)` — symmetric
`a->sync_partner = b; b->sync_partner = a;` (guards null / self).

### `sync_rendezvous()` — the barrier (`src/TCB.cpp`)
Called by the running thread `me` (partner `p`):
1. `p == nullptr` → return 1 (unpaired: no-op, defensive; spec guarantees pairing).
2. `p->sync_waiting` → partner already blocked here: clear its flag, **wake p**, and
   `me` continues (returns 1, does not block). Both proceed.
3. else → set `me->sync_waiting = true`, mark BLOCKED, return 0 (trap switches away);
   `me` is released by the partner's later `sync()`.

**Repeatable by construction:** the flag lives on the *waiter*; only one partner can be
waiting at a time (the second caller always finds the flag set and releases it). After
release both flags are false — clean for the next round, no counter to reset.

### Unique IDs from 1 (driver exemption)
`TCB::make_driver(t)` — called on the `userMain` thread right after it is created
(`main.cpp`). It sets the driver's id to 0, resets `next_id` to 0, marks it
`is_kernel`, and releases the `max_user_threads` quota slot the driver took in
`create`. The first real user thread then gets **id 1** (and it doesn't consume a
concurrency slot, so all paired threads run).

### Return-value plumbing
Blocking `sync` uses the standard block/switch path (`trap.cpp`: advance `sepc` +
`switch_to_next`). The woken thread's `a0` is set to 0 via its saved `trap_frame`
(same mechanism as semaphore/messaging wake).

### `Thread::sync()` is non-static but operates on the caller
The spec declares `sync()` non-static. Semantically it rendezvous the **calling**
thread with the caller's partner, not `this`. The test calls `sync()` from inside a
thread's own `run()`, where `this` == the running thread, so this is consistent.

### API layers
| Layer | Symbols |
|-------|---------|
| ABI | `SYS_THREAD_PAIR 0x19`, `SYS_THREAD_SYNC 0x1A` |
| C | `void thread_pair(thread_t, thread_t)`, `int thread_sync()` |
| Kernel | `TCB::sync_pair`, `TCB::sync_rendezvous`, `TCB::make_driver` |
| C++ | `static Thread::pair(Thread*,Thread*)`, `void Thread::sync()` |

---

## Files changed / added

| File | Change |
|------|--------|
| `h/TCB.hpp` | `sync_partner`, `sync_waiting`; `sync_pair`, `sync_rendezvous`, `make_driver` |
| `src/TCB.cpp` | init fields (create/init); `sync_wake` helper; `sync_pair`, `sync_rendezvous`, `make_driver` |
| `src/main.cpp` | `TCB::make_driver(userMainTCB)` after creating the driver |
| `h/syscall_abi.hpp` | `SYS_THREAD_PAIR 0x19`, `SYS_THREAD_SYNC 0x1A` |
| `h/syscall_c.h` | `thread_pair`, `thread_sync` |
| `src/syscall.cpp` | `thread_pair` → `ecall2`, `thread_sync` → `ecall0` |
| `src/trap.cpp` | `case SYS_THREAD_PAIR`, `case SYS_THREAD_SYNC` (block-and-switch) |
| `h/syscall_cpp.hpp` | `static void pair(Thread*,Thread*)`, `void sync()` |
| `src/syscall_cpp.cpp` | `Thread::pair` → `thread_pair(handles)`, `Thread::sync` → `thread_sync` |
| `tests/ThreadSync_test.{cpp,hpp}` | **new** — test #8, C++ API only |
| `tests/userMain.cpp` | menu `8`, digit branch widened to `'8'`, level-3 gate, `case 8:` |

---

## Test #8 (`tests/ThreadSync_test.cpp`)

Two `SyncThread`s are paired via `Thread::pair(t1, t2)`. Each runs a `for` loop of 3
iterations: print `ID: pre-sync iteracija i`, call `sync()`, print
`ID: post-sync iteracija i`. Main joins both via a `done` semaphore. With the driver
exemption, the two threads have IDs **1** and **2**.

### Expected output (illustrative interleaving)

```
--- ThreadSync (Modifikacija: pair + sync rendezvous) ---
1: pre-sync iteracija 0
2: pre-sync iteracija 0
2: post-sync iteracija 0
2: pre-sync iteracija 1
1: post-sync iteracija 0
1: pre-sync iteracija 1
1: post-sync iteracija 1
1: pre-sync iteracija 2
2: post-sync iteracija 1
2: pre-sync iteracija 2
2: post-sync iteracija 2
1: post-sync iteracija 2
Obe niti zavrsile sinhronizaciju.
ThreadSync: PASS
```

**Invariant (scheduling-independent):** for each iteration `i`, **both** threads print
their `pre-sync iteracija i` before **either** prints its `post-sync iteracija i` —
that is exactly what the barrier guarantees. Each thread prints 3 pre + 3 post lines,
and the program ends with the summary + PASS after both signal `done`. The exact line
ordering may vary with scheduling, but no thread ever gets ahead of its partner by
more than one barrier phase. Thread IDs are **1** and **2** (driver exemption).

---

## Edge cases handled

1. **Repeated sync** — flag-based, self-resetting; works for any number of calls.
2. **Unpaired thread calls sync** — `sync_partner == nullptr` → safe no-op (returns).
3. **Partner already waiting** — release path; the second caller does not block.
4. **IDs from 1** — `make_driver` exempts the userMain driver (id 0, quota released),
   so user threads are 1, 2, ...
5. **Wake return value** — woken thread's `a0` set to 0 via saved `trap_frame`.
6. **pair symmetry / self-pair** — `sync_pair` guards null and `a == b`.

---

## Build / verify

Verified: builds and runs correctly on the target (QEMU), option 8 produces the
expected output above (threads report IDs 1 and 2, each iteration's two pre-sync lines
both appear before either post-sync line) and prints `ThreadSync: PASS`.

```
make
make qemu       # choose option 8
```

Menu option: `8  ThreadSync (Modifikacija: pair + sync)`. Gated behind
`LEVEL_3_IMPLEMENTED == 1`. Confirm the two threads report IDs 1 and 2, that each
iteration's two pre-sync lines both appear before either post-sync line, and that it
ends with `ThreadSync: PASS`.
