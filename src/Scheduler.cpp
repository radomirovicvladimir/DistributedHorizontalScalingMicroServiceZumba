#include "../h/Scheduler.hpp"
#include "../h/MemoryAllocator.hpp"
#include "../h/debug.hpp"

TCB* Scheduler::head = nullptr;
TCB* Scheduler::tail = nullptr;

// Graveyard: FINISHED thread TCBs waiting to have their stacks + TCB memory
// reclaimed. We can't free them synchronously inside TCB::exit because we're
// still using the exiting thread's stack — the code sequence
//    Scheduler::switch_to_next -> context_switch(old, new)
// reads/writes memory on `old`'s stack. So the exiting thread queues itself
// here, and the NEXT thread to enter switch_to_next (which is by definition
// running on a different stack) reaps the graveyard entries.
static TCB* graveyard = nullptr;

static void reap_graveyard(TCB* running) {
    while (graveyard) {
        TCB* t = graveyard;
        graveyard = t->next;
        // Never reap ourselves — belt-and-suspenders; TCB::exit shouldn't
        // put running on the graveyard in the first place.
        if (t == running) continue;
        if (t->stack_bottom) MemoryAllocator::free(t->stack_bottom);
        MemoryAllocator::free(t);
    }
}

void Scheduler::put(TCB* t) {
    if (!t) return;
    t->next = nullptr;
    if (!head) {
        head = tail = t;
    } else {
        tail->next = t;
        tail       = t;
    }
}

TCB* Scheduler::get() {
    TCB* t = head;
    if (!t) return nullptr;
    head = t->next;
    if (!head) tail = nullptr;
    t->next = nullptr;
    return t;
}

bool Scheduler::empty() { return head == nullptr; }

// Central switch point. Called synchronously from:
//   - thread_dispatch: put running back, run next
//   - thread_exit:     leave running as FINISHED, run next (and enqueue on graveyard)
//   - sem_wait:        caller has already set state=BLOCKED and put self on
//                      the sem's blocked queue; we just switch away.
//
// The caller must have adjusted `TCB::running->state` appropriately BEFORE
// entering. This routine only decides who runs next and swaps context.
void Scheduler::switch_to_next() {
    extern TCB* g_idle;                // set up in TCB::init(); see TCB.cpp
    TCB* old = TCB::running;

    // Pick next: prefer the ready queue, fall back to idle. Idle is never
    // in the queue so we won't accidentally "double" it.
    TCB* nxt = get();
    if (!nxt) nxt = g_idle;

    // Decide what to do with `old`:
    //   FINISHED → graveyard (reaped by next thread's return from ctx switch).
    //   BLOCKED  → leave alone; already on some sem's blocked queue.
    //   RUNNING  → still runnable, re-queue (unless it's idle — idle isn't
    //              in the queue; it re-enters via the empty()-fallback).
    if (old->state == TCB::FINISHED) {
        old->next = graveyard;
        graveyard = old;
    } else if (old->state == TCB::BLOCKED) {
        // no-op — caller owns the linkage
    } else if (old != g_idle && old->state == TCB::RUNNING) {
        old->state = TCB::READY;
        put(old);
    }

    // Fast path: only if we'd be switching to ourselves AND we're still
    // runnable, stay put. (A FINISHED or BLOCKED thread must switch away
    // even if the only alternative is idle — otherwise we return into
    // code we shouldn't.)
    if (nxt == old &&
        old->state != TCB::FINISHED &&
        old->state != TCB::BLOCKED) {
        old->state = TCB::RUNNING;
        return;
    }

    nxt->state    = TCB::RUNNING;
    TCB::running  = nxt;

    // context_switch saves old's callee-saved regs into old->context and
    // loads nxt->context into the CPU. When we come back (as `old` being
    // restored later), we resume immediately after this call.
    context_switch(old->context, nxt->context);

    // Whoever wakes up here is now on THEIR own stack — safe to reap any
    // FINISHED thread whose TCB was previously graveyarded.
    reap_graveyard(TCB::running);
}
