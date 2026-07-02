#pragma once
#include "TCB.hpp"

class Scheduler {
public:
    static void  put(TCB* t);
    static TCB*  get();
    static bool  empty();

    static void  switch_to_next();

private:
    static TCB* head;
    static TCB* tail;

    Scheduler() = delete;
};

extern "C" void context_switch(uint64* old_ctx, uint64* new_ctx);
