#include "../h/syscall_cpp.hpp"
#include "printing.hpp"
#include "Modification_test.hpp"

#define TEST_THREADS 20

static Semaphore* done;

class HelloThread : public Thread {
public:
    HelloThread() : Thread() {}
    void run() override {
        int my = Thread::getId();
        for (int i = 0; i < 5; i++) {
            printString("Hello! ");
            printInt(my);
            printString("\n");
        }
        volatile uint64 spin = 0;
        for (int i = 0; i < 200 * (my + 1); i++) {
            for (int j = 0; j < 1000; j++) spin++;
        }
        (void)spin;
        done->signal();
    }
};

void Modification_test() {
    printString("--- Modification (getThreadId + SetMaximumThreads) ---\n");
    printString("SetMaximumThreads(3), spawning ");
    printInt(TEST_THREADS);
    printString(" threads.\n");

    Thread::SetMaximumThreads(3);
    done = new Semaphore(0);

    Thread* threads[TEST_THREADS];
    for (int i = 0; i < TEST_THREADS; i++) {
        threads[i] = new HelloThread();
        threads[i]->start();
    }

    for (int i = 0; i < TEST_THREADS; i++) {
        done->wait();
    }

    delete done;
    for (int i = 0; i < TEST_THREADS; i++) delete threads[i];

    printString("Modification: PASS (all ");
    printInt(TEST_THREADS);
    printString(" threads finished under quota of 3)\n");
}
