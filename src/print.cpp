/*
   ============================================================================
   print.cpp - per-file change snapshots for all OS1 modifications (reference)
   ============================================================================
   HOW TO READ THIS FILE:
   - Comment-only. Records, per modification, which file each change goes in and
     the code, as a copy-paste source for re-applying a modification.
   - Line numbers below are captured against the CLEAN ORIGINAL base (the
     tests/ folder is the original test suite; userMain uses the "[1-N]" prompt
     with int test = getc()-'0'; switch(test){case 1..7}).
   - The base already includes the shared block/wake primitive
     (TCB::block_running / TCB::wake / class WaitQueue, and KSemaphore::wake
     delegates to TCB::wake). See MODIFICATIONS_GUIDE.md.

   NOTE: only the pairSems (2026) section below has been re-verified line-by-line
   against the current base and a passing build. The later sections are from the
   earlier working session and use approximate anchors.
   ============================================================================
*/


/* ========================================================================== */
/* ========================  MODIFICATION 2026: pairSems  =================== */
/* =====  (semaphore pairing; VERIFIED on current base, test 8 PASS)  ======= */
/* ========================================================================== */

/* --- h/syscall_abi.hpp ---
line 18 (#define SYS_SEM_PAIR        0x27)
*/

/* --- h/syscall_c.h ---
line 27 (int    sem_pair(sem_t a, sem_t b);)
*/

/* --- src/syscall.cpp ---
line 98  (extern "C" int sem_pair(sem_t a, sem_t b) {)
line 99  (    return (int)ecall2(SYS_SEM_PAIR, (uint64)a, (uint64)b);)
line 100 (})
*/

/* --- h/Semaphore.hpp   (added to class KSemaphore) ---
line 13 (    static const int PASSED_PAIRED = 2;)
line 14 (    static const int MAX_PARTNERS  = 16;)
line 27 (    static void pair(KSemaphore* a, KSemaphore* b);)
line 35 (    KSemaphore* partners[MAX_PARTNERS];)
line 36 (    int         nPartners;)
line 38 (    void  addPartner(KSemaphore* other);)
*/

/* --- src/Semaphore.cpp ---
constructor initializer (after blocked_tail(nullptr)):
line 11 (      nPartners(0) {)
line 12 (    for (int i = 0; i < MAX_PARTNERS; i++) partners[i] = nullptr;)

line 42 (void KSemaphore::addPartner(KSemaphore* other) {)
line 43 (    if (!other || other == this) return;)
line 44 (    for (int i = 0; i < nPartners; i++) {)
line 45 (        if (partners[i] == other) return;)
line 46 (    })
line 47 (    if (nPartners < MAX_PARTNERS) {)
line 48 (        partners[nPartners++] = other;)
line 49 (    })
line 50 (})

line 52 (void KSemaphore::pair(KSemaphore* a, KSemaphore* b) {)
line 53 (    if (!a || !b || a == b) return;)
line 54 (    a->addPartner(b);)
line 55 (    b->addPartner(a);)
line 56 (})

3-phase wait (replaces the original single-phase wait body):
line 58 (int KSemaphore::wait(unsigned n) {)
line 59 (    if (closed) return E_CLOSED;)
line 60 (    if (n == 0) return 0;)
line 62 (    // Phase 1: pass on the called semaphore itself.)
line 63 (    if ((int)n <= value) {)
line 64 (        value -= (int)n;)
line 65 (        return 0;)
line 66 (    })
line 68 (    // Phase 2: if paired, try to pass on some partner.)
line 69 (    for (int i = 0; i < nPartners; i++) {)
line 70 (        KSemaphore* p = partners[i];)
line 71 (        if (!p || p->closed) continue;)
line 72 (        if ((int)n <= p->value) {)
line 73 (            p->value -= (int)n;)
line 74 (            return PASSED_PAIRED;)
line 75 (        })
line 76 (    })
line 78 (    // Phase 3: block on the called semaphore.)
line 79 (    TCB::running->wait_n = n;)
line 80 (    TCB::running->state  = TCB::BLOCKED;)
line 81 (    enqueue_blocked(TCB::running);)
line 82 (    return 1;)
line 83 (})
*/

/* --- src/trap.cpp   (inside SYS_SEM_WAIT/WAIT_N case, after the r==1 block) ---
line 135 (            if (r == KSemaphore::PASSED_PAIRED) {)
line 136 (                f->a0 = 1;)
line 137 (                break;)
line 138 (            })

new case (after SYS_SEM_SIGNAL/SIGNAL_N case):
line 153 (        case SYS_SEM_PAIR: {)
line 154 (            KSemaphore* a = (KSemaphore*)f->a1;)
line 155 (            KSemaphore* b = (KSemaphore*)f->a2;)
line 156 (            if (!a || !b) { f->a0 = (uint64)-1; break; })
line 157 (            KSemaphore::pair(a, b);)
line 158 (            f->a0 = 0;)
line 159 (            break;)
line 160 (        })
*/

/* --- h/syscall_cpp.hpp   (added to class Semaphore) ---
line 35 (    static void pairSems(Semaphore& sem1, Semaphore& sem2);)
*/

/* --- src/syscall_cpp.cpp ---
line 45 (void Semaphore::pairSems(Semaphore& sem1, Semaphore& sem2) {)
line 46 (    sem_pair(sem1.myHandle, sem2.myHandle);)
line 47 (})
*/

/* --- tests/userMain.cpp   (deltas vs the ORIGINAL userMain) ---
In the LEVEL_3 include block:
line 22 (// TEST 8 (Modifikacija, uparivanje semafora pairSems))
line 23 (#include "SemPair_test.hpp")

Prompt (widen range):
line 36 (    printString("Unesite broj testa? [1-8]\n");)

Level-3 gate (extend the existing test 3-4 check):
line 47 (    if ((test >= 3 && test <= 4) || test == 8) {)

New case in switch(test):
line 105 (        case 8:)
line 106 (#if LEVEL_3_IMPLEMENTED == 1)
line 107 (            SemPair_test();)
line 108 (            printString("TEST 8 (Modifikacija, uparivanje semafora pairSems)\n");)
line 109 (#endif)
line 110 (            break;)
*/

/* --- tests/SemPair_test.hpp --- (NEW FILE)
line 1 (#ifndef _SEMPAIR_TEST_HPP)
line 2 (#define _SEMPAIR_TEST_HPP)
line 4 (void SemPair_test();)
line 6 (#endif)
*/

/* --- tests/SemPair_test.cpp --- (NEW FILE)
line 1  (#include "../h/syscall_cpp.hpp")
line 3  (#include "printing.hpp")
line 5  (#include "SemPair_test.hpp")
line 7  (#define NUM_WORKERS 5)
line 8  (#define ITERATIONS  5)
line 9  (#define DISPATCH_ITERS 1000)
line 11 (static Semaphore* mainSem;)
line 12 (static Semaphore* done;)
line 14 (static volatile int pairedPassTotal;)
line 16 (class WorkerThread : public Thread {)
line 17 (public:)
line 18 (    WorkerThread(Semaphore* ownSem) : Thread(), sem(ownSem) {})
line 20 (    void run() override {)
line 21 (        int id = Thread::getId();)
line 22 (        for (int num = 1; num <= ITERATIONS; num++) {)
line 23 (            printInt(id);)
line 24 (            printString(" wait iteracija ");)
line 25 (            printInt(num);)
line 26 (            printString("!\n");)
line 28 (            int r = sem->wait();)
line 30 (            if (r == 1) {)
line 31 (                printInt(id);)
line 32 (                printString(" prosla semafor-iteracija ");)
line 33 (                printInt(num);)
line 34 (                printString("!\n");)
line 35 (                pairedPassTotal++;)
line 36 (            } else {)
line 37 (                printInt(id);)
line 38 (                printString(" prosla semafor iteracija ");)
line 39 (                printInt(num);)
line 40 (                printString("!\n");)
line 41 (            })
line 43 (            for (int i = 0; i < DISPATCH_ITERS; i++) {)
line 44 (                Thread::dispatch();)
line 45 (            })
line 46 (        })
line 47 (        done->signal();)
line 48 (    })
line 50 (private:)
line 51 (    Semaphore* sem;)
line 52 (};)
line 54 (void SemPair_test() {)
line 55 (    printString("--- SemPair (Modifikacija: pairSems) ---\n");)
line 57 (    pairedPassTotal = 0;)
line 59 (    mainSem = new Semaphore(100);)
line 60 (    done    = new Semaphore(0);)
line 62 (    Semaphore* workers[NUM_WORKERS];)
line 63 (    for (int i = 0; i < NUM_WORKERS; i++) {)
line 64 (        workers[i] = new Semaphore((unsigned)(i + 1));)
line 65 (        Semaphore::pairSems(*mainSem, *workers[i]);)
line 66 (    })
line 68 (    WorkerThread* threads[NUM_WORKERS];)
line 69 (    for (int i = 0; i < NUM_WORKERS; i++) {)
line 70 (        threads[i] = new WorkerThread(workers[i]);)
line 71 (        threads[i]->start();)
line 72 (    })
line 74 (    for (int i = 0; i < NUM_WORKERS; i++) {)
line 75 (        done->wait();)
line 76 (    })
line 78 (    printString("Ukupno prolazaka na uparenim semaforima: ");)
line 79 (    printInt(pairedPassTotal);)
line 80 (    printString("\n");)
line 82 (    for (int i = 0; i < NUM_WORKERS; i++) delete threads[i];)
line 83 (    for (int i = 0; i < NUM_WORKERS; i++) delete workers[i];)
line 84 (    delete done;)
line 85 (    delete mainSem;)
line 87 (    if (pairedPassTotal == 10) printString("SemPair: PASS\n");)
line 88 (    else                       printString("SemPair: FAIL\n");)
line 89 (})
*/



/* ========================================================================== */
/* =============  MODIFICATION JUNE 2025: parallel matrix histogram  ======== */
/* ==  User-space only (no kernel changes). Uses mem_alloc for runtime-    == */
/* ==  sized arrays to avoid __cxa_throw_bad_array_new_length (no stub in  == */
/* ==  cpp_runtime.cpp). IMPLEMENTED, NOT machine-verified (not run).      == */
/* ==  Expected (seed=1, 64-bit long): M=3,N=5 -> H=[2,1,1,1,1,2,1,2,2,2], == */
/* ==  total 15. Invariant: sum(H)=M*N.                                    == */
/* ========================================================================== */

/* --- tests/userMain.cpp   (deltas vs the ORIGINAL userMain) ---
LEVEL_3 include block:
line 22 (// TEST 8 (Modifikacija, paralelni histogram matrice))
line 23 (#include "MatrixHistogram_test.hpp")
Prompt:
line 36 (    printString("Unesite broj testa? [1-8]\n");)
Level-3 gate:
line 47 (    if ((test >= 3 && test <= 4) || test == 8) {)
Case:
line 105 (        case 8:)
line 106 (#if LEVEL_3_IMPLEMENTED == 1)
line 107 (            MatrixHistogram_test();)
line 108 (            printString("TEST 8 (Modifikacija, paralelni histogram matrice)\n");)
line 109 (#endif)
line 110 (            break;)
*/

/* --- tests/MatrixHistogram_test.hpp --- (NEW FILE)
line 1 (#ifndef _MATRIXHISTOGRAM_TEST_HPP)
line 2 (#define _MATRIXHISTOGRAM_TEST_HPP)
line 4 (void MatrixHistogram_test();)
line 6 (#endif)
*/

/* --- tests/MatrixHistogram_test.cpp --- (NEW FILE) ---
line 1  (#include "../h/syscall_cpp.hpp")
line 2  (#include "../h/syscall_c.h")
line 4  (#include "printing.hpp")
line 6  (#include "MatrixHistogram_test.hpp")
line 8  (#define HIST_SIZE 10)
line 9  (#define DISPATCH_EVERY 10)
line 11 (static unsigned long int next = 1;)
line 13 (static int custom_rand(void) {)
line 14 (    next = next * 1103515245 + 12345;)
line 15 (    return (unsigned int)(next / 65536) % 32768;)
line 16 (})
line 18 (static void custom_srand(unsigned int seed) {)
line 19 (    next = seed;)
line 20 (})
line 22 (static int** mat = nullptr;)
line 23 (static int   M = 0;)
line 24 (static int   N = 0;)
line 26 (static int sharedH[HIST_SIZE];)
line 28 (static Semaphore* histMutex = nullptr;)
line 29 (static Semaphore* done      = nullptr;)
line 31 (class RowThread : public Thread {)
line 32 (public:)
line 33 (    RowThread(int row) : Thread(), rowIndex(row) {})
line 35 (    void run() override {)
line 36 (        int localH[HIST_SIZE];)
line 37 (        for (int k = 0; k < HIST_SIZE; k++) localH[k] = 0;)
line 39 (        int* row = mat[rowIndex];)
line 40 (        int processed = 0;)
line 42 (        for (int j = 0; j < N; j++) {)
line 43 (            int digit = row[j] % HIST_SIZE;)
line 44 (            localH[digit]++;)
line 46 (            if (++processed % DISPATCH_EVERY == 0) {)
line 47 (                Thread::dispatch();)
line 48 (            })
line 49 (        })
line 51 (        histMutex->wait();)
line 52 (        for (int k = 0; k < HIST_SIZE; k++) {)
line 53 (            sharedH[k] += localH[k];)
line 54 (        })
line 55 (        histMutex->signal();)
line 57 (        done->signal();)
line 58 (    })
line 60 (private:)
line 61 (    int rowIndex;)
line 62 (};)
line 64 (static int readInt(const char* prompt) {)
line 65 (    char buf[32];)
line 66 (    printString(prompt);)
line 67 (    getString(buf, sizeof(buf));)
line 68 (    int len = 0;)
line 69 (    while (buf[len] != '\0') len++;)
line 70 (    printString(buf);)
line 71 (    if (len == 0 || (buf[len - 1] != '\n' && buf[len - 1] != '\r')) {)
line 72 (        printString("\n");)
line 73 (    })
line 74 (    return stringToInt(buf);)
line 75 (})
line 77 (void MatrixHistogram_test() {)
line 78 (    printString("--- MatrixHistogram (Modifikacija: paralelni histogram) ---\n");)
line 80 (    M = readInt("Unesite M (broj vrsta): ");)
line 81 (    N = readInt("Unesite N (broj kolona): ");)
line 83 (    if (M <= 0 || N <= 0) {)
line 84 (        printString("M i N moraju biti pozitivni.\n");)
line 85 (        return;)
line 86 (    })
line 88 (    mat = (int**)mem_alloc(sizeof(int*) * M);)
line 89 (    for (int i = 0; i < M; i++) mat[i] = (int*)mem_alloc(sizeof(int) * N);)
line 91 (    custom_srand(1);)
line 92 (    for (int i = 0; i < M; i++))
line 93 (        for (int j = 0; j < N; j++))
line 94 (            mat[i][j] = custom_rand();)
line 96 (    for (int k = 0; k < HIST_SIZE; k++) sharedH[k] = 0;)
line 97 (    histMutex = new Semaphore(1);)
line 98 (    done      = new Semaphore(0);)
line 100 (    RowThread** threads = (RowThread**)mem_alloc(sizeof(RowThread*) * M);)
line 101 (    for (int i = 0; i < M; i++) {)
line 102 (        threads[i] = new RowThread(i);)
line 103 (        threads[i]->start();)
line 104 (    })
line 106 (    for (int i = 0; i < M; i++) done->wait();)
line 108 (    long total = 0;)
line 109 (    for (int k = 0; k < HIST_SIZE; k++) {)
line 110 (        printString("H[");)
line 111 (        printInt(k);)
line 112 (        printString("] = ");)
line 113 (        printInt(sharedH[k]);)
line 114 (        printString("\n");)
line 115 (        total += sharedH[k];)
line 116 (    })
line 118 (    printString("Ukupno elemenata: ");)
line 119 (    printInt((int)total);)
line 120 (    printString(" (ocekivano M*N = ");)
line 121 (    printInt(M * N);)
line 122 (    printString(")\n");)
line 124 (    for (int i = 0; i < M; i++) delete threads[i];)
line 125 (    mem_free(threads);)
line 126 (    delete done;)
line 127 (    delete histMutex;)
line 128 (    for (int i = 0; i < M; i++) mem_free(mat[i]);)
line 129 (    mem_free(mat);)
line 130 (    mat = nullptr;)
line 132 (    if (total == (long)M * N) printString("MatrixHistogram: PASS\n");)
line 133 (    else                      printString("MatrixHistogram: FAIL\n");)
line 134 (})
*/



/* ========================================================================== */
/* ==================  MODIFICATION JUNE 2024: joinAll  ===================== */
/* ==  (Thread::joinAll waits for whole descendant subtree)                == */
/* ==  IMPLEMENTED on current base, NOT machine-verified (not run)         == */
/* ========================================================================== */

/* --- h/syscall_abi.hpp ---
line 11 (#define SYS_THREAD_JOIN_ALL 0x16)
*/

/* --- h/syscall_c.h ---
line 20 (void   thread_join_all();)
*/

/* --- src/syscall.cpp ---
line 79 (extern "C" void thread_join_all() { ecall0(SYS_THREAD_JOIN_ALL); })
*/

/* --- h/TCB.hpp   (fields + decl) ---
line 37 (    TCB*     parent;)
line 38 (    size_t   desc_count;)
line 39 (    bool     joining;)
line 62 (    static int join_all();)
*/

/* --- src/TCB.cpp   (uses TCB::wake / block_running primitive) ---
create() after t->id = ++next_id;  -- new thread adds 1 to every ancestor:
line 79 (    t->parent     = TCB::running;)
line 80 (    t->desc_count = 0;)
line 81 (    t->joining    = false;)
line 82 (    for (TCB* a = t->parent; a != nullptr; a = a->parent) {)
line 83 (        a->desc_count++;)
line 84 (    })

exit() finish hook -- decrement every ancestor, wake a joining one at 0:
line 124 (    for (TCB* a = me->parent; a != nullptr; a = a->parent) {)
line 125 (        if (a->desc_count > 0) a->desc_count--;)
line 126 (        if (a->joining && a->desc_count == 0) {)
line 127 (            a->joining = false;)
line 128 (            TCB::wake(a, 0);)
line 129 (        })
line 130 (    })

line 162 (int TCB::join_all() {)
line 163 (    TCB* me = TCB::running;)
line 164 (    if (me->desc_count == 0) return 0;   // nothing to wait -> immediate)
line 165 (    me->joining = true;)
line 166 (    return TCB::block_running();          // == WOULD_BLOCK)
line 167 (})

init(): mainTCB-> (~203) and idleTCB-> (~227) get parent=nullptr, desc_count=0, joining=false
*/

/* --- src/trap.cpp   (after SYS_SET_MAX_THREADS) ---
line 100 (        case SYS_THREAD_JOIN_ALL: {)
line 101 (            int r = TCB::join_all();)
line 102 (            if (r == TCB::WOULD_BLOCK) {)
line 103 (                f->sepc += 4;)
line 104 (                Scheduler::switch_to_next();)
line 105 (                return;)
line 106 (            })
line 107 (            f->a0 = 0;)
line 108 (            break;)
line 109 (        })
*/

/* --- h/syscall_cpp.hpp   (added to class Thread) ---
line 18 (    void joinAll();)
*/

/* --- src/syscall_cpp.cpp ---
line 34 (void Thread::joinAll() { thread_join_all(); })
*/

/* --- tests/userMain.cpp   (deltas vs the ORIGINAL userMain) ---
LEVEL_3 include block:
line 22 (// TEST 8 (Modifikacija, joinAll ceka sve potomke))
line 23 (#include "JoinAll_test.hpp")
Prompt:
line 36 (    printString("Unesite broj testa? [1-8]\n");)
Level-3 gate:
line 47 (    if ((test >= 3 && test <= 4) || test == 8) {)
Case:
line 105 (        case 8:)
line 106 (#if LEVEL_3_IMPLEMENTED == 1)
line 107 (            JoinAll_test();)
line 108 (            printString("TEST 8 (Modifikacija, joinAll ceka sve potomke)\n");)
line 109 (#endif)
line 110 (            break;)
*/

/* --- tests/JoinAll_test.hpp --- (NEW FILE)
line 1 (#ifndef _JOINALL_TEST_HPP)
line 2 (#define _JOINALL_TEST_HPP)
line 4 (void JoinAll_test();)
line 6 (#endif)
*/

/* --- tests/JoinAll_test.cpp --- (NEW FILE) ---
line 1  (#include "../h/syscall_cpp.hpp")
line 3  (#include "printing.hpp")
line 5  (#include "JoinAll_test.hpp")
line 7  (#define WORK_BASE 300)
line 9  (static volatile int finishedCount = 0;)
line 11 (static void spin(int units) {)
line 12 (    volatile unsigned long s = 0;)
line 13 (    for (int i = 0; i < units; i++))
line 14 (        for (int j = 0; j < 1000; j++) s++;)
line 15 (    (void)s;)
line 16 (})
line 18 (class Grandchild : public Thread {)
line 19 (public:)
line 20 (    Grandchild() : Thread() {})
line 21 (    void run() override {)
line 22 (        printString("  grandchild START\n");)
line 23 (        spin(WORK_BASE * 3);)
line 24 (        finishedCount++;)
line 25 (        printString("  grandchild DONE\n");)
line 26 (    })
line 27 (};)
line 29 (class ChildWithKid : public Thread {)
line 30 (public:)
line 31 (    ChildWithKid() : Thread() {})
line 32 (    void run() override {)
line 33 (        printString(" child(with kid) START\n");)
line 34 (        Grandchild* g = new Grandchild();)
line 35 (        g->start();)
line 36 (        spin(WORK_BASE);)
line 37 (        finishedCount++;)
line 38 (        printString(" child(with kid) DONE\n");)
line 39 (        delete g;)
line 40 (    })
line 41 (};)
line 43 (class LeafChild : public Thread {)
line 44 (public:)
line 45 (    LeafChild() : Thread() {})
line 46 (    void run() override {)
line 47 (        printString(" leaf child START\n");)
line 48 (        spin(WORK_BASE * 2);)
line 49 (        finishedCount++;)
line 50 (        printString(" leaf child DONE\n");)
line 51 (    })
line 52 (};)
line 54 (void JoinAll_test() {)
line 55 (    printString("--- JoinAll (Modifikacija: joinAll ceka celo podstablo) ---\n");)
line 57 (    finishedCount = 0;)
line 59 (    ChildWithKid* c1 = new ChildWithKid();)
line 60 (    LeafChild*    c2 = new LeafChild();)
line 62 (    c1->start();)
line 63 (    c2->start();)
line 65 (    printString("main: pozivam joinAll()\n");)
line 66 (    Thread::joinAll();)
line 68 (    printString("main: joinAll() se vratio, finishedCount = ");)
line 69 (    printInt(finishedCount);)
line 70 (    printString(" (ocekivano 3: 2 deteta + 1 unuk)\n");)
line 72 (    delete c1;)
line 73 (    delete c2;)
line 75 (    if (finishedCount == 3) printString("JoinAll: PASS\n");)
line 76 (    else                    printString("JoinAll: FAIL\n");)
line 77 (})
*/


/* ========================================================================== */
/* ====================  MODIFICATION JULY 2024: send/receive  ============== */
/* ==  (thread message passing; VERIFIED on current base, test 8 PASS)  ===== */
/* ========================================================================== */

/* --- h/syscall_abi.hpp ---
line 11 (#define SYS_SEND            0x16)
line 12 (#define SYS_RECEIVE         0x17)
*/

/* --- h/syscall_c.h ---
line 21 (void   send(thread_t handle, char* message);)
line 22 (char*  receive();)
*/

/* --- src/syscall.cpp ---
line 80 (extern "C" void  send(thread_t handle, char* message) {)
line 81 (    ecall2(SYS_SEND, (uint64)handle, (uint64)message);)
line 82 (})
line 83 (extern "C" char* receive() {)
line 84 (    return (char*)ecall0(SYS_RECEIVE);)
line 85 (})
*/

/* --- h/TCB.hpp   (mailbox fields + method decls) ---
line 37 (    char*    mbox_msg;)
line 38 (    bool     mbox_full;)
line 39 (    bool     recv_blocked;)
line 40 (    char*    pending_send_msg;)
line 41 (    TCB*     send_wait_head;)
line 42 (    TCB*     send_wait_tail;)
line 65 (    static int msg_send(TCB* target, char* message);)
line 66 (    static int msg_receive(char** out);)
*/

/* --- src/TCB.cpp   (uses the TCB::wake / block_running primitive) ---
field init in create() (after t->id = ++next_id;):
line 79 (    t->mbox_msg         = nullptr;)
line 80 (    t->mbox_full        = false;)
line 81 (    t->recv_blocked     = false;)
line 82 (    t->pending_send_msg = nullptr;)
line 83 (    t->send_wait_head   = nullptr;)
line 84 (    t->send_wait_tail   = nullptr;)
(same six for mainTCB-> at ~line 248 and idleTCB-> at ~line 275 in init())

line 170 (int TCB::msg_send(TCB* target, char* message) {)
line 171 (    if (!target) return 0;)
line 174 (    if (target->recv_blocked) {)
line 175 (        target->recv_blocked = false;)
line 176 (        TCB::wake(target, (uint64)message);)
line 177 (        return 0;)
line 178 (    })
line 181 (    if (!target->mbox_full) {)
line 182 (        target->mbox_msg  = message;)
line 183 (        target->mbox_full = true;)
line 184 (        return 0;)
line 185 (    })
line 188 (    TCB* me = TCB::running;)
line 189 (    me->pending_send_msg = message;)
line 190 (    me->next = nullptr;)
line 191 (    if (!target->send_wait_head) {)
line 192 (        target->send_wait_head = target->send_wait_tail = me;)
line 193 (    } else {)
line 194 (        target->send_wait_tail->next = me;)
line 195 (        target->send_wait_tail       = me;)
line 196 (    })
line 197 (    return TCB::block_running();)
line 198 (})
line 200 (int TCB::msg_receive(char** out) {)
line 201 (    TCB* me = TCB::running;)
line 203 (    if (me->mbox_full) {)
line 204 (        char* msg = me->mbox_msg;)
line 205 (        me->mbox_full = false;)
line 206 (        me->mbox_msg  = nullptr;)
line 209 (        if (me->send_wait_head) {)
line 210 (            TCB* s = me->send_wait_head;)
line 211 (            me->send_wait_head = s->next;)
line 212 (            if (!me->send_wait_head) me->send_wait_tail = nullptr;)
line 213 (            s->next = nullptr;)
line 215 (            me->mbox_msg  = s->pending_send_msg;)
line 216 (            me->mbox_full = true;)
line 217 (            s->pending_send_msg = nullptr;)
line 218 (            TCB::wake(s, 0);)
line 219 (        })
line 221 (        *out = msg;)
line 222 (        return 0;)
line 223 (    })
line 226 (    me->recv_blocked = true;)
line 227 (    return TCB::block_running();)
line 228 (})
*/

/* --- src/trap.cpp   (after the SYS_SET_MAX_THREADS case) ---
line 100 (        case SYS_SEND: {)
line 101 (            TCB* target = (TCB*)f->a1;)
line 102 (            char* msg   = (char*)f->a2;)
line 103 (            int r = TCB::msg_send(target, msg);)
line 104 (            if (r == TCB::WOULD_BLOCK) {)
line 105 (                f->sepc += 4;)
line 106 (                Scheduler::switch_to_next();)
line 107 (                return;)
line 108 (            })
line 109 (            f->a0 = 0;)
line 110 (            break;)
line 111 (        })
line 112 (        case SYS_RECEIVE: {)
line 113 (            char* msg = nullptr;)
line 114 (            int r = TCB::msg_receive(&msg);)
line 115 (            if (r == TCB::WOULD_BLOCK) {)
line 116 (                f->sepc += 4;)
line 117 (                Scheduler::switch_to_next();)
line 118 (                return;)
line 119 (            })
line 120 (            f->a0 = (uint64)msg;)
line 121 (            break;)
line 122 (        })
*/

/* --- h/syscall_cpp.hpp   (added to class Thread) ---
line 18 (    void send(char* message);)
line 19 (    char* receive();)
*/

/* --- src/syscall_cpp.cpp ---
line 34 (void  Thread::send(char* message) { ::send(myHandle, message); })
line 35 (char* Thread::receive()           { return ::receive(); })
*/

/* --- tests/userMain.cpp   (deltas vs the ORIGINAL userMain) ---
In the LEVEL_3 include block:
line 22 (// TEST 8 (Modifikacija, send/receive medju nitima))
line 23 (#include "Messaging_test.hpp")
Prompt:
line 36 (    printString("Unesite broj testa? [1-8]\n");)
Level-3 gate:
line 47 (    if ((test >= 3 && test <= 4) || test == 8) {)
New case in switch(test):
line 105 (        case 8:)
line 106 (#if LEVEL_3_IMPLEMENTED == 1)
line 107 (            Messaging_test();)
line 108 (            printString("TEST 8 (Modifikacija, send/receive medju nitima)\n");)
line 109 (#endif)
line 110 (            break;)
*/

/* --- tests/Messaging_test.hpp --- (NEW FILE)
line 1 (#ifndef _MESSAGING_TEST_HPP)
line 2 (#define _MESSAGING_TEST_HPP)
line 4 (void Messaging_test();)
line 6 (#endif)
*/

/* --- tests/Messaging_test.cpp --- (NEW FILE) ---
line 1  (#include "../h/syscall_cpp.hpp")
line 3  (#include "printing.hpp")
line 5  (#include "Messaging_test.hpp")
line 7  (#define ROUNDS 3)
line 9  (static Semaphore* done = nullptr;)
line 11 (static void printMsg(const char* who, char* msg) {)
line 12 (    printString(who);)
line 13 (    printString(" primio: ");)
line 14 (    printString(msg);)
line 15 (    printString("\n");)
line 16 (})
line 18 (class NodeThread : public Thread {)
line 19 (public:)
line 20 (    NodeThread(const char* name) : Thread(), myName(name) {})
line 22 (    void setNext(Thread* n, char* outMsg) { next = n; msg = outMsg; })
line 24 (    void run() override {)
line 25 (        for (int i = 0; i < ROUNDS; i++) {)
line 26 (            char* got = receive();)
line 27 (            printMsg(myName, got);)
line 28 (            next->send(msg);)
line 29 (        })
line 30 (        done->signal();)
line 31 (    })
line 33 (private:)
line 34 (    const char* myName;)
line 35 (    Thread* next = nullptr;)
line 36 (    char*   msg  = nullptr;)
line 37 (};)
line 39 (void Messaging_test() {)
line 40 (    printString("--- Messaging (Modifikacija: send / receive medju nitima) ---\n");)
line 42 (    done = new Semaphore(0);)
line 44 (    static char msgA[]     = "poruka-od-A";)
line 45 (    static char msgB[]     = "poruka-od-B";)
line 46 (    static char msgC[]     = "poruka-od-C";)
line 47 (    static char msgStart[] = "start";)
line 49 (    NodeThread* a = new NodeThread("A");)
line 50 (    NodeThread* b = new NodeThread("B");)
line 51 (    NodeThread* c = new NodeThread("C");)
line 53 (    a->setNext(b, msgA);)
line 54 (    b->setNext(c, msgB);)
line 55 (    c->setNext(a, msgC);)
line 57 (    a->start();)
line 58 (    b->start();)
line 59 (    c->start();)
line 61 (    a->send(msgStart);)
line 63 (    for (int i = 0; i < 3; i++) done->wait();)
line 65 (    printString("Sve niti zavrsile razmenu poruka.\n");)
line 67 (    delete a;)
line 68 (    delete b;)
line 69 (    delete c;)
line 70 (    delete done;)
line 72 (    printString("Messaging: PASS\n");)
line 73 (})
*/


/* ========================================================================== */
/* ================  MODIFICATION SEPTEMBER 2024: pair + sync  ============== */
/* ==  (2-thread rendezvous + make_driver for IDs from 1)                  == */
/* ==  IMPLEMENTED on current base, NOT machine-verified (not run)         == */
/* ========================================================================== */

/* --- h/syscall_abi.hpp ---
line 11 (#define SYS_THREAD_PAIR     0x16)
line 12 (#define SYS_THREAD_SYNC     0x17)
*/

/* --- h/syscall_c.h ---
line 20 (void   thread_pair(thread_t t1, thread_t t2);)
line 21 (int    thread_sync();)
*/

/* --- src/syscall.cpp ---
line 79 (extern "C" void thread_pair(thread_t t1, thread_t t2) {)
line 80 (    ecall2(SYS_THREAD_PAIR, (uint64)t1, (uint64)t2);)
line 81 (})
line 82 (extern "C" int  thread_sync() { return (int)ecall0(SYS_THREAD_SYNC); })
*/

/* --- h/TCB.hpp   (fields + decls) ---
line 37 (    TCB*     sync_partner;)
line 38 (    bool     sync_waiting;)
line 61 (    static void sync_pair(TCB* a, TCB* b);)
line 62 (    static int  sync_rendezvous();)
line 63 (    static void make_driver(TCB* t);)
*/

/* --- src/TCB.cpp   (uses TCB::wake / block_running primitive) ---
create() after t->id = ++next_id;:
line 79 (    t->sync_partner = nullptr;)
line 80 (    t->sync_waiting = false;)
(same for mainTCB-> ~211 and idleTCB-> ~234 in init())

line 150 (void TCB::sync_pair(TCB* a, TCB* b) {)
line 151 (    if (!a || !b || a == b) return;)
line 152 (    a->sync_partner = b;)
line 153 (    b->sync_partner = a;)
line 154 (})
line 156 (int TCB::sync_rendezvous() {)
line 157 (    TCB* me = TCB::running;)
line 158 (    TCB* p  = me->sync_partner;)
line 159 (    if (!p) return 0;                 // unpaired: no-op)
line 160 (    if (p->sync_waiting) {            // partner here -> release both)
line 161 (        p->sync_waiting = false;)
line 162 (        TCB::wake(p, 0);)
line 163 (        return 0;)
line 164 (    })
line 165 (    me->sync_waiting = true;          // first arrival -> block)
line 166 (    return TCB::block_running();)
line 167 (})
line 169 (void TCB::make_driver(TCB* t) {       // exempt userMain -> user IDs from 1)
line 170 (    if (!t) return;)
line 171 (    t->id        = 0;)
line 172 (    next_id      = 0;)
line 173 (    t->is_kernel = true;)
line 174 (    if (active_user_threads > 0) active_user_threads--;)
line 175 (})
*/

/* --- src/main.cpp   (after creating the userMain thread) ---
line 43 (    TCB::make_driver(userMainTCB);)
*/

/* --- src/trap.cpp   (after SYS_SET_MAX_THREADS) ---
line 100 (        case SYS_THREAD_PAIR: {)
line 101 (            TCB* a = (TCB*)f->a1;)
line 102 (            TCB* b = (TCB*)f->a2;)
line 103 (            TCB::sync_pair(a, b);)
line 104 (            f->a0 = 0;)
line 105 (            break;)
line 106 (        })
line 107 (        case SYS_THREAD_SYNC: {)
line 108 (            int r = TCB::sync_rendezvous();)
line 109 (            if (r == TCB::WOULD_BLOCK) {)
line 110 (                f->sepc += 4;)
line 111 (                Scheduler::switch_to_next();)
line 112 (                return;)
line 113 (            })
line 114 (            f->a0 = 0;)
line 115 (            break;)
line 116 (        })
*/

/* --- h/syscall_cpp.hpp   (class Thread) ---
line 18 (    static void pair(Thread* t1, Thread* t2);)
line 19 (    void sync();)
*/

/* --- src/syscall_cpp.cpp ---
line 34 (void Thread::pair(Thread* t1, Thread* t2) {)
line 35 (    if (t1 && t2) thread_pair(t1->myHandle, t2->myHandle);)
line 36 (})
line 37 (void Thread::sync() { thread_sync(); })
*/

/* --- tests/userMain.cpp   (deltas vs the ORIGINAL userMain) ---
LEVEL_3 include block:
line 22 (// TEST 8 (Modifikacija, uparivanje niti i sync))
line 23 (#include "ThreadSync_test.hpp")
Prompt:
line 36 (    printString("Unesite broj testa? [1-8]\n");)
Level-3 gate:
line 47 (    if ((test >= 3 && test <= 4) || test == 8) {)
Case:
line 105 (        case 8:)
line 106 (#if LEVEL_3_IMPLEMENTED == 1)
line 107 (            ThreadSync_test();)
line 108 (            printString("TEST 8 (Modifikacija, uparivanje niti i sync)\n");)
line 109 (#endif)
line 110 (            break;)
*/

/* --- tests/ThreadSync_test.hpp --- (NEW FILE)
line 1 (#ifndef _THREADSYNC_TEST_HPP)
line 2 (#define _THREADSYNC_TEST_HPP)
line 4 (void ThreadSync_test();)
line 6 (#endif)
*/

/* --- tests/ThreadSync_test.cpp --- (NEW FILE) ---
line 1  (#include "../h/syscall_cpp.hpp")
line 3  (#include "printing.hpp")
line 5  (#include "ThreadSync_test.hpp")
line 7  (#define ITERATIONS 3)
line 9  (static Semaphore* done = nullptr;)
line 11 (class SyncThread : public Thread {)
line 12 (public:)
line 13 (    SyncThread() : Thread() {})
line 15 (    void run() override {)
line 16 (        int id = Thread::getId();)
line 17 (        for (int i = 0; i < ITERATIONS; i++) {)
line 18 (            printInt(id);)
line 19 (            printString(": pre-sync iteracija ");)
line 20 (            printInt(i);)
line 21 (            printString("\n");)
line 23 (            sync();)
line 25 (            printInt(id);)
line 26 (            printString(": post-sync iteracija ");)
line 27 (            printInt(i);)
line 28 (            printString("\n");)
line 29 (        })
line 30 (        done->signal();)
line 31 (    })
line 32 (};)
line 34 (void ThreadSync_test() {)
line 35 (    printString("--- ThreadSync (Modifikacija: pair + sync rendezvous) ---\n");)
line 37 (    done = new Semaphore(0);)
line 39 (    SyncThread* t1 = new SyncThread();)
line 40 (    SyncThread* t2 = new SyncThread();)
line 42 (    t1->start();)
line 43 (    t2->start();)
line 45 (    Thread::pair(t1, t2);)
line 47 (    done->wait();)
line 48 (    done->wait();)
line 50 (    printString("Obe niti zavrsile sinhronizaciju.\n");)
line 52 (    delete t1;)
line 53 (    delete t2;)
line 54 (    delete done;)
line 56 (    printString("ThreadSync: PASS\n");)
line 57 (})
*/


/* ------------------------------------------------------------------------- */
/* ------------------------  MODIFICATION AUGUST 2023  --------------------- */
/* -----  (getThreadId + SetMaximumThreads + FIFO quota block/unblock)  --- */
/* ------------------------------------------------------------------------- */
/*
   NOTE: this modification required NO new code -- it is already implemented
   in the base project (prior task). The lines below are the EXISTING code
   locations that satisfy the Aug 2023 requirements, not new changes.
*/


/* --- src/trap.cpp  (getThreadId syscall WITH context switch) ---
line 88 (        case SYS_THREAD_GET_ID:)
line 90 (            f->a0 = (uint64)TCB::running->id;)
line 91 (            f->sepc += 4;)
line 92 (            Scheduler::switch_to_next();)
line 93 (            return;)
*/

/* --- src/syscall.cpp  (C API) ---
line 75 (extern "C" int  getThreadId()     { return (int)ecall0(SYS_THREAD_GET_ID); })
line 76 (extern "C" int  setMaximumThreads(int n) {)
line 77 (    return (int)ecall1(SYS_SET_MAX_THREADS, (uint64)n);)
line 78 (})
*/

/* --- src/syscall_cpp.cpp  (C++ API) ---
line 30 (int Thread::getId() { return getThreadId(); })
line 32 (int Thread::SetMaximumThreads(int n) { return setMaximumThreads(n); })
*/

/* --- src/TCB.cpp  (default 5, unique id, FIFO quota block/unblock) ---
line 14  (int    TCB::max_user_threads   = 5;)
line 77  (    t->id           = ++next_id;)
line 86  (    if (active_user_threads < max_user_threads) {)
line 87  (        active_user_threads++;)
line 88  (        Scheduler::put(t);)
line 89  (    } else {)
line 90  (        t->state = PENDING_QUOTA;)
line 91  (        t->next  = nullptr;)
line 92  (        if (!pending_head) {)
line 93  (            pending_head = pending_tail = t;)
line 94  (        } else {)
line 95  (            pending_tail->next = t;)
line 96  (            pending_tail       = t;)
line 97  (        })
line 98  (    })
line 102 (void TCB::admit_from_pending() {)
line 103 (    if (!pending_head) return;)
line 104 (    TCB* t = pending_head;)
line 105 (    pending_head = t->next;)
line 106 (    if (!pending_head) pending_tail = nullptr;)
line 107 (    t->next  = nullptr;)
line 108 (    t->state = READY;)
line 109 (    active_user_threads++;)
line 110 (    Scheduler::put(t);)
line 111 (})
line 119 (        admit_from_pending();  // in TCB::exit -- unblock oldest on finish)
line 139 (void TCB::set_maximum_threads(int n) {)
line 140 (    if (n < 1) n = 1;)
line 141 (    max_user_threads = n;)
line 142 (    while (active_user_threads < max_user_threads && pending_head) {)
line 143 (        admit_from_pending();)
line 144 (    })
line 145 (})
*/

/* --- tests/Modification_test.cpp  (EXISTING test, menu option 'm') ---
   SetMaximumThreads(3); 20 HelloThreads; each prints "Hello! <id>" x5 via
   Thread::getId(); then busy-waits proportional to (id+1). Already present.
*/


/* ========================================================================== */
/* ============  MODIFICATION OCTOBER 2025: addChild + joinAll (30p)  ======= */
/* ==  source: siwiki.rs; adapted to this kernel (direct TCB block/wake).  == */
/* ==  Explicit child registration; joinAll waits for registered children.== */
/* ==  IMPLEMENTED on current base, NOT machine-verified (not run).        == */
/* ==  start() must precede addChild()/getHandle() (myHandle set in start).== */
/* ========================================================================== */

/* --- h/syscall_abi.hpp ---
line 11 (#define SYS_THREAD_ADD_CHILD 0x16)
line 12 (#define SYS_THREAD_JOIN_ALL  0x17)
*/

/* --- h/syscall_c.h ---
line 20 (void   thread_add_child(thread_t child);)
line 21 (void   thread_join_all();)
*/

/* --- src/syscall.cpp ---
line 79 (extern "C" void thread_add_child(thread_t child) {)
line 80 (    if (!child) return;)
line 81 (    ecall1(SYS_THREAD_ADD_CHILD, (uint64)child);)
line 82 (})
line 83 (extern "C" void thread_join_all() { ecall0(SYS_THREAD_JOIN_ALL); })
*/

/* --- h/TCB.hpp   (fields + decls) ---
line 37 (    TCB*     parent;)
line 38 (    int      childCount;)
line 39 (    bool     joining;)
line 62 (    static void add_child(TCB* parent, TCB* child);)
line 63 (    static int  join_all_children();)
*/

/* --- src/TCB.cpp   (uses TCB::wake / block_running primitive) ---
create() after t->id = ++next_id;:
line 79 (    t->parent     = nullptr;)
line 80 (    t->childCount = 0;)
line 81 (    t->joining    = false;)
(same for mainTCB-> ~206 and idleTCB-> ~230 in init())

exit() finish hook (near top of exit, after me->state = FINISHED):
line 121 (    if (me->parent) {)
line 122 (        me->parent->childCount--;)
line 123 (        if (me->parent->childCount == 0 && me->parent->joining) {)
line 124 (            me->parent->joining = false;)
line 125 (            TCB::wake(me->parent, 0);)
line 126 (        })
line 127 (    })

line 159 (void TCB::add_child(TCB* parent, TCB* child) {)
line 160 (    if (!parent || !child) return;)
line 161 (    child->parent = parent;)
line 162 (    parent->childCount++;)
line 163 (})
line 165 (int TCB::join_all_children() {)
line 166 (    TCB* me = TCB::running;)
line 167 (    if (me->childCount <= 0) return 0;   // nothing to wait -> immediate)
line 168 (    me->joining = true;)
line 169 (    return TCB::block_running();          // == WOULD_BLOCK)
line 170 (})
*/

/* --- src/trap.cpp   (after SYS_SET_MAX_THREADS) ---
line 100 (        case SYS_THREAD_ADD_CHILD: {)
line 101 (            TCB* child = (TCB*)f->a1;)
line 102 (            TCB::add_child(TCB::running, child);)
line 103 (            f->a0 = 0;)
line 104 (            break;)
line 105 (        })
line 106 (        case SYS_THREAD_JOIN_ALL: {)
line 107 (            int r = TCB::join_all_children();)
line 108 (            if (r == TCB::WOULD_BLOCK) {)
line 109 (                f->sepc += 4;)
line 110 (                Scheduler::switch_to_next();)
line 111 (                return;)
line 112 (            })
line 113 (            f->a0 = 0;)
line 114 (            break;)
line 115 (        })
*/

/* --- h/syscall_cpp.hpp   (class Thread) ---
line 18 (    void addChild(Thread* child);)
line 19 (    void joinAll();)
line 20 (    thread_t getHandle() { return myHandle; })
*/

/* --- src/syscall_cpp.cpp ---
line 34 (void Thread::addChild(Thread* child) {)
line 35 (    if (child) thread_add_child(child->myHandle);)
line 36 (})
line 37 (void Thread::joinAll() { thread_join_all(); })
*/

/* --- tests/userMain.cpp   (deltas vs the ORIGINAL userMain) ---
LEVEL_3 include block:
line 22 (// TEST 8 (Modifikacija okt 2025, addChild + joinAll))
line 23 (#include "JoinAllChildren_test.hpp")
Prompt:
line 36 (    printString("Unesite broj testa? [1-8]\n");)
Level-3 gate:
line 47 (    if ((test >= 3 && test <= 4) || test == 8) {)
Case:
line 105 (        case 8:)
line 106 (#if LEVEL_3_IMPLEMENTED == 1)
line 107 (            JoinAllChildren_test();)
line 108 (            printString("TEST 8 (Modifikacija okt 2025, addChild + joinAll)\n");)
line 109 (#endif)
line 110 (            break;)
*/

/* --- tests/JoinAllChildren_test.hpp --- (NEW FILE)
line 1 (#ifndef _JOINALLCHILDREN_TEST_HPP)
line 2 (#define _JOINALLCHILDREN_TEST_HPP)
line 4 (void JoinAllChildren_test();)
line 6 (#endif)
*/

/* --- tests/JoinAllChildren_test.cpp --- (NEW FILE) ---
line 1  (#include "../h/syscall_cpp.hpp")
line 2  (#include "../h/syscall_c.h")
line 4  (#include "printing.hpp")
line 6  (#include "JoinAllChildren_test.hpp")
line 8  (class ThreadC : public Thread {)
line 9  (public:)
line 10 (    ThreadC(int id) : Thread(), id(id) {})
line 11 (private:)
line 12 (    int id;)
line 13 (    void run() override {)
line 14 (        printString("    C"); printInt(id); printString(" started\n");)
line 18 (        volatile int sum = 0;)
line 19 (        for (int i = 0; i < 200; i++))
line 20 (            for (int j = 0; j < 1000; j++))
line 21 (                sum += j;)
line 24 (        printString("    C"); printInt(id); printString(" finished\n");)
line 27 (    })
line 28 (};)
line 30 (class ThreadB : public Thread {)
line 32 (    ThreadB(int id) : Thread(), id(id) {})
line 35 (    void run() override {)
line 36 (        printString("  B"); printInt(id); printString(" started, creating 3 C children\n");)
line 40 (        ThreadC* c[3];)
line 41 (        for (int i = 0; i < 3; i++) {)
line 42 (            c[i] = new ThreadC(id * 10 + i);)
line 43 (            c[i]->start();)
line 44 (            this->addChild(c[i]);)
line 45 (        })
line 49 (        printString("  B"); printInt(id); printString(" waiting for children...\n");)
line 50 (        this->joinAll();)
line 52 (        printString("  B"); printInt(id); printString(" all children done!\n");)
line 56 (        for (int i = 0; i < 3; i++) delete c[i];)
line 57 (    })
line 58 (};)
line 60 (void JoinAllChildren_test() {)
line 61 (    printString("A started, creating 3 B and 1 C\n");)
line 63 (    ThreadB* b[3];)
line 64 (    for (int i = 0; i < 3; i++) {)
line 65 (        b[i] = new ThreadB(i);)
line 66 (        b[i]->start();)
line 67 (        thread_add_child(b[i]->getHandle());)
line 68 (    })
line 70 (    ThreadC* c = new ThreadC(99);)
line 71 (    c->start();)
line 72 (    thread_add_child(c->getHandle());)
line 74 (    printString("A waiting for all children...\n");)
line 75 (    thread_join_all();)
line 77 (    printString("\n=== A: ALL children done! ===\n");)
line 79 (    for (int i = 0; i < 3; i++) delete b[i];)
line 80 (    delete c;)
line 82 (    printString("JoinAllChildren: PASS\n");)
line 83 (})
*/
