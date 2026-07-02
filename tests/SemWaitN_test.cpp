#include "../h/syscall_c.h"
#include "printing.hpp"
#include "SemWaitN_test.hpp"

static sem_t poolSem;
static sem_t doneSem;
static volatile int nWoken;

static void waiter3(void* arg) {
    int id = (int)(uint64)arg;
    printString("  worker ");
    printInt(id);
    printString(" wait_n(3)\n");
    int r = sem_wait_n(poolSem, 3);
    if (r == 0) {
        nWoken++;
        printString("  worker ");
        printInt(id);
        printString(" acquired 3\n");
    } else {
        printString("  worker ");
        printInt(id);
        printString(" got error\n");
    }
    sem_signal(doneSem);
}

void SemWaitN_test() {
    printString("--- SemWaitN ---\n");

    printString("Phase 1: value=10, one wait_n(3) succeeds without blocking\n");
    sem_open(&poolSem, 10);
    int r = sem_wait_n(poolSem, 3);
    printString("  wait_n(3) returned ");
    printInt(r);
    printString(" (expect 0)\n");
    sem_close(poolSem);

    printString("Phase 2: value=2 with 3 waiters wanting 3 each; signal 9 units\n");
    sem_open(&poolSem, 2);
    sem_open(&doneSem, 0);
    nWoken = 0;

    thread_t t1, t2, t3;
    thread_create(&t1, waiter3, (void*)(uint64)1);
    thread_create(&t2, waiter3, (void*)(uint64)2);
    thread_create(&t3, waiter3, (void*)(uint64)3);

    for (int i = 0; i < 6; i++) thread_dispatch();

    printString("Now signal_n(9) - should wake all 3 waiters (3 units each)\n");
    sem_signal_n(poolSem, 9);
    for (int i = 0; i < 3; i++) sem_wait(doneSem);

    printString("nWoken = ");
    printInt(nWoken);
    printString(" (expect 3)\n");

    sem_close(poolSem);
    sem_close(doneSem);

    if (nWoken == 3) printString("SemWaitN: PASS\n");
    else             printString("SemWaitN: FAIL\n");
}
