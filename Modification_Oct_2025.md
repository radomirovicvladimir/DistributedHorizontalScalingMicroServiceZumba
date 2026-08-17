# Modification October 2025 — `addChild` + `joinAll` (explicit child registration) — 30p

Source: https://siwiki.rs/wiki/ОС1/Модификације_октобар_2025 (reference solution).
Adapted from the wiki's target kernel to this project's kernel (see "Adaptation").

## Task (original, Serbian)

Dodati sistemske pozive `thread_add_child` i `thread_join_all`.
- `thread_add_child` registruje dete-nit za tekuću (pozivajuću) nit.
- `thread_join_all` blokira tekuću nit dok se sva njena deca ne izvrše.

U okviru C++ API dodati **nestatičke** metode `addChild(Thread* child)` i `joinAll()`
u klasu `Thread`.

**Test:** nit A pravi 3×B i 1×C; svaki B pravi 3×C. Roditelj čeka svu svoju decu pre
nego što nastavi.

## Semantics (per wiki)

- `addChild(child)` registers `child` as a child of the **calling** thread
  (kernel uses the running thread as the parent — the `Thread*` receiver is irrelevant
  to which thread becomes the parent; the running thread does).
- `joinAll()` blocks the caller until **all its registered (direct) children** finish.
  It waits only for children registered via `addChild`, not the whole subtree
  automatically — but because each B itself calls `joinAll`, the tree drains bottom-up.

---

## Adaptation to this kernel

The wiki targets a kernel with `RISCV.cpp/handleSupervisorTrap`, `dispatch()` handling
thread-finish, and a per-parent `Semaphore` (`childrenSem`). This project differs:

| Wiki | This project |
|------|--------------|
| finish handled in `dispatch()` | finish handled in `TCB::exit()` |
| per-parent `Semaphore* childrenSem` (init 0), `sem_wait`/`sem_signal` | **direct TCB block/wake** — no semaphore allocation |
| syscalls 0x16 / 0x17 | 0x16/0x17 already used here → **0x1B / 0x1C** |
| `getMyHandle()` | `Thread::getHandle()` |

The behavior is identical; only the plumbing matches our kernel.

### TCB state (`h/TCB.hpp`)
```cpp
TCB* parent;      // parent thread (nullptr if none)
int  childCount;  // number of unfinished registered children
bool joining;     // true while blocked in joinAll
```

### `add_child(parent, child)` (`src/TCB.cpp`)
`child->parent = parent; parent->childCount++;` (no lazy semaphore — we block directly).

### `join_all_children()` (`src/TCB.cpp`)
`if (childCount <= 0) return 1;` (nothing to wait) else `joining = true; BLOCKED; return 0`.

### finish hook (`TCB::exit`)
```cpp
if (parent) {
    parent->childCount--;
    if (parent->childCount == 0 && parent->joining) {
        parent->joining = false;
        parent->state = READY;
        // set woken parent's syscall return (a0) to 0
        Scheduler::put(parent);
    }
}
```
This is the analogue of the wiki's `dispatch()` decrement + `sem_signal`.

### API layers
| Layer | Symbols |
|-------|---------|
| ABI | `SYS_THREAD_ADD_CHILD 0x1B`, `SYS_THREAD_JOIN_ALL 0x1C` |
| C | `void thread_add_child(thread_t)`, `void thread_join_all()` |
| Kernel | `TCB::add_child`, `TCB::join_all_children` (+ `exit` hook) |
| C++ | `void Thread::addChild(Thread*)`, `void Thread::joinAll()`, `thread_t Thread::getHandle()` |

---

## Files changed / added

| File | Change |
|------|--------|
| `h/TCB.hpp` | `parent`, `childCount`, `joining`; `add_child`, `join_all_children` |
| `src/TCB.cpp` | init fields (create/init); `exit` finish hook; `add_child`, `join_all_children` |
| `h/syscall_abi.hpp` | `SYS_THREAD_ADD_CHILD 0x1B`, `SYS_THREAD_JOIN_ALL 0x1C` |
| `h/syscall_c.h` | `thread_add_child`, `thread_join_all` |
| `src/syscall.cpp` | `thread_add_child` → `ecall1`, `thread_join_all` → `ecall0` |
| `src/trap.cpp` | `case SYS_THREAD_ADD_CHILD`, `case SYS_THREAD_JOIN_ALL` (block-and-switch) |
| `h/syscall_cpp.hpp` | `addChild`, `joinAll`, `getHandle()` |
| `src/syscall_cpp.cpp` | `Thread::addChild` → `thread_add_child(child->myHandle)`, `Thread::joinAll` → `thread_join_all` |
| `tests/JoinAllChildren_test.{cpp,hpp}` | **new** — A/B/C tree (test #12) |
| `tests/userMain.cpp` | menu `12`; selection read as a line (multi-digit); level-3 gate; `case 12:` |

> Note: `start()` must be called BEFORE `addChild()`, because `start()` sets `myHandle`
> (via `thread_create`); otherwise `getHandle()`/`addChild` would pass a null handle.
> The test follows this order.

---

## Test #12 (`tests/JoinAllChildren_test.cpp`)

Thread tree:
```
A (userMain)
 ├─ B0 ── C0, C1, C2
 ├─ B1 ── C10, C11, C12
 ├─ B2 ── C20, C21, C22
 └─ C99
```
A registers B0,B1,B2,C99 via `thread_add_child` and calls `thread_join_all()`. Each B
registers its 3 C's via `this->addChild(...)` and calls `this->joinAll()`. Every parent
blocks until all its direct children finish.

### Expected output (actual observed run)

Verified passing on the target. Note the true per-parent unblocking: **B0 unblocks the
moment its own 3 children finish** (it does not wait for B1's/B2's children), so the
`B<i> all children done!` lines interleave with later C threads.

```
A started, creating 3 B and 1 C
A waiting for all children...
  B0 started, creating 3 C children
  B0 waiting for children...
  B1 started, creating 3 C children
  B1 waiting for children...
  B2 started, creating 3 C children
  B2 waiting for children...
    C99 started
    C99 finished
    C0 started
    C0 finished
    C1 started
    C1 finished
    C2 started
    C2 finished
  B0 all children done!
    C10 started
    C10 finished
    C11 started
    C11 finished
    C12 started
    C12 finished
    C20 started
    C20 finished
  B1 all children done!
    C21 started
    C21 finished
    C22 started
    C22 finished
  B2 all children done!

=== A: ALL children done! ===
JoinAllChildren: PASS
TEST 12 (Modifikacija okt 2025, addChild + joinAll)
```

**Invariants (scheduling-independent):**
- Each `B<i> all children done!` appears only **after** all three of its C children have
  printed `finished` (B0 is freed as soon as C0/C1/C2 finish — it does not wait for the
  other B's children).
- `=== A: ALL children done! ===` appears only **after** all three B's report done AND
  C99 has finished — i.e. after every descendant in the tree.
- Every C prints exactly one `started` and one `finished`; there are 3×3 + 1 = **10** C
  threads and **3** B threads. The exact interleaving of the C lines and the B-unblock
  points varies with scheduling; the ordering constraints above always hold.

---

## Edge cases handled

1. **No children** → `join_all_children` returns immediately (1); caller does not block.
2. **Child finishes before parent joins** → its `exit` already decremented `childCount`;
   join sees only unfinished children.
3. **Nested joinAll** (B waits for C while A waits for B) → each TCB has its own
   `childCount`/`joining`; independent, drains bottom-up.
4. **start() before addChild()** → required so `myHandle` is valid (documented; test
   follows it).
5. **Woken parent return value** → parent's `a0` set to 0 via its saved `trap_frame`.

---

## Build / verify

Verified: builds and runs correctly on the target (QEMU), option 12 produces the
observed output above and ends with `=== A: ALL children done! ===` /
`JoinAllChildren: PASS`.

```
make
make qemu       # choose option 12
```

Menu option: `12  JoinAllChildren (Modifikacija: addChild + joinAll)`. The menu reads
the selection as a line so the two-digit `12` parses. Confirm each B reports done only
after its C's finish, and A finishes last with `=== A: ALL children done! ===`.
