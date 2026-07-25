#include "../h/syscall_c.h"
#include "printing.hpp"
#include "ThreadFlood_test.hpp"

#define ROUNDS 50
#define BATCH  10

static volatile int flooded;

static void flood_body(void*) {
    flooded++;
}

void ThreadFlood_test() {
    printString("--- ThreadFlood ---\n");
    printString("Create/exit ");
    printInt(ROUNDS * BATCH);
    printString(" threads in batches of ");
    printInt(BATCH);
    printString(".\n");
    printString("If graveyard reaping is broken, we OOM before finishing.\n");

    flooded = 0;
    for (int r = 0; r < ROUNDS; r++) {
        thread_t t[BATCH];
        for (int i = 0; i < BATCH; i++) {
            int rc = thread_create(&t[i], flood_body, 0);
            if (rc != 0) {
                printString("  thread_create failed at round ");
                printInt(r);
                printString(" (rc=");
                printInt(rc);
                printString(") — likely OOM\n");
                printString("ThreadFlood: FAIL\n");
                return;
            }
        }

        for (int i = 0; i < BATCH * 3; i++) thread_dispatch();

        if ((r & 7) == 0) {
            printString("  round ");
            printInt(r);
            printString(", flooded=");
            printInt(flooded);
            printString("\n");
        }
    }

    while (flooded < ROUNDS * BATCH) thread_dispatch();

    printString("Total ran: ");
    printInt(flooded);
    printString(" (expect ");
    printInt(ROUNDS * BATCH);
    printString(")\n");

    if (flooded == ROUNDS * BATCH)
        printString("ThreadFlood: PASS\n");
    else
        printString("ThreadFlood: FAIL (some threads didn't run)\n");
}
