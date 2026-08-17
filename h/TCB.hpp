#pragma once
#include "../lib/hw.h"

class TCB {
public:
    enum State { READY, RUNNING, BLOCKED, FINISHED, PENDING_QUOTA };

    // Index of the a0 register within a saved TrapFrame (see src/trap.cpp).
    // A woken thread's syscall return value is written here.
    static const int FRAME_A0_IDX = 8;

    // Sentinel returned by block_running(): tells the trap handler the caller
    // blocked and it should advance sepc and switch away.
    static const int WOULD_BLOCK = 1;

    static const int CTX_RA = 0;
    static const int CTX_SP = 1;
    static const int CTX_S0 = 2;
    static const int CTX_LEN = 14;

    uint64  context[CTX_LEN];
    void*   stack_bottom;
    void  (*body)(void*);
    void*   arg;
    TCB*    next;
    State   state;
    bool    is_kernel;

    void*    trap_frame;

    unsigned wait_n;

    int      sem_result;

    size_t   id;

    static int create(TCB** handle_out,
                      void (*body)(void*),
                      void* arg,
                      void* stack_top);

    __attribute__((noreturn)) static void exit();

    static void dispatch();

    static size_t get_thread_id();

    static void set_maximum_threads(int n);

    // --- Generic block / wake primitive (shared by all sync modifications) ---
    // Mark the running thread BLOCKED. Returns WOULD_BLOCK; the trap handler
    // then advances sepc and calls Scheduler::switch_to_next().
    static int block_running();
    // Make a blocked thread runnable again, delivering retval as its syscall
    // return value (written into its saved trap frame's a0).
    static void wake(TCB* t, uint64 retval = 0);

    static void init();

private:

    static void body_wrapper();

    static void* seed_initial_frame(void* stack_top, bool user_mode);

    static void admit_from_pending();

    static TCB* mainTCB;
    static TCB* idleTCB;
    static void idle_body(void*);

    static size_t next_id;

    static int    max_user_threads;
    static int    active_user_threads;
    static TCB*   pending_head;
    static TCB*   pending_tail;

    friend class Scheduler;
public:
    static TCB* running;
};

// Intrusive FIFO queue of TCBs, linked through TCB::next. Reusable for any
// per-object list of blocked threads (semaphore waiters, mailbox senders,
// paired-partner waiters, quota-pending threads, ...).
class WaitQueue {
public:
    WaitQueue() : head(nullptr), tail(nullptr) {}

    bool empty() const { return head == nullptr; }

    TCB* peek() const { return head; }

    void enqueue(TCB* t) {
        if (!t) return;
        t->next = nullptr;
        if (!head) head = tail = t;
        else { tail->next = t; tail = t; }
    }

    TCB* dequeue() {
        TCB* t = head;
        if (!t) return nullptr;
        head = t->next;
        if (!head) tail = nullptr;
        t->next = nullptr;
        return t;
    }

private:
    TCB* head;
    TCB* tail;
};
