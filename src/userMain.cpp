// User-mode entry, per PDF §"Odnos jezgra i korisničke aplikacije": kernel's
// main() initializes, then spawns a thread whose body is this function.
// This file is the OS1 2026 canonical test harness (matches the layout in
// tests/uputstvo.txt) so that the same public tests the grader runs will
// build against our kernel unchanged.
//
// LEVEL_N_IMPLEMENTED flags mirror the grading tasks — flip the ones we've
// finished to 1, keep the rest at 0. `uputstvo.txt` documents this exact
// pattern; deviating breaks compatibility with the test harness.

#include "../tests/printing.hpp"

#define LEVEL_1_IMPLEMENTED 1
#define LEVEL_2_IMPLEMENTED 1
#define LEVEL_3_IMPLEMENTED 0
#define LEVEL_4_IMPLEMENTED 0

#if LEVEL_2_IMPLEMENTED == 1
#include "../tests/Threads_C_API_test.hpp"
#include "../tests/Threads_CPP_API_test.hpp"
#include "../tests/System_Mode_test.hpp"
#endif

// (Task 3 tests re-added when Semaphore is implemented.)
// (Task 4 tests never — we're skipping that task and using console.lib.)

extern "C" void userMain() {
    printString("Unesite broj testa? [1-7]\n");
    char digit = getc();
    putc(digit);                                // echo so the user sees what they typed
    putc('\n');
    int test = digit - '0';
    getc();                                     // consume the newline (Enter key)

    if ((test >= 1 && test <= 2) || test == 7) {
        if (LEVEL_2_IMPLEMENTED == 0) {
            printString("Nije navedeno da je zadatak 2 implementiran\n");
            return;
        }
    }
    if (test >= 3 && test <= 4) {
        if (LEVEL_3_IMPLEMENTED == 0) {
            printString("Nije navedeno da je zadatak 3 implementiran\n");
            return;
        }
    }
    if (test >= 5 && test <= 6) {
        if (LEVEL_4_IMPLEMENTED == 0) {
            printString("Nije navedeno da je zadatak 4 implementiran\n");
            return;
        }
    }

    switch (test) {
        case 1:
#if LEVEL_2_IMPLEMENTED == 1
            Threads_C_API_test();
            printString("TEST 1 (zadatak 2, niti C API i sinhrona promena konteksta)\n");
#endif
            break;
        case 2:
#if LEVEL_2_IMPLEMENTED == 1
            Threads_CPP_API_test();
            printString("TEST 2 (zadatak 2., niti CPP API i sinhrona promena konteksta)\n");
#endif
            break;
        case 7:
#if LEVEL_2_IMPLEMENTED == 1
            System_Mode_test();
            printString("Test se nije uspesno zavrsio\n");
            printString("TEST 7 (zadatak 2., testiranje da li se korisnicki kod izvrsava u korisnickom rezimu)\n");
#endif
            break;
        default:
            printString("Niste uneli odgovarajuci broj za test (podržani: 1, 2, 7)\n");
    }
}
