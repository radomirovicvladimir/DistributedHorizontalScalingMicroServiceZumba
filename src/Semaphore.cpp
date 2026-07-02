#include "../h/Semaphore.hpp"
#include "../h/TCB.hpp"
#include "../h/Scheduler.hpp"
#include "../h/debug.hpp"

static const int FRAME_A0_IDX = 8;

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

        value -= (int)n;
        return 0;
    }

    TCB::running->wait_n = n;
    TCB::running->state  = TCB::BLOCKED;
    enqueue_blocked(TCB::running);
    return 1;
}

int KSemaphore::signal(unsigned n) {
    if (closed) return E_CLOSED;
    if (n == 0) return 0;

    value += (int)n;

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

    while (TCB* t = dequeue_blocked()) {
        wake(t, E_CLOSED);
    }
    return 0;
}
