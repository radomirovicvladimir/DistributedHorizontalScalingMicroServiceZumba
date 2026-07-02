#pragma once
#include "../lib/hw.h"

extern "C" {
    void kputc(char c);
    void kputs(const char* s);
    void kputhex(uint64 v);
    void kputdec(uint64 v);
    __attribute__((noreturn)) void khalt();
    __attribute__((noreturn)) void kpanic(const char* msg);
}
