#include "printing.hpp"

#define LEVEL_1_IMPLEMENTED 1
#define LEVEL_2_IMPLEMENTED 1
#define LEVEL_3_IMPLEMENTED 1
#define LEVEL_4_IMPLEMENTED 0

#if LEVEL_2_IMPLEMENTED == 1
#include "Threads_C_API_test.hpp"
#include "Threads_CPP_API_test.hpp"
#include "System_Mode_test.hpp"
#endif

#if LEVEL_3_IMPLEMENTED == 1
#include "ConsumerProducer_C_API_test.hpp"
#include "ConsumerProducer_CPP_Sync_API_test.hpp"
#endif

#include "MemAllocStress_test.hpp"
#include "SemFairness_test.hpp"
#include "SemWaitN_test.hpp"
#include "SemClose_test.hpp"
#include "ThreadFlood_test.hpp"

static void print_menu() {
    printString("=== Test menu ===\n");
    printString("  1  Threads (C API)\n");
    printString("  2  Threads (C++ API)\n");
    printString("  3  Producer-Consumer (C API)\n");
    printString("  4  Producer-Consumer (C++ Sync API)\n");
    printString("  7  System Mode (U-mode fault check)\n");
    printString("  --- extras (local) ---\n");
    printString("  a  MemAllocStress\n");
    printString("  b  SemFairness (FIFO wake order)\n");
    printString("  c  SemWaitN (wait_n / signal_n atomicity)\n");
    printString("  d  SemClose (wake blocked waiters with -1)\n");
    printString("  e  ThreadFlood (mass create/exit, graveyard proof)\n");
    printString("Odaberite test: ");
}

void userMain() {
    print_menu();
    char sel = getc();
    putc(sel);
    putc('\n');
    getc();

    if (sel >= '1' && sel <= '7') {
        int test = sel - '0';
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
            case 3:
#if LEVEL_3_IMPLEMENTED == 1
                producerConsumer_C_API();
                printString("TEST 3 (zadatak 3., kompletan C API sa semaforima, sinhrona promena konteksta)\n");
#endif
                break;
            case 4:
#if LEVEL_3_IMPLEMENTED == 1
                producerConsumer_CPP_Sync_API();
                printString("TEST 4 (zadatak 3., kompletan CPP API sa semaforima, sinhrona promena konteksta)\n");
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
                printString("Nepodrzan test\n");
        }
        return;
    }

    switch (sel) {
        case 'a':
            MemAllocStress_test();
            break;
        case 'b':
            SemFairness_test();
            break;
        case 'c':
            SemWaitN_test();
            break;
        case 'd':
            SemClose_test();
            break;
        case 'e':
            ThreadFlood_test();
            break;
        default:
            printString("Nepoznata opcija.\n");
    }
}
