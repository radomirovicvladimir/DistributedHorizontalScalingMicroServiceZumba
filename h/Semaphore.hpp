#pragma once
#include "../lib/hw.h"

class TCB;

// Placement-new declaration. Definition lives in cpp_runtime.cpp. Needed
// where kernel-side code writes `new (ptr) KSemaphore(...)` — without this
// forward-decl the compiler doesn't know a placement-new operator exists.
inline void* operator new(size_t, void* p) noexcept { return p; }

// Kernel-side semaphore (a.k.a. SCB — Semaphore Control Block). The user-
// visible `Semaphore` in h/syscall_cpp.hpp is a thin wrapper around a
// `sem_t = KSemaphore*` handle.
//
// Semantics (per PDF §"C API" p.7 and Dijkstra convention):
//   value < 0  ⇒ (-value) threads are blocked waiting.
//   sem_wait(n):    value -= n. If now < 0, block caller.
//   sem_signal(n):  value += n. While value >= 0 and blocked queue non-empty,
//                   dequeue one waiter and wake it with return 0.
//   sem_close:      wake all waiters with return -1; restore their `wait_n`
//                   to `value` so future signals see a consistent count.
//
// Wait_n / signal_n behavior mirrors OS12026's canonical implementation:
// atomic n-unit operations. wait_n stores the requested n in TCB::wait_n so
// close can undo its subtraction from `value` on release.
//
// Non-preemptive kernel ⇒ no locking needed inside these ops. The trap
// dispatcher is the single mutator.

class KSemaphore {
public:
    static const int E_CLOSED = -1;    // returned to a waiter when closed
    static const int E_BAD    = -2;    // returned to caller on invalid args

    explicit KSemaphore(int init);
    ~KSemaphore();

    // Called from the trap handler on SYS_SEM_WAIT / SEM_WAIT_N.
    //   Returns  0: acquired immediately — caller stays runnable.
    //   Returns  1: blocked — caller must invoke Scheduler::switch_to_next().
    //               (When woken, its sem_wait C-API will return whatever the
    //               waker wrote into its saved trap frame's a0: 0 on
    //               successful signal, -1 on close.)
    //   Returns E_BAD: n was 0 or self is invalid.
    int wait(unsigned n = 1);

    // Called from the trap handler on SYS_SEM_SIGNAL / SEM_SIGNAL_N.
    //   Returns 0 on success. Wakes as many waiters as `value` allows,
    //   in FIFO order, one at a time (each dequeue increments the wake
    //   count by 1 and does not decrement `value` — that decrement was
    //   already applied by wait when the waiter blocked).
    int signal(unsigned n = 1);

    // Called from the trap handler on SYS_SEM_CLOSE (and from ~KSemaphore).
    //   Marks the sem closed, wakes all waiters with return -1, restoring
    //   `value` by adding back each waiter's `wait_n` (they never actually
    //   got the resources they subtracted from `value` in wait). After
    //   close, any wait/signal returns -1.
    int close();

    bool isClosed() const { return closed; }

private:
    int   value;
    bool  closed;
    TCB*  blocked_head;
    TCB*  blocked_tail;

    void  enqueue_blocked(TCB* t);
    TCB*  dequeue_blocked();
    void  wake(TCB* t, int retval);    // sets frame->a0 = retval, ready-queues t
};
