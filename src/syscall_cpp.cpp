#include "../h/syscall_cpp.hpp"
#include "../h/syscall_c.h"

// C++ API adapter for Task 2. Interface skeleton is fixed by the PDF —
// non-static data members, base classes, and virtual-function ordering may
// NOT be extended (PDF p.11), so all implementation goes into the .cpp.
//
// PDF p.11: "Ukoliko je konstruktorom postavljen pokazivač na funkciju,
// operaciju run treba ignorisati u svakom slučaju." — if the function-pointer
// ctor is used, run() is ignored even if overridden. Our helper knows both:
//   * function-pointer ctor: body != nullptr, arg = user's arg
//   * protected default ctor: body = &Thread::run_trampoline, arg = this
// The kernel-side wrapper only sees body(arg), so this dispatching is opaque
// to it — nice separation of layers.

// --- Thread ---------------------------------------------------------------

void Thread::run_trampoline(void* self_v) {
    Thread* self = static_cast<Thread*>(self_v);
    // Virtual dispatch: derived class's run() fires. If the derived class
    // never overrode run(), the base's empty body {} runs and the thread
    // terminates immediately — the kernel wrapper then calls thread_exit.
    self->run();
}

Thread::Thread(void (*body_)(void*), void* arg_)
    : myHandle(nullptr), body(body_), arg(arg_) {}

Thread::Thread()
    : myHandle(nullptr),
      body(&Thread::run_trampoline),
      arg(this) {}

Thread::~Thread() {
    // PDF says destructors indicate objects can be destroyed. We don't yet
    // support "kill this thread from outside" — that's beyond Task 2's scope.
    // Nothing to reclaim here; the TCB and stack are freed by the kernel
    // wrapper when the thread's body returns via thread_exit.
}

int Thread::start() {
    return thread_create(&myHandle, body, arg);
}

void Thread::dispatch() { thread_dispatch(); }

// Task 4 territory — no-op stub so any app.lib references still link.
int Thread::sleep(time_t) { return -1; }

// --- Semaphore ------------------------------------------------------------

Semaphore::Semaphore(unsigned init) : myHandle(nullptr) {
    sem_open(&myHandle, init);
}

Semaphore::~Semaphore() {
    if (myHandle) sem_close(myHandle);
}

int Semaphore::wait()   { return sem_wait(myHandle); }
int Semaphore::signal() { return sem_signal(myHandle); }

// --- PeriodicThread stub (Task 4 — not implementing) ---------------------

PeriodicThread::PeriodicThread(time_t p) : Thread(), period(p) {}
void PeriodicThread::terminate() { thread_exit(); }

// --- Console (Task 4 fallback via console.lib) ---------------------------

char Console::getc()       { return ::getc(); }
void Console::putc(char c) {        ::putc(c); }

extern "C" int time_sleep(time_t) { return -1; }
