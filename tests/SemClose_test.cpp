#include "../h/syscall_c.h"
#include "printing.hpp"
#include "SemClose_test.hpp"

#define N_WAITERS 4

static sem_t closableSem;
static sem_t doneSem;
static volatile int returnedNeg;
static volatile int returnedZero;

static void closable_waiter(void* arg) {
    int id = (int)(uint64)arg;
    printString("  waiter ");
    printInt(id);
    printString(" calls sem_wait\n");
    int r = sem_wait(closableSem);
    if (r < 0) {
        returnedNeg++;
        printString("  waiter ");
        printInt(id);
        printString(" got negative (as expected on close)\n");
    } else {
        returnedZero++;
        printString("  waiter ");
        printInt(id);
        printString(" got 0 (semaphore was signaled instead of closed?)\n");
    }
    sem_signal(doneSem);
}

void SemClose_test() {
    printString("--- SemClose ---\n");
    printString("Test: close semaphore while ");
    printInt(N_WAITERS);
    printString(" threads are blocked on wait\n");

    sem_open(&closableSem, 0);
    sem_open(&doneSem, 0);
    returnedNeg = 0;
    returnedZero = 0;

    thread_t t[N_WAITERS];
    for (int i = 0; i < N_WAITERS; i++) {
        thread_create(&t[i], closable_waiter, (void*)(uint64)i);
    }

    for (int i = 0; i < N_WAITERS * 2; i++) thread_dispatch();

    printString("All waiters blocked. Now sem_close(closableSem)\n");
    sem_close(closableSem);

    for (int i = 0; i < N_WAITERS; i++) sem_wait(doneSem);

    printString("returnedNeg=");
    printInt(returnedNeg);
    printString(" (expect ");
    printInt(N_WAITERS);
    printString(")\n");

    sem_close(doneSem);

    if (returnedNeg == N_WAITERS && returnedZero == 0)
        printString("SemClose: PASS\n");
    else
        printString("SemClose: FAIL\n");
}
