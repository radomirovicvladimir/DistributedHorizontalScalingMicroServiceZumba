#include "../h/syscall_cpp.hpp"
#include "../h/syscall_c.h"

void Thread::run_trampoline(void* self_v) {
    Thread* self = static_cast<Thread*>(self_v);

    self->run();
}

Thread::Thread(void (*body_)(void*), void* arg_)
    : myHandle(nullptr), body(body_), arg(arg_) {}

Thread::Thread()
    : myHandle(nullptr),
      body(&Thread::run_trampoline),
      arg(this) {}

Thread::~Thread() {

}

int Thread::start() {
    return thread_create(&myHandle, body, arg);
}

void Thread::dispatch() { thread_dispatch(); }

int Thread::sleep(time_t) { return -1; }

Semaphore::Semaphore(unsigned init) : myHandle(nullptr) {
    sem_open(&myHandle, init);
}

Semaphore::~Semaphore() {
    if (myHandle) sem_close(myHandle);
}

int Semaphore::wait()   { return sem_wait(myHandle); }
int Semaphore::signal() { return sem_signal(myHandle); }

PeriodicThread::PeriodicThread(time_t p) : Thread(), period(p) {}
void PeriodicThread::terminate() { thread_exit(); }

char Console::getc()       { return ::getc(); }
void Console::putc(char c) {        ::putc(c); }

extern "C" int time_sleep(time_t) { return -1; }
