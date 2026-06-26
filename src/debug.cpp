#include "../h/debug.hpp"
#include "../lib/hw.h"

// Polling UART. The console controller's status bit 5 (CONSOLE_TX_STATUS_BIT)
// indicates "ready to accept a byte for transmission" (PDF §Konzola, p.17).
// We spin on that bit then store one byte into CONSOLE_TX_DATA.
//
// Note: CONSOLE_STATUS / CONSOLE_TX_DATA are extern const uint64 holding the
// MMIO addresses (defined in hw.lib). Cast their VALUE to a volatile pointer.

extern "C" void kputc(char c) {
    volatile uint8* st = (volatile uint8*)CONSOLE_STATUS;
    volatile uint8* tx = (volatile uint8*)CONSOLE_TX_DATA;
    while (!(*st & CONSOLE_TX_STATUS_BIT)) { /* spin */ }
    *tx = (uint8)c;
}

extern "C" void kputs(const char* s) {
    while (*s) kputc(*s++);
}

extern "C" void kputhex(uint64 v) {
    kputc('0'); kputc('x');
    bool any = false;
    for (int i = 60; i >= 0; i -= 4) {
        uint8 nyb = (uint8)((v >> i) & 0xF);
        if (nyb || any || i == 0) {
            kputc(nyb < 10 ? char('0' + nyb) : char('a' + nyb - 10));
            any = true;
        }
    }
}

extern "C" void kputdec(uint64 v) {
    if (v == 0) { kputc('0'); return; }
    char buf[24]; int n = 0;
    while (v) { buf[n++] = char('0' + v % 10); v /= 10; }
    while (n--) kputc(buf[n]);
}

extern "C" __attribute__((noreturn)) void khalt() {
    *(volatile uint32*)0x100000 = 0x5555;
    for (;;) { /* unreachable, but compiler doesn't know */ }
}

extern "C" __attribute__((noreturn)) void kpanic(const char* msg) {
    kputs("\nPANIC: ");
    kputs(msg);
    kputc('\n');
    khalt();
}
