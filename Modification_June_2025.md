# Modification June 2025 — Parallel matrix histogram — 20p

## Task (original, Serbian)

Dinamički alocirati matricu `mat` celih brojeva (`int`), dimenzija M×N, gde se M i N
učitavaju sa standardnog ulaza. Matricu nakon alociranja popuniti pseudoslučajnim
vrednostima koristeći dati generator:

```c
static unsigned long int next = 1;
int custom_rand(void) {
    next = next * 1103515245 + 12345;
    return (unsigned int)(next / 65536) % 32768;
}
void custom_srand(unsigned int seed) {
    next = seed;
}
```

Napraviti histogram `H[10]` broja pojavljivanja vrednosti unutar matrice po modulu 10
(`mat[i][j] % 10`) za sve elemente. Pravljenje histograma odraditi kreiranjem **M
uporednih niti** gde svaka nit računa lokalni histogram za jednu vrstu, a nakon toga
taj histogram spaja sa **deljenim** histogramom u kojem će se nalaziti finalne
vrednosti za celu matricu. Glavna nit treba da ispiše konačni histogram kada sve niti
završe obradu.

**20 poena:** Tokom izvršavanja niti, nakon svakih 10 obrađenih elemenata pozvati
`dispatch`.

> Note: the original text had OCR typos in the RNG (`next next 1103515245...`). The
> corrected standard LCG above is what is implemented — it must be reproduced verbatim
> so expected values match.

---

## Design decisions

### D1 — Shared histogram + mutex

M threads each do `sharedH[k] += localH[k]` for k = 0..9. Even though the kernel is
cooperative (threads yield only at `dispatch`), the 10-slot merge is a read-modify-write;
if a `dispatch` fired mid-merge another thread could interleave. To be correct and
universal, the merge is guarded by a **binary semaphore** `histMutex` (init 1):
`wait()` … merge 10 slots … `signal()`. Each thread builds its **local** histogram
lock-free (private array) and locks only briefly for the merge — matching the spec's
"spaja sa deljenim histogramom".

### D2 — Join before print

`done` semaphore (init 0): each thread `signal()`s as its last act; main `wait()`s M
times before printing. Guarantees "glavna nit ispisuje ... kada sve niti završe".

### D3 — One thread per row

`RowThread` subclasses `Thread` (C++ API); constructor takes the row index. `run()`
reads only `mat[rowIndex][0..N-1]` — no RNG calls in threads, so the matrix content is
fixed before any thread starts (determinism).

### D4 — The 20p dispatch rule

A per-thread counter over processed elements: `if (++processed % 10 == 0)
Thread::dispatch();`. Placed in the element-counting loop (the "obrađeni elementi"),
NOT inside the mutex critical section — dispatching while holding the lock would make
other threads spin.

### D5 — RNG determinism (64-bit)

`next` is `unsigned long int`. On the target ABI (`-mabi=lp64`) that is **64-bit**, so
the LCG arithmetic wraps mod 2^64. The matrix is filled in main with `custom_srand(1)`
before threads start, in row-major order, so values are fully deterministic.

### D6 — Allocation

`int** mat = new int*[M]; mat[i] = new int[N];` (global `operator new[]`/`delete[]`
are provided in `cpp_runtime.cpp`). Freed symmetrically after join.

### D7 — Reading M and N

`getString(buf, size)` + `stringToInt(buf)` from `printing.hpp`, with prompts.

---

## Files changed / added

| File | Change |
|------|--------|
| `tests/MatrixHistogram_test.{cpp,hpp}` | **new** — the entire feature (RNG, matrix, M threads, mutex merge, join, print). No kernel changes needed. |
| `tests/userMain.cpp` | menu line `9`, digit branch widened to `'9'`, level-3 gate for test 9, `case 9:` |

This modification is **entirely test-side**: it uses only the existing Thread and
Semaphore APIs, so unlike the pairSems modification there are no changes under `h/`
or `src/`.

---

## Expected output (deterministic, seed = 1, 64-bit `next`)

The RNG is seeded with `custom_srand(1)` and the matrix is filled row-major, so for a
given M, N the values are fixed. Values are `custom_rand() % 10`.

### Example: M = 3, N = 5

Matrix (and `% 10`):
```
row 0: 16838  5758 10113 17515 31051   -> 8 8 3 5 1
row 1:  5627 23010  7419 16212  4086   -> 7 0 9 2 6
row 2:  2749 12767  9084 12060 32225   -> 9 7 4 0 5
```
Output:
```
--- MatrixHistogram (Modifikacija: paralelni histogram) ---
Unesite M (broj vrsta): 3
Unesite N (broj kolona): 5
H[0] = 2
H[1] = 1
H[2] = 1
H[3] = 1
H[4] = 1
H[5] = 2
H[6] = 1
H[7] = 2
H[8] = 2
H[9] = 2
Ukupno elemenata: 15 (ocekivano M*N = 15)
MatrixHistogram: PASS
TEST 9 (Modifikacija, paralelni histogram matrice)
```

### Example: M = 2, N = 3

```
row 0: 16838  5758 10113   -> 8 8 3
row 1: 17515 31051  5627   -> 5 1 7
```
```
H[0] = 0
H[1] = 1
H[2] = 0
H[3] = 1
H[4] = 0
H[5] = 1
H[6] = 0
H[7] = 1
H[8] = 2
H[9] = 0
Ukupno elemenata: 6 (ocekivano M*N = 6)
MatrixHistogram: PASS
```

**Key invariant (order-independent):** `sum(H[0..9]) == M*N`. The histogram totals do
not depend on thread scheduling — only the interleaving of the merge does, and the
mutex makes that safe. So the printed H values are identical on every run for a given
M, N.

> If the target's `unsigned long` were 32-bit instead of 64-bit, the LCG would wrap
> differently and the exact H values would change (but the invariant sum = M*N still
> holds). This project builds lp64, so the 64-bit values above apply.

---

## Edge cases handled

1. `M <= 0` or `N <= 0` → prints an error and returns (no allocation, no threads).
2. `custom_rand()` returns 0..32767 (non-negative), so `% 10` is always 0..9 — no
   negative-modulo issue, histogram indices always valid.
3. Merge race → `histMutex` (binary semaphore) serializes the 10-slot merge.
4. `dispatch` placed outside the critical section (in the element loop only).
5. Join before free → main `done->wait()`s M times before deleting `mat`/histogram.
6. `M > max_user_threads` (quota = 5): extra threads queue in `PENDING_QUOTA` and run
   as slots free; correctness unaffected, only interleaving changes. (Call
   `Thread::SetMaximumThreads(M)` first if you want all rows running concurrently.)
7. RNG determinism: matrix filled in main before threads start; threads never call
   `custom_rand`.

---

## Build / verify (NOT compiled on this host)

No `make`/RISC-V toolchain on this Windows machine — manual-review only. To verify:

```
make
make qemu       # choose option 9, then type M and N
```

Menu option: `9  MatrixHistogram (Modifikacija: paralelni histogram)`. Gated behind
`LEVEL_3_IMPLEMENTED == 1`. The expected H values above were computed with an
independent Python reimplementation of the same 64-bit LCG.
