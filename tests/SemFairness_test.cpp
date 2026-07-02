#include "../h/syscall_c.h"
#include "printing.hpp"
#include "SemFairness_test.hpp"

#define WORKERS 5

static sem_t fairSem;
static sem_t doneSem;
static volatile int wakeOrder[WORKERS];
static volatile int wakeIdx;

static void fair_worker(void* arg) {
    int myId = (int)(uint64)arg;
    sem_wait(fairSem);
    int slot = wakeIdx++;
    if (slot < WORKERS) wakeOrder[slot] = myId;
    printString("  woke worker ");
    printInt(myId);
    printString("\n");
    sem_signal(doneSem);
}

void SemFairness_test() {
    printString("--- SemFairness ---\n");
    printString("Expect wake order: 0 1 2 3 4 (FIFO)\n");

    sem_open(&fairSem, 0);
    sem_open(&doneSem, 0);
    wakeIdx = 0;
    for (int i = 0; i < WORKERS; i++) wakeOrder[i] = -1;

    thread_t t[WORKERS];
    for (int i = 0; i < WORKERS; i++) {
        thread_create(&t[i], fair_worker, (void*)(uint64)i);
        thread_dispatch();
    }

    printString("all workers blocked; signaling one at a time\n");
    for (int i = 0; i < WORKERS; i++) {
        sem_signal(fairSem);
        thread_dispatch();
    }

    for (int i = 0; i < WORKERS; i++) sem_wait(doneSem);

    printString("Observed order:");
    bool ok = true;
    for (int i = 0; i < WORKERS; i++) {
        printString(" ");
        printInt(wakeOrder[i]);
        if (wakeOrder[i] != i) ok = false;
    }
    printString("\n");

    sem_close(fairSem);
    sem_close(doneSem);

    if (ok) printString("SemFairness: PASS\n");
    else    printString("SemFairness: FAIL (order not FIFO)\n");
}
