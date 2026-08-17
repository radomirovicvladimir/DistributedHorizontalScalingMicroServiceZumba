# MODIFICATIONS_GUIDE.md — how to implement any OS1 "modifikacija" fast

Every modification we've done (pairSems, joinAll, send/receive, sync, addChild/joinAll,
getThreadId/quota) is a variation of **one pattern**: a thread blocks on some condition,
and another thread's action unblocks it. This guide captures the common scaffold plus
the reusable kernel primitive so a new modification is ~20 lines.

---

## 1. The 7-layer syscall scaffold (mechanical, same every time)

A user-callable kernel feature threads through these files. Copy this checklist:

| # | File | What to add |
|---|------|-------------|
| 1 | `h/syscall_abi.hpp` | `#define SYS_XXX 0x??` (pick an unused code) |
| 2 | `h/syscall_c.h` | `ret_t xxx(args);` |
| 3 | `src/syscall.cpp` | `extern "C" ret_t xxx(args){ return (ret_t)ecallN(SYS_XXX, args); }` |
| 4 | `src/trap.cpp` | `case SYS_XXX:` → call the kernel method (see block/wake below) |
| 5 | `h/TCB.hpp` + `src/TCB.cpp` | the kernel state + logic (the ONLY part that really differs) |
| 6 | `h/syscall_cpp.hpp` + `src/syscall_cpp.cpp` | `Thread::xxx()` / `Semaphore::xxx()` wrapper |
| 7 | `tests/Xxx_test.{cpp,hpp}` + `tests/userMain.cpp` | test + menu registration |

`ecallN` variants already exist in `src/syscall.cpp`: `ecall0/1/2/4`.

### Used ABI codes (pick outside these)
```
0x01/0x02 mem      0x11-0x15 thread(create/exit/dispatch/getid/setmax)
0x21-0x26 sem      0x31 sleep     0x41/0x42 getc/putc
```
Historically-used-by-modifications (avoid reusing within one build): 0x16-0x1C.

---

## 2. The reusable kernel primitive (in TCB)

Instead of hand-writing block/wake each time, use these (in `h/TCB.hpp` / `src/TCB.cpp`):

```cpp
// Block the calling thread. Returns TCB::WOULD_BLOCK (== 1).
static int  TCB::block_running();

// Wake a blocked thread, delivering `retval` as its syscall return value
// (written into its saved trap frame's a0).
static void TCB::wake(TCB* t, uint64 retval = 0);
```

And an intrusive FIFO of blocked threads (linked via `TCB::next`):

```cpp
class WaitQueue {
    bool empty() const;
    TCB* peek() const;
    void enqueue(TCB* t);
    TCB* dequeue();
};
```

`FRAME_A0_IDX` and `WOULD_BLOCK` are constants on `TCB`.

### The trap.cpp block-and-switch idiom (layer 4)
Any syscall whose kernel method may block uses exactly this shape:
```cpp
case SYS_XXX: {
    int r = TCB::xxx(...);          // returns WOULD_BLOCK if it blocked
    if (r == TCB::WOULD_BLOCK) {
        f->sepc += 4;               // step past the ecall
        Scheduler::switch_to_next();// yield; the waker will reschedule us
        return;                     // a0 is set later by TCB::wake
    }
    f->a0 = (uint64)r;              // immediate (non-blocking) return
    break;
}
```

Non-blocking syscalls just set `f->a0` and `break;` (the handler advances sepc).

---

## 3. What actually differs per modification (layer 5 only)

Only the **condition to block** and the **event that wakes** change:

| Modification | TCB state | block when | wake when |
|---|---|---|---|
| pairSems      | partner list           | can't pass self or any partner | signal (standard) |
| joinAll subtree | `desc_count`         | descendants > 0 | last descendant exits |
| addChild/joinAll | `parent`,`childCount` | children > 0 | last child exits |
| send/receive  | 1-slot mailbox + WaitQueue of senders | slot full / empty | partner receives / sends |
| sync (pair)   | `partner`,`waiting`    | partner not yet here | partner arrives |
| getThreadId/quota | `pending` WaitQueue | over `max_user_threads` | a thread exits |

The finish-driven ones (joinAll, addChild) also hook `TCB::exit()` to fire the wake.

---

## 4. Worked skeleton — a new block/wake modification in ~20 lines

Say the task is "`int foo()` blocks until another thread calls `bar()`":

```cpp
// --- h/TCB.hpp (state + decls) ---
static WaitQueue foo_waiters;      // (or per-object)
static int  foo();
static void bar();

// --- src/TCB.cpp (layer 5) ---
int TCB::foo() {
    if (/* condition already satisfied */) return 0;   // immediate
    foo_waiters.enqueue(TCB::running);
    return block_running();                            // WOULD_BLOCK
}
void TCB::bar() {
    TCB* t = foo_waiters.dequeue();
    if (t) TCB::wake(t, 0);                            // deliver return value
}

// --- src/trap.cpp (layer 4) ---
case SYS_FOO: {
    int r = TCB::foo();
    if (r == TCB::WOULD_BLOCK) { f->sepc += 4; Scheduler::switch_to_next(); return; }
    f->a0 = (uint64)r; break;
}
case SYS_BAR: TCB::bar(); f->a0 = 0; break;
```

Plus layers 1-3 (ABI/C decl/def) and 6-7 (C++ wrapper + test) from the checklist.

---

## 5. Non-block/wake modifications

Two of the seven don't fit the block/wake mold — recognize them so you don't over-engineer:

- **Matrix histogram (June 2025):** pure user-space — dynamic matrix + M worker threads +
  a mutex (`Semaphore(1)`) + join semaphore. No new kernel primitive; uses existing
  threads/semaphores only.
- **getThreadId + SetMaximumThreads (Aug 2023):** already implemented in the base kernel
  (quota + FIFO `PENDING_QUOTA`). Nothing to build.

For anything that is "compute in parallel then combine," the recipe is: **N threads +
one mutex semaphore for the shared result + one done-semaphore to join** — no kernel
changes.

---

## 6. Test + menu conventions (layer 7)

- Single-digit test: menu reads one char (`getc`). Widen the `sel >= '1' && sel <= 'N'`
  guard and add a `case N:`.
- Two-digit test (10, 11, 12): read the whole line instead —
  `getString(line,...)`, `stringToInt(line)`, gate on `line[0]`.
- Gate under the right `LEVEL_x_IMPLEMENTED`.
- Join worker threads before the test function returns (a `done` semaphore, waited
  once per worker) so `userMain` doesn't finish early.

---

## 7. Reference

`src/print.cpp` holds the exact per-file line-by-line changes for all seven completed
modifications (comment-only), each in its own separated section. Use it as the
copy-paste source when re-applying any modification. The per-modification design docs
are `Modification_*.md` / `Modicifation_2026_june.md`.
