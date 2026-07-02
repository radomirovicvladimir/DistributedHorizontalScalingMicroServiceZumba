#pragma once
#include "TCB.hpp"

// FIFO ready queue. One instance owned by the kernel — kept as an all-static
// "uslužna klasa" (PDF footnote 18) since there's only ever one scheduler.
//
// The idle thread is NEVER put on this queue; it's the fallback in switch_to_next()
// when the queue is empty. This keeps FIFO fairness for user threads while still
// guaranteeing something is always runnable.
//
// TCB uses its `next` pointer as the queue link — enqueueing a TCB twice would
// corrupt the queue. All callers must respect: a TCB is either RUNNING, on the
// ready queue, or FINISHED (or blocked on a semaphore in Task 3).

class Scheduler {
public:
    static void  put(TCB* t);      // enqueue at tail
    static TCB*  get();             // dequeue from head; nullptr if empty
    static bool  empty();

    // Synchronous context switch: put current onto ready queue if it's still
    // runnable, then transfer control to the next ready thread (or idle).
    // Called from thread_dispatch, thread_exit, and later sem_wait.
    // When state == FINISHED, the current TCB is NOT re-enqueued.
    static void  switch_to_next();

private:
    static TCB* head;
    static TCB* tail;

    Scheduler() = delete;
};

// Assembly primitive: save 14 callee-saved regs into *old, load from *neu, ret.
// Implemented in src/context_switch.S. Extern-"C" so the mangled C++ name
// doesn't get in the way.
extern "C" void context_switch(uint64* old_ctx, uint64* new_ctx);
