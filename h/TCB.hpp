#pragma once
#include "../lib/hw.h"

class TCB {
public:
    enum State { READY, RUNNING, BLOCKED, FINISHED, PENDING_QUOTA };

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
