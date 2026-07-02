#include "../h/Semaphore.hpp"
#include "../h/TCB.hpp"
#include "../h/Scheduler.hpp"
#include "../h/debug.hpp"

// TrapFrame's a0 field offset. MUST match struct TrapFrame in src/trap.cpp
// and the layout in src/trap_entry.S:
//     0..7  : ra, t0-t6
//     8..15 : a0, a1, a2, a3, a4, a5, a6, a7
//     16    : sepc
//     17    : sstatus
// So a0 is at index 8 (byte offset 8*8 = 64).
static const int FRAME_A0_IDX = 8;

// Semantics we implement:
//
//   * `value` is a non-negative count of available units (Dijkstra's P/V).
//     It NEVER goes negative — deficit is tracked implicitly by the length
//     of the blocked queue.
//
//   * wait(n):
//        if value >= n:          value -= n; return 0 (acquired).
//        else:                   block current thread (queued with wait_n).
//     A blocked thread does NOT modify value — no debt accounting.
//
//   * signal(n):
//        value += n. Then attempt to satisfy head-of-queue waiters:
//        while head exists AND value >= head.wait_n:
//            value -= head.wait_n
//            dequeue head, wake it (return 0).
//
//   * close():
//        mark closed. Wake all waiters with return -1. `value` is left as
//        whatever it was; further wait/signal returns -1.
//
// This formulation matches the PDF's expected behavior and handles
// heterogeneous wait_n cleanly. The OS12026 project uses a signed-value
// convention which subtly mis-handles wait_n > 1 — we avoid that by never
// letting value go negative.

KSemaphore::KSemaphore(int init)
    : value(init < 0 ? 0 : init),
      closed(false),
      blocked_head(nullptr),
      blocked_tail(nullptr) {}

KSemaphore::~KSemaphore() {
    close();
}

void KSemaphore::enqueue_blocked(TCB* t) {
    t->next = nullptr;
    if (!blocked_head) {
        blocked_head = blocked_tail = t;
    } else {
        blocked_tail->next = t;
        blocked_tail       = t;
    }
}

TCB* KSemaphore::dequeue_blocked() {
    TCB* t = blocked_head;
    if (!t) return nullptr;
    blocked_head = t->next;
    if (!blocked_head) blocked_tail = nullptr;
    t->next = nullptr;
    return t;
}

void KSemaphore::wake(TCB* t, int retval) {
    // Write the return value into the woken thread's saved trap frame's a0
    // slot. When the scheduler eventually restores this thread and the trap
    // epilogue does `sret`, user code sees `sem_wait` return `retval`.
    if (t->trap_frame) {
        uint64* frame = (uint64*)t->trap_frame;
        frame[FRAME_A0_IDX] = (uint64)(long)retval;
    }
    t->sem_result = retval;
    t->state      = TCB::READY;
    Scheduler::put(t);
}

int KSemaphore::wait(unsigned n) {
    if (closed) return E_CLOSED;
    if (n == 0) return 0;

    if ((int)n <= value) {
        // Enough resources: acquire immediately.
        value -= (int)n;
        return 0;
    }

    // Not enough. Block. Remember how many we needed so signal knows when
    // to release us and close knows what to restore.
    TCB::running->wait_n = n;
    TCB::running->state  = TCB::BLOCKED;
    enqueue_blocked(TCB::running);
    return 1;                    // caller must switch away
}

int KSemaphore::signal(unsigned n) {
    if (closed) return E_CLOSED;
    if (n == 0) return 0;

    value += (int)n;

    // Wake as many head-of-queue waiters as we can afford. Each takes their
    // wait_n out of `value` and the next waiter becomes head.
    while (blocked_head && (int)blocked_head->wait_n <= value) {
        TCB* t = dequeue_blocked();
        value -= (int)t->wait_n;
        wake(t, 0);
    }
    return 0;
}

int KSemaphore::close() {
    if (closed) return 0;
    closed = true;
    // Wake all remaining waiters with -1. Value stays as is — subsequent
    // ops on this sem return E_CLOSED anyway.
    while (TCB* t = dequeue_blocked()) {
        wake(t, E_CLOSED);
    }
    return 0;
}
