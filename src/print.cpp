/*
	ctrl+a + x
   ============================================================================
   print.cpp - per-file change snapshots for all OS1 modifications (reference)
   ============================================================================
   HOW TO READ THIS FILE:
   - Comment-only. It records, per modification, which file each change goes in
     and the code, as a copy-paste source for re-applying a modification.
   - Line numbers are ANCHORS, not exact. They were captured one-modification-
     at-a-time against a clean base; once modifications stack (or the base
     changes) they shift. Locate code by the quoted anchor line, not by N.
   - These snapshots assume the shared block/wake primitive is present in the
     base kernel: TCB::block_running(), TCB::wake(TCB*, uint64),
     TCB::FRAME_A0_IDX / TCB::WOULD_BLOCK, and class WaitQueue (h/TCB.hpp,
     src/TCB.cpp). See MODIFICATIONS_GUIDE.md for the pattern and the 7-layer
     syscall scaffold. Sync modifications call TCB::wake instead of rolling
     their own wake helper.
   ============================================================================
*/

/* --- h/syscall_abi.hpp ---
line 18 (#define SYS_SEM_PAIR        0x27)
*/

/* --- h/syscall_c.h ---
line 27 (int    sem_pair(sem_t a, sem_t b);)
*/

/* --- h/syscall_cpp.hpp ---
line 35 (    static void pairSems(Semaphore& sem1, Semaphore& sem2);)
*/

/* --- h/Semaphore.hpp ---
line 13 (    // Internal wait() return codes:)
line 14 (    //   0             passed on the called semaphore (standard))
line 15 (    //   PASSED_PAIRED passed on a paired semaphore  -> sem_wait returns 1)
line 16 (    //   1             must block on the called semaphore (thread switched away))
line 17 (    //   E_CLOSED/...  error)
line 18 (    static const int PASSED_PAIRED = 2;)
line 20 (    static const int MAX_PARTNERS = 16;)
line 33 (    // Pair two semaphores symmetrically (idempotent, self-pairing ignored).)
line 34 (    static void pair(KSemaphore* a, KSemaphore* b);)
line 42 (    KSemaphore* partners[MAX_PARTNERS];)
line 43 (    int         nPartners;)
line 45 (    void  addPartner(KSemaphore* other);)
*/

/* --- src/Semaphore.cpp ---
   (pairSems adds partner list + 3-phase wait to KSemaphore. Blocking/waking
   still goes through KSemaphore's own wait/signal + KSemaphore::wake, which in
   the post-primitive base is a one-line delegate to TCB::wake -- no change
   needed in this section.)
line 13 (      nPartners(0) {)
line 14 (    for (int i = 0; i < MAX_PARTNERS; i++) partners[i] = nullptr;)
line 15 (})
line 51 (void KSemaphore::addPartner(KSemaphore* other) {)
line 52 (    if (!other || other == this) return;      // no self-pairing)
line 53 (    for (int i = 0; i < nPartners; i++) {)
line 54 (        if (partners[i] == other) return;      // already paired (idempotent))
line 55 (    })
line 56 (    if (nPartners < MAX_PARTNERS) {)
line 57 (        partners[nPartners++] = other;)
line 58 (    })
line 59 (})
line 61 (void KSemaphore::pair(KSemaphore* a, KSemaphore* b) {)
line 62 (    if (!a || !b || a == b) return;)
line 63 (    // Symmetric: whichever is waited on must see the other as a partner.)
line 64 (    a->addPartner(b);)
line 65 (    b->addPartner(a);)
line 66 (})
line 72 (    // Phase 1: try to pass on the called semaphore itself (standard).)
line 78 (    // Phase 2: only if paired, try to pass on some paired semaphore.)
line 79 (    for (int i = 0; i < nPartners; i++) {)
line 80 (        KSemaphore* p = partners[i];)
line 81 (        if (!p || p->closed) continue;         // closed partner cannot grant passage)
line 82 (        if ((int)n <= p->value) {)
line 83 (            p->value -= (int)n;                // update the partner in the standard way)
line 84 (            return PASSED_PAIRED;              // sem_wait must return 1)
line 85 (        })
line 86 (    })
line 88 (    // Phase 3: block on the called semaphore (standard).)
*/

/* --- src/syscall.cpp ---
line 98  (extern "C" int sem_pair(sem_t a, sem_t b) {)
line 99  (    return (int)ecall2(SYS_SEM_PAIR, (uint64)a, (uint64)b);)
line 100 (})
*/

/* --- src/syscall_cpp.cpp ---
line 45 (void Semaphore::pairSems(Semaphore& sem1, Semaphore& sem2) {)
line 46 (    sem_pair(sem1.myHandle, sem2.myHandle);)
line 47 (})
*/

/* --- src/trap.cpp ---
line 135 (            if (r == KSemaphore::PASSED_PAIRED) {)
line 136 (                // Passed on a paired semaphore: thread did NOT block,)
line 137 (                // wait returns 1 to the caller.)
line 138 (                f->a0 = 1;)
line 139 (                break;)
line 140 (            })
line 155 (        case SYS_SEM_PAIR: {)
line 156 (            KSemaphore* a = (KSemaphore*)f->a1;)
line 157 (            KSemaphore* b = (KSemaphore*)f->a2;)
line 158 (            if (!a || !b) { f->a0 = (uint64)-1; break; })
line 159 (            KSemaphore::pair(a, b);)
line 160 (            f->a0 = 0;)
line 161 (            break;)
line 162 (        })
*/

/* --- tests/userMain.cpp   (deltas vs the ORIGINAL userMain: prompt "[1-7]",
   int test = getc()-'0'; getc();  switch(test){ case 1..7; default }) ---
   Add near the other test includes:
     #include "SemPair_test.hpp"
   Optional level gate (test 8 is a zadatak-3 feature) alongside the existing
   `if (test >= 3 && test <= 4) { ...LEVEL_3... }`:
     if ((test >= 3 && test <= 4) || test == 8) { ...LEVEL_3 check... }
   Add a case to the switch (single digit 8 selects fine via getc()-'0'):
     case 8:
     #if LEVEL_3_IMPLEMENTED == 1
                 SemPair_test();
                 printString("TEST 8 (Modifikacija, uparivanje semafora pairSems)\n");
     #endif
                 break;
   (No menu-string / print_menu edits needed: the original has no menu list,
   just the "[1-7]" prompt. Update the prompt text to [1-8] if desired.)
*/

/* --- tests/SemPair_test.hpp --- (NEW FILE, entirely for this modification)
line 1 (#ifndef _SEMPAIR_TEST_HPP)
line 2 (#define _SEMPAIR_TEST_HPP)
line 4 (void SemPair_test();)
line 6 (#endif)
*/

/* --- tests/SemPair_test.cpp --- (NEW FILE, entirely for this modification)
line 1  (#include "../h/syscall_cpp.hpp")
line 2  (#include "printing.hpp")
line 3  (#include "SemPair_test.hpp")
line 18 (#define NUM_WORKERS 5)
line 19 (#define ITERATIONS  5)
line 20 (#define DISPATCH_ITERS 1000)
line 22 (static Semaphore* mainSem;)
line 23 (static Semaphore* done;)
line 25 (static volatile int pairedPassTotal;   // ukupno prolazaka na uparenim semaforima)
line 27 (class WorkerThread : public Thread {)
line 28 (public:)
line 29 (    WorkerThread(Semaphore* ownSem) : Thread(), sem(ownSem) {})
line 31 (    void run() override {)
line 32 (        int id = Thread::getId();)
line 33 (        for (int num = 1; num <= ITERATIONS; num++) {)
line 34 (            printInt(id);)
line 35 (            printString(" wait iteracija ");)
line 36 (            printInt(num);)
line 37 (            printString("!\n");)
line 39 (            int r = sem->wait();)
line 41 (            if (r == 1) {)
line 42 (                // prosla na uparenom semaforu)
line 43 (                printInt(id);)
line 44 (                printString(" prosla semafor-iteracija ");)
line 45 (                printInt(num);)
line 46 (                printString("!\n");)
line 47 (                pairedPassTotal++;)
line 48 (            } else {)
line 49 (                // prosla na svom (pozvanom) semaforu)
line 50 (                printInt(id);)
line 51 (                printString(" prosla semafor iteracija ");)
line 52 (                printInt(num);)
line 53 (                printString("!\n");)
line 54 (            })
line 56 (            // 20p: radi u 1000 iteracija dispatch)
line 57 (            for (int i = 0; i < DISPATCH_ITERS; i++) {)
line 58 (                Thread::dispatch();)
line 59 (            })
line 60 (        })
line 61 (        done->signal();)
line 62 (    })
line 64 (private:)
line 65 (    Semaphore* sem;)
line 66 (};)
line 68 (void SemPair_test() {)
line 69 (    printString("--- SemPair (Modifikacija: pairSems) ---\n");)
line 71 (    pairedPassTotal = 0;)
line 73 (    mainSem = new Semaphore(100);)
line 74 (    done    = new Semaphore(0);)
line 76 (    Semaphore* workers[NUM_WORKERS];)
line 77 (    for (int i = 0; i < NUM_WORKERS; i++) {)
line 78 (        workers[i] = new Semaphore((unsigned)(i + 1));   // 1,2,3,4,5)
line 79 (        Semaphore::pairSems(*mainSem, *workers[i]);       // upari glavni sa svakim)
line 80 (    })
line 82 (    WorkerThread* threads[NUM_WORKERS];)
line 83 (    for (int i = 0; i < NUM_WORKERS; i++) {)
line 84 (        threads[i] = new WorkerThread(workers[i]);)
line 85 (        threads[i]->start();)
line 86 (    })
line 89 (    for (int i = 0; i < NUM_WORKERS; i++) {)
line 90 (        done->wait();)
line 91 (    })
line 93 (    printString("Ukupno prolazaka na uparenim semaforima: ");)
line 94 (    printInt(pairedPassTotal);)
line 95 (    printString("\n");)
line 97 (    for (int i = 0; i < NUM_WORKERS; i++) delete threads[i];)
line 98 (    for (int i = 0; i < NUM_WORKERS; i++) delete workers[i];)
line 99 (    delete done;)
line 100 (    delete mainSem;)
line 102 (    printString("SemPair: PASS\n");)
line 103 (})
*/


/* ------------------------------------------------------------------------- */
/* ------------------------  MODIFICATION JUNE 2025  ------------------------ */
/* ---------------------  (parallel matrix histogram)  --------------------- */
/* ------------------------------------------------------------------------- */


/* --- src/cpp_runtime.cpp ---
line 13 (extern "C" void __cxa_throw_bad_array_new_length() { for (;;) {} })
*/

/* --- tests/userMain.cpp   (deltas vs the ORIGINAL userMain) ---
   Add include:  #include "MatrixHistogram_test.hpp"
   Optional L3 gate: if ((test >= 3 && test <= 4) || test == 9) { ...LEVEL_3... }
   Add case (single digit 9 selects via getc()-'0'):
     case 9:
     #if LEVEL_3_IMPLEMENTED == 1
                 MatrixHistogram_test();
                 printString("TEST 9 (Modifikacija, paralelni histogram matrice)\n");
     #endif
                 break;
*/

/* --- tests/MatrixHistogram_test.hpp --- (NEW FILE, entirely for this modification)
line 1 (#ifndef _MATRIXHISTOGRAM_TEST_HPP)
line 2 (#define _MATRIXHISTOGRAM_TEST_HPP)
line 4 (void MatrixHistogram_test();)
line 6 (#endif)
*/

/* --- tests/MatrixHistogram_test.cpp --- (NEW FILE, entirely for this modification)
line 1   (#include "../h/syscall_cpp.hpp")
line 2   (#include "printing.hpp")
line 3   (#include "MatrixHistogram_test.hpp")
line 5   (#define HIST_SIZE 10)
line 6   (#define DISPATCH_EVERY 10)
line 8   (static unsigned long int next = 1;)
line 10  (static int custom_rand(void) {)
line 11  (    next = next * 1103515245 + 12345;)
line 12  (    return (unsigned int)(next / 65536) % 32768;)
line 13  (})
line 15  (static void custom_srand(unsigned int seed) {)
line 16  (    next = seed;)
line 17  (})
line 19  (static int** mat = nullptr;)
line 20  (static int   M = 0;)
line 21  (static int   N = 0;)
line 23  (static int sharedH[HIST_SIZE];)
line 25  (static Semaphore* histMutex = nullptr;)
line 26  (static Semaphore* done      = nullptr;)
line 28  (class RowThread : public Thread {)
line 29  (public:)
line 30  (    RowThread(int row) : Thread(), rowIndex(row) {})
line 32  (    void run() override {)
line 33  (        int localH[HIST_SIZE];)
line 34  (        for (int k = 0; k < HIST_SIZE; k++) localH[k] = 0;)
line 36  (        int* row = mat[rowIndex];)
line 37  (        int processed = 0;)
line 39  (        for (int j = 0; j < N; j++) {)
line 40  (            int digit = row[j] % HIST_SIZE;)
line 41  (            localH[digit]++;)
line 43  (            if (++processed % DISPATCH_EVERY == 0) {)
line 44  (                Thread::dispatch();)
line 45  (            })
line 46  (        })
line 48  (        histMutex->wait();)
line 49  (        for (int k = 0; k < HIST_SIZE; k++) {)
line 50  (            sharedH[k] += localH[k];)
line 51  (        })
line 52  (        histMutex->signal();)
line 54  (        done->signal();)
line 55  (    })
line 57  (private:)
line 58  (    int rowIndex;)
line 59  (};)
line 61  (static int readInt(const char* prompt) {)
line 62  (    char buf[32];)
line 63  (    printString(prompt);)
line 64  (    getString(buf, sizeof(buf));)
line 65  (    int len = 0;)
line 66  (    while (buf[len] != '\0') len++;)
line 67  (    printString(buf);)
line 68  (    if (len == 0 || (buf[len - 1] != '\n' && buf[len - 1] != '\r')) {)
line 69  (        printString("\n");)
line 70  (    })
line 71  (    return stringToInt(buf);)
line 72  (})
line 74  (void MatrixHistogram_test() {)
line 75  (    printString("--- MatrixHistogram (Modifikacija: paralelni histogram) ---\n");)
line 77  (    M = readInt("Unesite M (broj vrsta): ");)
line 78  (    N = readInt("Unesite N (broj kolona): ");)
line 80  (    if (M <= 0 || N <= 0) {)
line 81  (        printString("M i N moraju biti pozitivni.\n");)
line 82  (        return;)
line 83  (    })
line 85  (    mat = new int*[M];)
line 86  (    for (int i = 0; i < M; i++) mat[i] = new int[N];)
line 88  (    custom_srand(1);)
line 89  (    for (int i = 0; i < M; i++))
line 90  (        for (int j = 0; j < N; j++))
line 91  (            mat[i][j] = custom_rand();)
line 93  (    for (int k = 0; k < HIST_SIZE; k++) sharedH[k] = 0;)
line 94  (    histMutex = new Semaphore(1);)
line 95  (    done      = new Semaphore(0);)
line 97  (    RowThread** threads = new RowThread*[M];)
line 98  (    for (int i = 0; i < M; i++) {)
line 99  (        threads[i] = new RowThread(i);)
line 100 (        threads[i]->start();)
line 101 (    })
line 103 (    for (int i = 0; i < M; i++) done->wait();)
line 105 (    long total = 0;)
line 106 (    for (int k = 0; k < HIST_SIZE; k++) {)
line 107 (        printString("H[");)
line 108 (        printInt(k);)
line 109 (        printString("] = ");)
line 110 (        printInt(sharedH[k]);)
line 111 (        printString("\n");)
line 112 (        total += sharedH[k];)
line 113 (    })
line 115 (    printString("Ukupno elemenata: ");)
line 116 (    printInt((int)total);)
line 117 (    printString(" (ocekivano M*N = ");)
line 118 (    printInt(M * N);)
line 119 (    printString(")\n");)
line 121 (    for (int i = 0; i < M; i++) delete threads[i];)
line 122 (    delete[] threads;)
line 123 (    delete done;)
line 124 (    delete histMutex;)
line 125 (    for (int i = 0; i < M; i++) delete[] mat[i];)
line 126 (    delete[] mat;)
line 127 (    mat = nullptr;)
line 129 (    if (total == (long)M * N) printString("MatrixHistogram: PASS\n");)
line 130 (    else                      printString("MatrixHistogram: FAIL\n");)
line 131 (})
*/


/* ------------------------------------------------------------------------- */
/* ------------------------  MODIFICATION JUNE 2024  ------------------------ */
/* -----------------  (Thread::joinAll - wait all descendants)  ------------ */
/* ------------------------------------------------------------------------- */


/* --- h/TCB.hpp ---
line 29 (    TCB*     parent;)
line 30 (    size_t   desc_count;)
line 31 (    bool     joining;)
line 46 (    static int join_all();)
*/

/* --- src/TCB.cpp ---
line 79  (    t->parent       = TCB::running;)
line 80  (    t->desc_count   = 0;)
line 81  (    t->joining      = false;)
line 83  (    for (TCB* a = t->parent; a != nullptr; a = a->parent) {)
line 84  (        a->desc_count++;)
line 85  (    })
line 125 (    for (TCB* a = me->parent; a != nullptr; a = a->parent) {)
line 126 (        if (a->desc_count > 0) a->desc_count--;)
line 127 (        if (a->joining && a->desc_count == 0) {)
line 128 (            a->joining = false;)
line 129 (            TCB::wake(a, 0);      // primitive (was: state=READY; Scheduler::put))
line 130 (        })
line 131 (    })
line 145 (int TCB::join_all() {)
line 146 (    TCB* me = TCB::running;)
line 148 (    if (me->desc_count == 0) return 0;   // nothing to wait -> immediate (a0=0))
line 150 (    me->joining = true;)
line 152 (    return TCB::block_running();          // == WOULD_BLOCK -> trap switches away)
line 153 (})
   NOTE: with the primitive, "blocked" is TCB::WOULD_BLOCK and "immediate" is 0.
   The trap.cpp case must test `r == TCB::WOULD_BLOCK` to decide to switch away
   (see the canonical idiom in MODIFICATIONS_GUIDE.md), not `r == 0`.
line 192 (    mainTCB->parent       = nullptr;)
line 193 (    mainTCB->desc_count   = 0;)
line 194 (    mainTCB->joining      = false;)
line 216 (    idleTCB->parent       = nullptr;)
line 217 (    idleTCB->desc_count   = 0;)
line 218 (    idleTCB->joining      = false;)
*/

/* --- h/syscall_abi.hpp ---
line 11 (#define SYS_THREAD_JOIN_ALL 0x16)
*/

/* --- h/syscall_c.h ---
line 20 (int    thread_join_all();)
*/

/* --- src/syscall.cpp ---
line 79 (extern "C" int  thread_join_all() { return (int)ecall0(SYS_THREAD_JOIN_ALL); })
*/

/* --- src/trap.cpp ---
line 100 (        case SYS_THREAD_JOIN_ALL: {)
line 101 (            int r = TCB::join_all();)
line 102 (            if (r == TCB::WOULD_BLOCK) {   // blocked -> switch away)
line 103 (                f->sepc += 4;)
line 104 (                Scheduler::switch_to_next();)
line 105 (                return;)
line 106 (            })
line 107 (            f->a0 = 0;)
line 108 (            break;)
line 109 (        })
*/

/* --- h/syscall_cpp.hpp ---
line 18 (    static void joinAll();)
*/

/* --- src/syscall_cpp.cpp ---
line 34 (void Thread::joinAll() { thread_join_all(); })
*/

/* --- tests/userMain.cpp   (deltas vs the ORIGINAL userMain) ---
   Add include:  #include "JoinAll_test.hpp"

   NOTE: test 10 is TWO digits. The original reads one char:
       int test = getc() - '0';  getc();
   which cannot select "10". To use a two-digit number, replace that read with
   a line read (requires getString/stringToInt from printing.hpp):
       char line[16];
       getString(line, sizeof(line));
       int test = stringToInt(line);
   (Alternatively, give the test a single-digit number and keep the original read.)

   Optional L3 gate: if ((test >= 3 && test <= 4) || test == 10) { ...LEVEL_3... }
   Add case:
     case 10:
     #if LEVEL_3_IMPLEMENTED == 1
                 JoinAll_test();
                 printString("TEST 10 (Modifikacija, joinAll ceka sve potomke)\n");
     #endif
                 break;
*/

/* --- tests/JoinAll_test.hpp --- (NEW FILE, entirely for this modification)
line 1 (#ifndef _JOINALL_TEST_HPP)
line 2 (#define _JOINALL_TEST_HPP)
line 4 (void JoinAll_test();)
line 6 (#endif)
*/

/* --- tests/JoinAll_test.cpp --- (NEW FILE, entirely for this modification)
line 1  (#include "../h/syscall_cpp.hpp")
line 2  (#include "printing.hpp")
line 3  (#include "JoinAll_test.hpp")
line 5  (#define WORK_BASE 300)
line 7  (static volatile int finishedCount = 0;)
line 9  (static void spin(int units) {)
line 10 (    volatile unsigned long s = 0;)
line 11 (    for (int i = 0; i < units; i++))
line 12 (        for (int j = 0; j < 1000; j++) s++;)
line 13 (    (void)s;)
line 14 (})
line 16 (class Grandchild : public Thread {)
line 17 (public:)
line 18 (    Grandchild() : Thread() {})
line 19 (    void run() override {)
line 20 (        printString("  grandchild START\n");)
line 21 (        spin(WORK_BASE * 3);)
line 22 (        finishedCount++;)
line 23 (        printString("  grandchild DONE\n");)
line 24 (    })
line 25 (};)
line 27 (class ChildWithKid : public Thread {)
line 28 (public:)
line 29 (    ChildWithKid() : Thread() {})
line 30 (    void run() override {)
line 31 (        printString(" child(with kid) START\n");)
line 32 (        Grandchild* g = new Grandchild();)
line 33 (        g->start();)
line 34 (        spin(WORK_BASE);)
line 35 (        finishedCount++;)
line 36 (        printString(" child(with kid) DONE\n");)
line 37 (        delete g;)
line 38 (    })
line 39 (};)
line 41 (class LeafChild : public Thread {)
line 42 (public:)
line 43 (    LeafChild() : Thread() {})
line 44 (    void run() override {)
line 45 (        printString(" leaf child START\n");)
line 46 (        spin(WORK_BASE * 2);)
line 47 (        finishedCount++;)
line 48 (        printString(" leaf child DONE\n");)
line 49 (    })
line 50 (};)
line 52 (void JoinAll_test() {)
line 53 (    printString("--- JoinAll (Modifikacija: joinAll ceka celo podstablo) ---\n");)
line 55 (    finishedCount = 0;)
line 57 (    ChildWithKid* c1 = new ChildWithKid();)
line 58 (    LeafChild*    c2 = new LeafChild();)
line 60 (    c1->start();)
line 61 (    c2->start();)
line 63 (    printString("main: pozivam joinAll()\n");)
line 64 (    Thread::joinAll();)
line 66 (    printString("main: joinAll() se vratio, finishedCount = ");)
line 67 (    printInt(finishedCount);)
line 68 (    printString(" (ocekivano 3: 2 deteta + 1 unuk)\n");)
line 70 (    delete c1;)
line 71 (    delete c2;)
line 73 (    if (finishedCount == 3) printString("JoinAll: PASS\n");)
line 74 (    else                    printString("JoinAll: FAIL\n");)
line 75 (})
*/


/* ------------------------------------------------------------------------- */
/* ------------------------  MODIFICATION JULY 2024  ------------------------ */
/* -------------  (thread message passing: send / receive)  --------------- */
/* ------------------------------------------------------------------------- */


/* --- h/TCB.hpp ---
line 29 (    char*    mbox_msg;)
line 30 (    bool     mbox_full;)
line 31 (    bool     recv_blocked;)
line 32 (    char*    recv_result;)
line 33 (    char*    pending_send_msg;)
line 34 (    TCB*     send_wait_head;)
line 35 (    TCB*     send_wait_tail;)
line 50 (    static int msg_send(TCB* target, char* message);)
line 51 (    static int msg_receive(char** out);)
*/

/* --- src/TCB.cpp   (line numbers are anchors; uses TCB::wake primitive) ---
   Per-thread mailbox state initialized in create() and init():
     t->mbox_msg = nullptr; t->mbox_full = false; t->recv_blocked = false;
     t->recv_result = nullptr; t->pending_send_msg = nullptr;
     t->send_wait_head = nullptr; t->send_wait_tail = nullptr;
   (do the same for mainTCB-> and idleTCB-> in init())

   NO local wake helper -- use TCB::wake from the primitive.

int TCB::msg_send(TCB* target, char* message) {
    if (!target) return 0;

    if (target->recv_blocked) {                 // receiver waiting -> handoff
        target->recv_result  = message;
        target->recv_blocked = false;
        TCB::wake(target, (uint64)message);     // primitive
        return 0;
    }

    if (!target->mbox_full) {                    // slot free -> deposit
        target->mbox_msg  = message;
        target->mbox_full = true;
        return 0;
    }

    TCB* me = TCB::running;                       // slot full -> block sender
    me->pending_send_msg = message;
    me->next = nullptr;
    if (!target->send_wait_head) {
        target->send_wait_head = target->send_wait_tail = me;
    } else {
        target->send_wait_tail->next = me;
        target->send_wait_tail       = me;
    }
    return TCB::block_running();                  // primitive (== WOULD_BLOCK)
}

int TCB::msg_receive(char** out) {
    TCB* me = TCB::running;

    if (me->mbox_full) {
        char* msg = me->mbox_msg;
        me->mbox_full = false;
        me->mbox_msg  = nullptr;

        if (me->send_wait_head) {                 // let one blocked sender deposit
            TCB* s = me->send_wait_head;
            me->send_wait_head = s->next;
            if (!me->send_wait_head) me->send_wait_tail = nullptr;
            s->next = nullptr;

            me->mbox_msg  = s->pending_send_msg;
            me->mbox_full = true;
            s->pending_send_msg = nullptr;
            TCB::wake(s, 0);                       // primitive
        }

        *out = msg;
        return 0;
    }

    me->recv_blocked = true;
    return TCB::block_running();                   // primitive
}

   NOTE: send_wait_head/tail can be replaced by a WaitQueue member; kept as raw
   pointers here to match the reverted snapshot. Either is fine.
*/

/* --- src/TCB.cpp  (mailbox field init, mainTCB) ---
line 242 (    mainTCB->mbox_msg         = nullptr;)
line 243 (    mainTCB->mbox_full        = false;)
line 244 (    mainTCB->recv_blocked     = false;)
line 245 (    mainTCB->recv_result      = nullptr;)
line 246 (    mainTCB->pending_send_msg = nullptr;)
line 247 (    mainTCB->send_wait_head   = nullptr;)
line 248 (    mainTCB->send_wait_tail   = nullptr;)
line 270 (    idleTCB->mbox_msg         = nullptr;)
line 271 (    idleTCB->mbox_full        = false;)
line 272 (    idleTCB->recv_blocked     = false;)
line 273 (    idleTCB->recv_result      = nullptr;)
line 274 (    idleTCB->pending_send_msg = nullptr;)
line 275 (    idleTCB->send_wait_head   = nullptr;)
line 276 (    idleTCB->send_wait_tail   = nullptr;)
*/

/* --- h/syscall_abi.hpp ---
line 11 (#define SYS_SEND            0x17)
line 12 (#define SYS_RECEIVE         0x18)
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

/* --- src/trap.cpp ---
line 100 (        case SYS_SEND: {)
line 101 (            TCB* target = (TCB*)f->a1;)
line 102 (            char* msg   = (char*)f->a2;)
line 103 (            int r = TCB::msg_send(target, msg);)
line 104 (            if (r == TCB::WOULD_BLOCK) {   // sender blocked -> switch away)
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
line 115 (            if (r == TCB::WOULD_BLOCK) {   // receiver blocked -> switch away)
line 116 (                f->sepc += 4;)
line 117 (                Scheduler::switch_to_next();)
line 118 (                return;)
line 119 (            })
line 120 (            f->a0 = (uint64)msg;)
line 121 (            break;)
line 122 (        })
*/

/* --- h/syscall_cpp.hpp ---
line 18 (    void send(char* message);)
line 19 (    char* receive();)
*/

/* --- src/syscall_cpp.cpp ---
line 34 (void  Thread::send(char* message) { ::send(myHandle, message); })
line 35 (char* Thread::receive()           { return ::receive(); })
*/

/* --- tests/userMain.cpp   (deltas vs the ORIGINAL userMain) ---
   Add include:  #include "Messaging_test.hpp"

   NOTE: test 11 is TWO digits -> the original single-char read
   (int test = getc()-'0'; getc();) cannot select it. Switch to a line read:
       char line[16]; getString(line, sizeof(line)); int test = stringToInt(line);
   (or assign a single-digit number and keep the original read).

   Optional L3 gate: if ((test >= 3 && test <= 4) || test == 11) { ...LEVEL_3... }
   Add case:
     case 11:
     #if LEVEL_3_IMPLEMENTED == 1
                 Messaging_test();
                 printString("TEST 11 (Modifikacija, send/receive medju nitima)\n");
     #endif
                 break;
*/

/* --- tests/Messaging_test.hpp --- (NEW FILE, entirely for this modification)
line 1 (#ifndef _MESSAGING_TEST_HPP)
line 2 (#define _MESSAGING_TEST_HPP)
line 4 (void Messaging_test();)
line 6 (#endif)
*/

/* --- tests/Messaging_test.cpp --- (NEW FILE, entirely for this modification)
line 1  (#include "../h/syscall_cpp.hpp")
line 2  (#include "printing.hpp")
line 3  (#include "Messaging_test.hpp")
line 5  (#define ROUNDS 3)
line 7  (static Thread* tA = nullptr;)
line 8  (static Thread* tB = nullptr;)
line 9  (static Thread* tC = nullptr;)
line 10 (static Semaphore* done = nullptr;)
line 12 (static void printMsg(const char* who, char* msg) {)
line 13 (    printString(who);)
line 14 (    printString(" primio: ");)
line 15 (    printString(msg);)
line 16 (    printString("\n");)
line 17 (})
line 19 (class NodeThread : public Thread {)
line 20 (public:)
line 21 (    NodeThread(const char* name) : Thread(), myName(name) {})
line 23 (    void setNext(Thread* n, char* outMsg) { next = n; msg = outMsg; })
line 25 (    void run() override {)
line 26 (        for (int i = 0; i < ROUNDS; i++) {)
line 27 (            char* got = receive();)
line 28 (            printMsg(myName, got);)
line 29 (            next->send(msg);)
line 30 (        })
line 31 (        done->signal();)
line 32 (    })
line 34 (private:)
line 35 (    const char* myName;)
line 36 (    Thread* next = nullptr;)
line 37 (    char*   msg  = nullptr;)
line 38 (};)
line 40 (void Messaging_test() {)
line 41 (    printString("--- Messaging (Modifikacija: send / receive medju nitima) ---\n");)
line 43 (    done = new Semaphore(0);)
line 45 (    static char msgA[] = "poruka-od-A";)
line 46 (    static char msgB[] = "poruka-od-B";)
line 47 (    static char msgC[] = "poruka-od-C";)
line 48 (    static char msgStart[] = "start";)
line 50 (    NodeThread* a = new NodeThread("A");)
line 51 (    NodeThread* b = new NodeThread("B");)
line 52 (    NodeThread* c = new NodeThread("C");)
line 53 (    tA = a; tB = b; tC = c;)
line 55 (    a->setNext(b, msgA);)
line 56 (    b->setNext(c, msgB);)
line 57 (    c->setNext(a, msgC);)
line 59 (    a->start();)
line 60 (    b->start();)
line 61 (    c->start();)
line 63 (    a->send(msgStart);)
line 65 (    for (int i = 0; i < 3; i++) done->wait();)
line 67 (    printString("Sve niti zavrsile razmenu poruka.\n");)
line 69 (    delete a;)
line 70 (    delete b;)
line 71 (    delete c;)
line 72 (    delete done;)
line 74 (    printString("Messaging: PASS\n");)
line 75 (})
*/


/* ------------------------------------------------------------------------- */
/* ----------------------  MODIFICATION SEPTEMBER 2024  -------------------- */
/* --------------  (thread pairing + sync() rendezvous)  ------------------ */
/* ------------------------------------------------------------------------- */


/* --- h/TCB.hpp ---
line 29 (    TCB*     sync_partner;)
line 30 (    bool     sync_waiting;)
line 45 (    static void sync_pair(TCB* a, TCB* b);)
line 46 (    static int  sync_rendezvous();)
line 48 (    static void make_driver(TCB* t);)
*/

/* --- src/TCB.cpp ---
line 79  (    t->sync_partner = nullptr;)
line 80  (    t->sync_waiting = false;)

   NO local sync_wake helper -- use TCB::wake from the primitive.

void TCB::sync_pair(TCB* a, TCB* b) {
    if (!a || !b || a == b) return;
    a->sync_partner = b;
    b->sync_partner = a;
}

int TCB::sync_rendezvous() {
    TCB* me = TCB::running;
    TCB* p  = me->sync_partner;

    if (!p) return 0;                  // unpaired: no-op, don't block

    if (p->sync_waiting) {             // partner already here -> release both
        p->sync_waiting = false;
        TCB::wake(p, 0);               // primitive
        return 0;                      // caller does not block
    }

    me->sync_waiting = true;           // first arrival -> block
    return TCB::block_running();       // == WOULD_BLOCK -> trap switches away
}

void TCB::make_driver(TCB* t) {
    if (!t) return;
    t->id        = 0;
    next_id      = 0;
    t->is_kernel = true;
    if (active_user_threads > 0) active_user_threads--;
}

   (init mainTCB-> / idleTCB-> sync_partner=nullptr, sync_waiting=false too)
*/

/* --- src/TCB.cpp  (sync field init, mainTCB) ---
line 210 (    mainTCB->sync_partner = nullptr;)
line 211 (    mainTCB->sync_waiting = false;)
line 233 (    idleTCB->sync_partner = nullptr;)
line 234 (    idleTCB->sync_waiting = false;)
*/

/* --- src/main.cpp ---
line 43 (    TCB::make_driver(userMainTCB);)
*/

/* --- h/syscall_abi.hpp ---
line 11 (#define SYS_THREAD_PAIR     0x19)
line 12 (#define SYS_THREAD_SYNC     0x1A)
*/

/* --- h/syscall_c.h ---
line 21 (void   thread_pair(thread_t t1, thread_t t2);)
line 22 (int    thread_sync();)
*/

/* --- src/syscall.cpp ---
line 80 (extern "C" void thread_pair(thread_t t1, thread_t t2) {)
line 81 (    ecall2(SYS_THREAD_PAIR, (uint64)t1, (uint64)t2);)
line 82 (})
line 83 (extern "C" int  thread_sync() { return (int)ecall0(SYS_THREAD_SYNC); })
*/

/* --- src/trap.cpp ---
line 100 (        case SYS_THREAD_PAIR: {)
line 101 (            TCB* a = (TCB*)f->a1;)
line 102 (            TCB* b = (TCB*)f->a2;)
line 103 (            TCB::sync_pair(a, b);)
line 104 (            f->a0 = 0;)
line 105 (            break;)
line 106 (        })
line 107 (        case SYS_THREAD_SYNC: {)
line 108 (            int r = TCB::sync_rendezvous();)
line 109 (            if (r == TCB::WOULD_BLOCK) {   // blocked -> switch away)
line 110 (                f->sepc += 4;)
line 111 (                Scheduler::switch_to_next();)
line 112 (                return;)
line 113 (            })
line 114 (            f->a0 = 0;)
line 115 (            break;)
line 116 (        })
*/

/* --- h/syscall_cpp.hpp ---
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
   Add include:  #include "ThreadSync_test.hpp"
   Optional L3 gate: if ((test >= 3 && test <= 4) || test == 8) { ...LEVEL_3... }
   Add case (single digit 8 selects via getc()-'0'):
     case 8:
     #if LEVEL_3_IMPLEMENTED == 1
                 ThreadSync_test();
                 printString("TEST 8 (Modifikacija, uparivanje niti i sync)\n");
     #endif
                 break;
   NOTE: this modification also requires user thread IDs to start at 1, which is
   done in the KERNEL (TCB::make_driver called from main.cpp), not in userMain.
*/

/* --- tests/ThreadSync_test.hpp --- (NEW FILE, entirely for this modification)
line 1 (#ifndef _THREADSYNC_TEST_HPP)
line 2 (#define _THREADSYNC_TEST_HPP)
line 4 (void ThreadSync_test();)
line 6 (#endif)
*/

/* --- tests/ThreadSync_test.cpp --- (NEW FILE, entirely for this modification)
line 1  (#include "../h/syscall_cpp.hpp")
line 2  (#include "printing.hpp")
line 3  (#include "ThreadSync_test.hpp")
line 5  (#define ITERATIONS 3)
line 7  (static Semaphore* done = nullptr;)
line 9  (class SyncThread : public Thread {)
line 10 (public:)
line 11 (    SyncThread() : Thread() {})
line 13 (    void run() override {)
line 14 (        int id = Thread::getId();)
line 15 (        for (int i = 0; i < ITERATIONS; i++) {)
line 16 (            printInt(id);)
line 17 (            printString(": pre-sync iteracija ");)
line 18 (            printInt(i);)
line 19 (            printString("\n");)
line 21 (            sync();)
line 23 (            printInt(id);)
line 24 (            printString(": post-sync iteracija ");)
line 25 (            printInt(i);)
line 26 (            printString("\n");)
line 27 (        })
line 28 (        done->signal();)
line 29 (    })
line 30 (};)
line 32 (void ThreadSync_test() {)
line 33 (    printString("--- ThreadSync (Modifikacija: pair + sync rendezvous) ---\n");)
line 35 (    done = new Semaphore(0);)
line 37 (    SyncThread* t1 = new SyncThread();)
line 38 (    SyncThread* t2 = new SyncThread();)
line 40 (    Thread::pair(t1, t2);)
line 42 (    t1->start();)
line 43 (    t2->start();)
line 45 (    done->wait();)
line 46 (    done->wait();)
line 48 (    printString("Obe niti zavrsile sinhronizaciju.\n");)
line 50 (    delete t1;)
line 51 (    delete t2;)
line 52 (    delete done;)
line 54 (    printString("ThreadSync: PASS\n");)
line 55 (})
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


/* ------------------------------------------------------------------------- */
/* ------------------------  MODIFICATION OCTOBER 2025  -------------------- */
/* ----------  (addChild + joinAll, explicit child registration, 30p)  ---- */
/* --  source: siwiki.rs, adapted to this kernel (direct TCB block/wake)  - */
/* ------------------------------------------------------------------------- */


/* --- h/TCB.hpp ---
line 29 (    TCB*     parent;)
line 30 (    int      childCount;)
line 31 (    bool     joining;)
line 46 (    static void add_child(TCB* parent, TCB* child);)
line 47 (    static int  join_all_children();)
*/

/* --- src/TCB.cpp ---
line 79  (    t->parent     = nullptr;)
line 80  (    t->childCount = 0;)
line 81  (    t->joining    = false;)
line 121 (    if (me->parent) {                        // in TCB::exit(), finish hook)
line 122 (        me->parent->childCount--;)
line 123 (        if (me->parent->childCount == 0 && me->parent->joining) {)
line 124 (            me->parent->joining = false;)
line 125 (            TCB::wake(me->parent, 0);            // primitive (was inline frame[8]=0 + put))
line 126 (        })
line 127 (    })
line 164 (void TCB::add_child(TCB* parent, TCB* child) {)
line 165 (    if (!parent || !child) return;)
line 166 (    child->parent = parent;)
line 167 (    parent->childCount++;)
line 168 (})
line 170 (int TCB::join_all_children() {)
line 171 (    TCB* me = TCB::running;)
line 172 (    if (me->childCount <= 0) return 0;       // nothing to wait -> immediate)
line 173 (    me->joining = true;)
line 175 (    return TCB::block_running();             // == WOULD_BLOCK -> trap switches away)
line 176 (})
line 196 (    mainTCB->parent     = nullptr;   [+ childCount=0, joining=false])
line 220 (    idleTCB->parent     = nullptr;   [+ childCount=0, joining=false])
*/

/* --- h/syscall_abi.hpp ---
line 11 (#define SYS_THREAD_ADD_CHILD 0x1B)
line 12 (#define SYS_THREAD_JOIN_ALL  0x1C)
*/

/* --- h/syscall_c.h ---
line 21 (void   thread_add_child(thread_t child);)
line 22 (void   thread_join_all();)
*/

/* --- src/syscall.cpp ---
line 80 (extern "C" void thread_add_child(thread_t child) {)
line 81 (    if (!child) return;)
line 82 (    ecall1(SYS_THREAD_ADD_CHILD, (uint64)child);)
line 83 (})
line 84 (extern "C" void thread_join_all() { ecall0(SYS_THREAD_JOIN_ALL); })
*/

/* --- src/trap.cpp ---
line 100 (        case SYS_THREAD_ADD_CHILD: {)
line 101 (            TCB* child = (TCB*)f->a1;)
line 102 (            TCB::add_child(TCB::running, child);)
line 103 (            f->a0 = 0;)
line 104 (            break;)
line 105 (        })
line 106 (        case SYS_THREAD_JOIN_ALL: {)
line 107 (            int r = TCB::join_all_children();)
line 108 (            if (r == TCB::WOULD_BLOCK) {   // blocked -> switch away)
line 109 (                f->sepc += 4;)
line 110 (                Scheduler::switch_to_next();)
line 111 (                return;)
line 112 (            })
line 113 (            f->a0 = 0;)
line 114 (            break;)
line 115 (        })
*/

/* --- h/syscall_cpp.hpp ---
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
   Add include:  #include "JoinAllChildren_test.hpp"

   NOTE: test 12 is TWO digits -> the original single-char read
   (int test = getc()-'0'; getc();) cannot select it. Switch to a line read:
       char line[16]; getString(line, sizeof(line)); int test = stringToInt(line);
   (or assign a single-digit number and keep the original read).

   Optional L3 gate: if ((test >= 3 && test <= 4) || test == 12) { ...LEVEL_3... }
   Add case:
     case 12:
     #if LEVEL_3_IMPLEMENTED == 1
                 JoinAllChildren_test();
                 printString("TEST 12 (Modifikacija okt 2025, addChild + joinAll)\n");
     #endif
                 break;
*/

/* --- tests/JoinAllChildren_test.hpp --- (NEW FILE, entirely for this modification)
line 1 (#ifndef _JOINALLCHILDREN_TEST_HPP)
line 2 (#define _JOINALLCHILDREN_TEST_HPP)
line 4 (void JoinAllChildren_test();)
line 6 (#endif)
*/

/* --- tests/JoinAllChildren_test.cpp --- (NEW FILE, entirely for this modification)
line 1  (#include "../h/syscall_cpp.hpp")
line 2  (#include "../h/syscall_c.h")
line 3  (#include "printing.hpp")
line 4  (#include "JoinAllChildren_test.hpp")
line 6  (class ThreadC : public Thread {)
line 7  (public:)
line 8  (    ThreadC(int id) : Thread(), id(id) {})
line 9  (private:)
line 10 (    int id;)
line 11 (    void run() override {)
line 12 (        printString("    C");)
line 13 (        printInt(id);)
line 14 (        printString(" started\n");)
line 16 (        volatile int sum = 0;)
line 17 (        for (int i = 0; i < 200; i++))
line 18 (            for (int j = 0; j < 1000; j++))
line 19 (                sum += j;)
line 20 (        (void)sum;)
line 22 (        printString("    C");)
line 23 (        printInt(id);)
line 24 (        printString(" finished\n");)
line 25 (    })
line 26 (};)
line 28 (class ThreadB : public Thread {)
line 29 (public:)
line 30 (    ThreadB(int id) : Thread(), id(id) {})
line 31 (private:)
line 32 (    int id;)
line 33 (    void run() override {)
line 34 (        printString("  B");)
line 35 (        printInt(id);)
line 36 (        printString(" started, creating 3 C children\n");)
line 38 (        ThreadC* c[3];)
line 39 (        for (int i = 0; i < 3; i++) {)
line 40 (            c[i] = new ThreadC(id * 10 + i);)
line 41 (            c[i]->start();)
line 42 (            this->addChild(c[i]);)
line 43 (        })
line 45 (        printString("  B");)
line 46 (        printInt(id);)
line 47 (        printString(" waiting for children...\n");)
line 48 (        this->joinAll();)
line 50 (        printString("  B");)
line 51 (        printInt(id);)
line 52 (        printString(" all children done!\n");)
line 54 (        for (int i = 0; i < 3; i++) delete c[i];)
line 55 (    })
line 56 (};)
line 58 (void JoinAllChildren_test() {)
line 59 (    printString("A started, creating 3 B and 1 C\n");)
line 61 (    ThreadB* b[3];)
line 62 (    for (int i = 0; i < 3; i++) {)
line 63 (        b[i] = new ThreadB(i);)
line 64 (        b[i]->start();)
line 65 (        thread_add_child(b[i]->getHandle());)
line 66 (    })
line 68 (    ThreadC* c = new ThreadC(99);)
line 69 (    c->start();)
line 70 (    thread_add_child(c->getHandle());)
line 72 (    printString("A waiting for all children...\n");)
line 73 (    thread_join_all();)
line 75 (    printString("\n=== A: ALL children done! ===\n");)
line 77 (    for (int i = 0; i < 3; i++) delete b[i];)
line 78 (    delete c;)
line 80 (    printString("JoinAllChildren: PASS\n");)
line 81 (})
*/

