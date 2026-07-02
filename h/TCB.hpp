#pragma once
#include "../lib/hw.h"

class TCB {
public:
    enum State { READY, RUNNING, BLOCKED, FINISHED };

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

    static int create(TCB** handle_out,
                      void (*body)(void*),
                      void* arg,
                      void* stack_top);

    __attribute__((noreturn)) static void exit();

    static void dispatch();

    static void init();

private:

    static void body_wrapper();

    static void* seed_initial_frame(void* stack_top, bool user_mode);

    static TCB* mainTCB;
    static TCB* idleTCB;
    static void idle_body(void*);

    friend class Scheduler;
public:
    static TCB* running;
};
