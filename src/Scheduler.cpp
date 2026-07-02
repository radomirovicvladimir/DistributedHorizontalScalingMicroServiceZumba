#include "../h/Scheduler.hpp"
#include "../h/MemoryAllocator.hpp"
#include "../h/debug.hpp"

TCB* Scheduler::head = nullptr;
TCB* Scheduler::tail = nullptr;

static TCB* graveyard = nullptr;

static void reap_graveyard(TCB* running) {
    while (graveyard) {
        TCB* t = graveyard;
        graveyard = t->next;

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

void Scheduler::switch_to_next() {
    extern TCB* g_idle;
    TCB* old = TCB::running;

    TCB* nxt = get();
    if (!nxt) nxt = g_idle;

    if (old->state == TCB::FINISHED) {
        old->next = graveyard;
        graveyard = old;
    } else if (old->state == TCB::BLOCKED) {

    } else if (old != g_idle && old->state == TCB::RUNNING) {
        old->state = TCB::READY;
        put(old);
    }

    if (nxt == old &&
        old->state != TCB::FINISHED &&
        old->state != TCB::BLOCKED) {
        old->state = TCB::RUNNING;
        return;
    }

    nxt->state    = TCB::RUNNING;
    TCB::running  = nxt;

    context_switch(old->context, nxt->context);

    reap_graveyard(TCB::running);
}
