#include "../h/debug.hpp"
#include "../lib/hw.h"

// PDF §Konzola, p.17: CONSOLE_STATUS bit 5 = "TX ready". Spin then write.
// CONSOLE_*_DATA are extern const uint64 holding the MMIO addresses.

extern "C" void kputc(char c) {
    while (!(*(volatile uint8*)CONSOLE_STATUS & CONSOLE_TX_STATUS_BIT)) {}
    *(volatile uint8*)CONSOLE_TX_DATA = (uint8)c;
}

extern "C" void kputs(const char* s) { while (*s) kputc(*s++); }

extern "C" void kputhex(uint64 v) {
    kputs("0x");
    bool any = false;
    for (int i = 60; i >= 0; i -= 4) {
        uint8 n = (v >> i) & 0xF;
        if (n || any || i == 0) {
            kputc(n < 10 ? char('0' + n) : char('a' + n - 10));
            any = true;
        }
    }
}

extern "C" void kputdec(uint64 v) {
    if (!v) { kputc('0'); return; }
    char b[24]; int n = 0;
    while (v) { b[n++] = char('0' + v % 10); v /= 10; }
    while (n--) kputc(b[n]);
}

extern "C" void khalt() {
    *(volatile uint32*)0x100000 = 0x5555;
    for (;;) {}
}

extern "C" void kpanic(const char* msg) {
    kputs("\nPANIC: "); kputs(msg); kputc('\n');
    khalt();
}
