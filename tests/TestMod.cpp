#include "printing.hpp"
#include "TestMod.hpp"

Semaphore* startSEM    = nullptr;
Semaphore* finishedSEM = nullptr;

class Produce: public Thread {
private:
    int id;
public:
    Produce(int i) : Thread() { id = i; }

    void run() override {
        printString("Nit ");
        printInt(id);
        printString(" je zapocela\n");

        startSEM->wait();

        time_sleep(id * 5);
        thread_dispatch();

        finishedSEM->signal();

        printString("Nit ");
        printInt(id);
        printString(" se zavrsila\n");
    }
};

void testMod() {
    startSEM    = new Semaphore(0);
    finishedSEM = new Semaphore(0);

    Produce* threads[5];
    for (int i = 0; i < 5; i++) {
        threads[i] = new Produce(i + 1);
        threads[i]->start();
    }

    startSEM->signal_n(5);

    finishedSEM->wait_n(5);

    printString("Sve niti zavrsene.\n");

    for (int i = 0; i < 5; i++) delete threads[i];
    delete startSEM;
    delete finishedSEM;
}
