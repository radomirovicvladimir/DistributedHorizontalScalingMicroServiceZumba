#pragma once
#include "../lib/hw.h"

class TCB;

inline void* operator new(size_t, void* p) noexcept { return p; }

class KSemaphore {
public:
    static const int E_CLOSED = -1;
    static const int E_BAD    = -2;

    explicit KSemaphore(int init);
    ~KSemaphore();

    int wait(unsigned n = 1);

    int signal(unsigned n = 1);

    int close();

    bool isClosed() const { return closed; }

private:
    int   value;
    bool  closed;
    TCB*  blocked_head;
    TCB*  blocked_tail;

    void  enqueue_blocked(TCB* t);
    TCB*  dequeue_blocked();
    void  wake(TCB* t, int retval);
};
