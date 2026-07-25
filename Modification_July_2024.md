# Modification July 2024 — thread message passing (`send` / `receive`) — 20p

## Task (original)

**C API**
```c
void  send(thread_t handle, char* message);
char* receive();
```
**C++ API**
```cpp
void  Thread::send(char* message);
char* Thread::receive();
```

`send` šalje poruku `message` niti iza ručke `handle`. Ako je neka poruka već
poslata a nit je nije primila sa `receive`, nit koja zove `send` se **blokira**. Ako
nit pozove `receive` a nema primljenih poruka, **blokira se** dok joj neka druga nit
ne pošalje poruku. Test: napraviti niti A, B i C koje međusobno šalju i primaju
poruke i to ispisuju.

## Locked-in semantics

- **Single-slot mailbox** per thread (holds at most 1 unreceived message).
- `receive()` reads the **calling** thread's own mailbox.
- Messages are passed **by pointer, as-is** (shallow — the caller keeps the buffer
  alive; the test uses `static` buffers).
- `Thread::send(msg)` sends to the **object's** thread (`this->myHandle`).
- `Thread::receive()` is a member but reads the **caller's** mailbox (via the C
  `receive()`), matching the task's literal signature.

---

## Design

### TCB mailbox state (`h/TCB.hpp`)
```cpp
char* mbox_msg;          // pending message (valid when mbox_full)
bool  mbox_full;         // slot holds an unreceived message
bool  recv_blocked;      // owner is blocked in receive()
char* recv_result;       // message handed to a woken receiver
char* pending_send_msg;  // message a blocked sender wants to deposit
TCB*  send_wait_head;    // FIFO queue of senders blocked on this mailbox
TCB*  send_wait_tail;
```

### `msg_send(target, msg)` (`src/TCB.cpp`)
1. `target->recv_blocked` → **direct handoff**: set `recv_result`, clear the flag,
   wake the receiver (its `a0` = msg). Sender does not block. Return 0.
2. else if slot empty → **deposit** (`mbox_msg=msg; mbox_full=true`). Return 0.
3. else (slot full) → **block sender**: store `pending_send_msg`, enqueue on
   `target->send_wait`, mark BLOCKED. Return 1 (trap switches away).

### `msg_receive(out)` (`src/TCB.cpp`), caller = running
1. If slot full → take msg, clear slot; if a sender is queued, dequeue it, refill the
   slot from its `pending_send_msg`, and wake it (send returns). `*out = msg`, return 0.
2. else (empty) → **block receiver**: `recv_blocked = true`, BLOCKED. Return 1.

### Return-value plumbing (`src/trap.cpp`)
- `SYS_SEND`: `msg_send`; if it returns 1, advance `sepc` + `switch_to_next` (sender
  blocked); else `a0 = 0`.
- `SYS_RECEIVE`: `msg_receive`; if 1, advance + switch (receiver blocked, `a0` set at
  wake time via the saved `trap_frame[a0]`, like `KSemaphore::wake`); else `a0 = msg`.

### API layers
| Layer | Symbols |
|-------|---------|
| ABI | `SYS_SEND 0x17`, `SYS_RECEIVE 0x18` |
| C | `void send(thread_t, char*)` → `ecall2`; `char* receive()` → `ecall0` |
| C++ | `void Thread::send(char*)` → `::send(myHandle,msg)`; `char* Thread::receive()` → `::receive()` |

---

## Files changed / added

| File | Change |
|------|--------|
| `h/TCB.hpp` | mailbox fields; `static int msg_send(TCB*,char*)`, `static int msg_receive(char**)` |
| `src/TCB.cpp` | init fields in `create`/`init`; `wake_with` helper; `msg_send`, `msg_receive` |
| `h/syscall_abi.hpp` | `SYS_SEND 0x17`, `SYS_RECEIVE 0x18` |
| `h/syscall_c.h` | `void send(thread_t,char*)`, `char* receive()` |
| `src/syscall.cpp` | `send` → `ecall2`, `receive` → `ecall0` |
| `src/trap.cpp` | `case SYS_SEND`, `case SYS_RECEIVE` (block-and-switch handling) |
| `h/syscall_cpp.hpp` | `void Thread::send(char*)`, `char* Thread::receive()` |
| `src/syscall_cpp.cpp` | thin wrappers over the C API |
| `tests/Messaging_test.{cpp,hpp}` | **new** — A/B/C ring exchange |
| `tests/userMain.cpp` | menu `11`; selection read as a line via `getString`+`stringToInt` (two-digit "11"); level-3 gate + `case 11:` |

---

## Test #11 (`tests/Messaging_test.cpp`)

Three threads A, B, C form a ring **A → B → C → A**. Each does `ROUNDS = 3` cycles of
`receive()` (blocking), print, then `send()` to the next node. Main kicks the ring by
sending `"start"` to A, then joins all three via a `done` semaphore.

### Expected output (deterministic, cooperative scheduler)

```
--- Messaging (Modifikacija: send / receive medju nitima) ---
A primio: start
B primio: poruka-od-A
C primio: poruka-od-B
A primio: poruka-od-C
B primio: poruka-od-A
C primio: poruka-od-B
A primio: poruka-od-C
B primio: poruka-od-A
C primio: poruka-od-B
Sve niti zavrsile razmenu poruka.
Messaging: PASS
TEST 11 (Modifikacija, send/receive medju nitima)
```

**Trace:** main deposits "start" into A's empty slot. A receives it, prints, sends
"poruka-od-A" to B (deposit), then blocks on its next receive. B receives, prints,
sends to C, blocks. C receives, prints, sends to A — but A is now blocked in receive,
so it's a **direct handoff** and A wakes. The single token circulates; exactly one
message is ever in flight, so no mailbox ever needs to hold two (no send ever blocks
on fullness in this test — the blocking-sender path is exercised by the design but
not by this particular ring). Nine receives total (3 per thread), all matched.

**Invariant:** each thread prints exactly 3 lines; the program ends with the summary
and PASS after all three signal `done`.

---

## Edge cases handled

1. **Receiver waiting when send arrives** → direct handoff (no deposit, no sender
   block); receiver's `a0` set at wake time.
2. **Slot full on send** → sender blocks on a FIFO `send_wait` queue; woken (in order)
   when the receiver drains the slot and refills it from the sender's
   `pending_send_msg`.
3. **receive on empty** → caller blocks; woken by the next send's handoff.
4. **send to a finished thread** → slot empty → deposit succeeds, never received;
   harmless (the test's final C→A send hits an already-finished A).
5. **Message lifetime** → shallow pointer; buffers must outlive delivery (test uses
   `static` char arrays).
6. **send to self** → allowed (deposits into own slot); user must avoid receive-then-
   never-send self-deadlocks.
7. **Return value across block** → woken receiver gets the pointer via its saved
   `trap_frame[a0]` (same mechanism as semaphore wake).

---

## Build / verify

Verified: builds and runs correctly on the target (QEMU), option 11 produces the
expected output above and prints `Messaging: PASS`.

```
make
make qemu       # choose option 11
```

Menu option: `11  Messaging (Modifikacija: send/receive)`. The menu reads the
selection as a line (`getString` + `stringToInt`) so the two-digit `11` parses;
single-digit numeric and lettered options are unaffected.
