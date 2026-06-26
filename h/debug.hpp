#pragma once
#include "../lib/hw.h"

// Polling-UART debug helpers. Safe to call before traps/threads exist.
// Used for early-boot logging, panic, and smoke tests in Task 1.

extern "C" {
    void kputc(char c);
    void kputs(const char* s);
    void kputhex(uint64 v);
    void kputdec(uint64 v);

    // Halt QEMU cleanly. Writes 0x5555 to 0x100000 (qemu virt machine "test" device).
    __attribute__((noreturn)) void khalt();

    // Print message and halt. Use from asserts / invariant checks.
    __attribute__((noreturn)) void panic(const char* msg);
}
