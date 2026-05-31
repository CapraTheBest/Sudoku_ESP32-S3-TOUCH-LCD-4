# Sudoku Engine Implementation Plan (Piano A di 2)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Costruire il motore di gioco del Sudoku in C++ puro (solver, generatore a soluzione unica, stato della partita con undo), interamente coperto da test unitari nativi su PC.

**Architecture:** Il motore è una libreria PlatformIO portabile in `lib/sudoku/`, senza alcuna dipendenza da Arduino o LVGL, così da compilare e girare sotto l'ambiente `[env:native]` (Unity) su PC. Il firmware (Piano B) linkerà questa stessa libreria. RNG iniettato come function pointer per avere test deterministici.

**Tech Stack:** C++17, PlatformIO, Unity test framework, platform `native` (gcc host). Nessuna rete, nessun LVGL in questo piano.

---

## File Structure

- `platformio.ini` — solo l'ambiente `[env:native]` in questo piano (il Piano B aggiungerà l'env device).
- `lib/sudoku/sudoku_types.h` — `Difficulty`, costanti (`N`, `CELLS`), tipo `RandFn`.
- `lib/sudoku/sudoku_solver.h` / `.cpp` — `solve()`, `countSolutions()`.
- `lib/sudoku/sudoku_generator.h` / `.cpp` — `generateSolution()`, `generatePuzzle()`.
- `lib/sudoku/sudoku_board.h` / `.cpp` — classe `Board` (stato, undo, conflitti, vittoria).
- `test/test_engine/test_engine.cpp` — tutti i test Unity del motore.

Tutto sotto namespace `sudoku`. La libreria non include header Arduino/LVGL: deve restare compilabile dal compilatore host.

---

## Task 1: Scaffolding ambiente di test nativo

**Files:**
- Create: `platformio.ini`
- Create: `lib/sudoku/sudoku_types.h`
- Create: `test/test_engine/test_engine.cpp`

- [ ] **Step 1: Crea `platformio.ini` con il solo env nativo**

```ini
; =============================================================================
; Sudoku Panel - PlatformIO configuration
; Piano A: solo ambiente di test nativo (engine C++ puro).
; Il Piano B aggiunge [env:esp32-s3-touch-lcd-4] per il device.
; =============================================================================

[platformio]
default_envs = native

[env:native]
platform = native
test_framework = unity
build_flags = -std=gnu++17 -Wall
; Nessun src Arduino da compilare in questo ambiente
build_src_filter = -<*>
```

- [ ] **Step 2: Crea `lib/sudoku/sudoku_types.h`**

```cpp
#pragma once
#include <cstdint>

namespace sudoku {

constexpr int N = 9;        // lato della griglia
constexpr int CELLS = 81;   // numero di celle

enum class Difficulty : uint8_t { Easy = 0, Medium = 1, Hard = 2 };

// RNG iniettabile: restituisce un intero in [0, maxExclusive).
// Permette test deterministici (LCG nei test, esp_random sul device).
using RandFn = uint32_t (*)(uint32_t maxExclusive);

} // namespace sudoku
```

- [ ] **Step 3: Crea `test/test_engine/test_engine.cpp` con un test segnaposto**

```cpp
#include <unity.h>
#include "sudoku_types.h"

void setUp(void) {}
void tearDown(void) {}

void test_scaffolding_builds(void) {
    TEST_ASSERT_EQUAL_INT(81, sudoku::CELLS);
}

int main(int, char **) {
    UNITY_BEGIN();
    RUN_TEST(test_scaffolding_builds);
    return UNITY_END();
}
```

- [ ] **Step 4: Esegui i test per verificare che l'ambiente funzioni**

Run: `pio test -e native`
Expected: compila ed esegue, 1 test PASS (`test_scaffolding_builds`).

- [ ] **Step 5: Commit**

```bash
git add platformio.ini lib/sudoku/sudoku_types.h test/test_engine/test_engine.cpp
git commit -m "build: native test scaffolding + sudoku types"
```

---

## Task 2: Solver — `solve()`

Riempie una griglia parzialmente compilata con una soluzione valida (backtracking).

**Files:**
- Create: `lib/sudoku/sudoku_solver.h`
- Create: `lib/sudoku/sudoku_solver.cpp`
- Modify: `test/test_engine/test_engine.cpp`

- [ ] **Step 1: Scrivi il test che fallisce**

Aggiungi in `test_engine.cpp` (sopra `main`) il puzzle di esempio e il test:

```cpp
#include "sudoku_solver.h"

// Puzzle noto (0 = vuoto) e relativa soluzione unica.
static const uint8_t PUZZLE[81] = {
    5,3,0, 0,7,0, 0,0,0,
    6,0,0, 1,9,5, 0,0,0,
    0,9,8, 0,0,0, 0,6,0,
    8,0,0, 0,6,0, 0,0,3,
    4,0,0, 8,0,3, 0,0,1,
    7,0,0, 0,2,0, 0,0,6,
    0,6,0, 0,0,0, 2,8,0,
    0,0,0, 4,1,9, 0,0,5,
    0,0,0, 0,8,0, 0,7,9
};
static const uint8_t SOLUTION[81] = {
    5,3,4, 6,7,8, 9,1,2,
    6,7,2, 1,9,5, 3,4,8,
    1,9,8, 3,4,2, 5,6,7,
    8,5,9, 7,6,1, 4,2,3,
    4,2,6, 8,5,3, 7,9,1,
    7,1,3, 9,2,4, 8,5,6,
    9,6,1, 5,3,7, 2,8,4,
    2,8,7, 4,1,9, 6,3,5,
    3,4,5, 2,8,6, 1,7,9
};

void test_solve_fills_known_puzzle(void) {
    uint8_t g[81];
    for (int i = 0; i < 81; i++) g[i] = PUZZLE[i];
    TEST_ASSERT_TRUE(sudoku::solve(g));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(SOLUTION, g, 81);
}
```

E registra il test in `main`:

```cpp
    RUN_TEST(test_solve_fills_known_puzzle);
```

- [ ] **Step 2: Esegui il test per verificare che fallisce**

Run: `pio test -e native`
Expected: FAIL di compilazione (`sudoku_solver.h` non esiste / `solve` non dichiarato).

- [ ] **Step 3: Crea `lib/sudoku/sudoku_solver.h`**

```cpp
#pragma once
#include <cstdint>

namespace sudoku {

// Risolve la griglia in place (grid[81], 0 = vuoto). True se trovata soluzione.
bool solve(uint8_t grid[81]);

// Conta le soluzioni fino a `limit` (early-exit). Per l'unicità usare limit=2.
int countSolutions(const uint8_t grid[81], int limit);

} // namespace sudoku
```

- [ ] **Step 4: Crea `lib/sudoku/sudoku_solver.cpp` con `solve()`**

```cpp
#include "sudoku_solver.h"

namespace sudoku {

static bool canPlace(const uint8_t g[81], int idx, uint8_t v) {
    int r = idx / 9, c = idx % 9;
    for (int i = 0; i < 9; i++) {
        if (g[r * 9 + i] == v) return false;
        if (g[i * 9 + c] == v) return false;
    }
    int br = (r / 3) * 3, bc = (c / 3) * 3;
    for (int dr = 0; dr < 3; dr++)
        for (int dc = 0; dc < 3; dc++)
            if (g[(br + dr) * 9 + (bc + dc)] == v) return false;
    return true;
}

static int firstEmpty(const uint8_t g[81]) {
    for (int i = 0; i < 81; i++) if (g[i] == 0) return i;
    return -1;
}

bool solve(uint8_t g[81]) {
    int idx = firstEmpty(g);
    if (idx < 0) return true;
    for (uint8_t v = 1; v <= 9; v++) {
        if (canPlace(g, idx, v)) {
            g[idx] = v;
            if (solve(g)) return true;
            g[idx] = 0;
        }
    }
    return false;
}

// countSolutions definita nel Task 3.

} // namespace sudoku
```

- [ ] **Step 5: Esegui il test per verificare che passa**

Run: `pio test -e native`
Expected: PASS `test_solve_fills_known_puzzle`.

- [ ] **Step 6: Commit**

```bash
git add lib/sudoku/sudoku_solver.h lib/sudoku/sudoku_solver.cpp test/test_engine/test_engine.cpp
git commit -m "feat(engine): backtracking solve()"
```

---

## Task 3: Solver — `countSolutions()`

Serve a garantire l'unicità della soluzione nel generatore.

**Files:**
- Modify: `lib/sudoku/sudoku_solver.cpp`
- Modify: `test/test_engine/test_engine.cpp`

- [ ] **Step 1: Scrivi i test che falliscono**

Aggiungi in `test_engine.cpp`:

```cpp
void test_count_unique_puzzle_is_one(void) {
    TEST_ASSERT_EQUAL_INT(1, sudoku::countSolutions(PUZZLE, 2));
}

void test_count_empty_grid_is_capped_at_limit(void) {
    uint8_t empty[81] = {0};
    // Una griglia vuota ha moltissime soluzioni: l'early-exit deve fermarsi a 2.
    TEST_ASSERT_EQUAL_INT(2, sudoku::countSolutions(empty, 2));
}
```

Registra in `main`:

```cpp
    RUN_TEST(test_count_unique_puzzle_is_one);
    RUN_TEST(test_count_empty_grid_is_capped_at_limit);
```

- [ ] **Step 2: Esegui il test per verificare che fallisce**

Run: `pio test -e native`
Expected: FAIL di link (`countSolutions` non definita).

- [ ] **Step 3: Implementa `countSolutions()` in `sudoku_solver.cpp`**

Sostituisci il commento `// countSolutions definita nel Task 3.` con:

```cpp
static void countRec(uint8_t g[81], int limit, int &count) {
    if (count >= limit) return;
    int idx = firstEmpty(g);
    if (idx < 0) { count++; return; }
    for (uint8_t v = 1; v <= 9 && count < limit; v++) {
        if (canPlace(g, idx, v)) {
            g[idx] = v;
            countRec(g, limit, count);
            g[idx] = 0;
        }
    }
}

int countSolutions(const uint8_t grid[81], int limit) {
    uint8_t g[81];
    for (int i = 0; i < 81; i++) g[i] = grid[i];
    int count = 0;
    countRec(g, limit, count);
    return count;
}
```

- [ ] **Step 4: Esegui il test per verificare che passa**

Run: `pio test -e native`
Expected: PASS dei due nuovi test.

- [ ] **Step 5: Commit**

```bash
git add lib/sudoku/sudoku_solver.cpp test/test_engine/test_engine.cpp
git commit -m "feat(engine): countSolutions() with early-exit for uniqueness"
```

---

## Task 4: Board — reset, value, setValue, given

**Files:**
- Create: `lib/sudoku/sudoku_board.h`
- Create: `lib/sudoku/sudoku_board.cpp`
- Modify: `test/test_engine/test_engine.cpp`

- [ ] **Step 1: Scrivi i test che falliscono**

Aggiungi in `test_engine.cpp`:

```cpp
#include "sudoku_board.h"

// Costruisce una maschera given: true dove PUZZLE != 0.
static void givenMaskFromPuzzle(bool out[81]) {
    for (int i = 0; i < 81; i++) out[i] = (PUZZLE[i] != 0);
}

void test_board_reset_sets_given_values(void) {
    bool given[81]; givenMaskFromPuzzle(given);
    sudoku::Board b;
    b.reset(SOLUTION, given);
    // Una cella data deve avere il valore della soluzione ed essere "given".
    TEST_ASSERT_EQUAL_UINT8(5, b.value(0));   // PUZZLE[0]=5
    TEST_ASSERT_TRUE(b.isGiven(0));
    // Una cella non data deve essere vuota e modificabile.
    TEST_ASSERT_EQUAL_UINT8(0, b.value(2));   // PUZZLE[2]=0
    TEST_ASSERT_FALSE(b.isGiven(2));
}

void test_board_setvalue_respects_given(void) {
    bool given[81]; givenMaskFromPuzzle(given);
    sudoku::Board b;
    b.reset(SOLUTION, given);
    // Su cella data: no-op, ritorna false.
    TEST_ASSERT_FALSE(b.setValue(0, 7));
    TEST_ASSERT_EQUAL_UINT8(5, b.value(0));
    // Su cella libera: ok.
    TEST_ASSERT_TRUE(b.setValue(2, 4));
    TEST_ASSERT_EQUAL_UINT8(4, b.value(2));
    // Cancellazione con 0.
    TEST_ASSERT_TRUE(b.setValue(2, 0));
    TEST_ASSERT_EQUAL_UINT8(0, b.value(2));
}
```

Registra in `main`:

```cpp
    RUN_TEST(test_board_reset_sets_given_values);
    RUN_TEST(test_board_setvalue_respects_given);
```

- [ ] **Step 2: Esegui il test per verificare che fallisce**

Run: `pio test -e native`
Expected: FAIL di compilazione (`sudoku_board.h` mancante).

- [ ] **Step 3: Crea `lib/sudoku/sudoku_board.h`**

```cpp
#pragma once
#include <cstdint>
#include <vector>
#include "sudoku_types.h"

namespace sudoku {

// Stato di una partita: soluzione completa, maschera dei dati iniziali,
// valori correnti dell'utente e cronologia per l'undo.
class Board {
public:
    Board();

    // Inizializza: value = solution dove given, 0 altrove. Svuota l'undo.
    void reset(const uint8_t solution[CELLS], const bool given[CELLS]);

    uint8_t value(int idx) const;        // 0 = vuota
    uint8_t solutionAt(int idx) const;
    bool    isGiven(int idx) const;

    // Imposta value[idx] (val 0..9, 0 = cancella). No-op su cella data.
    // Ritorna true se la mossa è stata applicata (e registrata per l'undo).
    bool setValue(int idx, uint8_t val);

    bool canUndo() const;
    bool undo();                          // ripristina l'ultima mossa

    bool isComplete() const;              // tutte le celle non-zero
    bool isSolved() const;                // completa e uguale a solution

    // Marca in out[81] le celle in conflitto (duplicato in riga/colonna/blocco).
    // Ritorna il numero di celle in conflitto.
    int conflicts(bool out[CELLS]) const;

    int filledCount() const;
    int givenCount() const;

private:
    uint8_t solution_[CELLS];
    uint8_t value_[CELLS];
    bool    given_[CELLS];
    struct Move { int idx; uint8_t oldVal; };
    std::vector<Move> history_;
};

} // namespace sudoku
```

- [ ] **Step 4: Crea `lib/sudoku/sudoku_board.cpp` (reset/value/setValue + stub del resto)**

```cpp
#include "sudoku_board.h"

namespace sudoku {

Board::Board() {
    for (int i = 0; i < CELLS; i++) { solution_[i] = 0; value_[i] = 0; given_[i] = false; }
}

void Board::reset(const uint8_t solution[CELLS], const bool given[CELLS]) {
    for (int i = 0; i < CELLS; i++) {
        solution_[i] = solution[i];
        given_[i] = given[i];
        value_[i] = given[i] ? solution[i] : 0;
    }
    history_.clear();
}

uint8_t Board::value(int idx) const      { return value_[idx]; }
uint8_t Board::solutionAt(int idx) const { return solution_[idx]; }
bool    Board::isGiven(int idx) const    { return given_[idx]; }

bool Board::setValue(int idx, uint8_t val) {
    if (given_[idx]) return false;
    if (val > 9) return false;
    history_.push_back({idx, value_[idx]});
    value_[idx] = val;
    return true;
}

// canUndo/undo: Task 5.
// isComplete/isSolved: Task 6.
// conflicts: Task 7.

int Board::filledCount() const {
    int n = 0; for (int i = 0; i < CELLS; i++) if (value_[i] != 0) n++; return n;
}
int Board::givenCount() const {
    int n = 0; for (int i = 0; i < CELLS; i++) if (given_[i]) n++; return n;
}

} // namespace sudoku
```

- [ ] **Step 5: Esegui il test per verificare che passa**

Run: `pio test -e native`
Expected: PASS dei due nuovi test.

- [ ] **Step 6: Commit**

```bash
git add lib/sudoku/sudoku_board.h lib/sudoku/sudoku_board.cpp test/test_engine/test_engine.cpp
git commit -m "feat(engine): Board reset/value/setValue with given protection"
```

---

## Task 5: Board — undo

**Files:**
- Modify: `lib/sudoku/sudoku_board.cpp`
- Modify: `test/test_engine/test_engine.cpp`

- [ ] **Step 1: Scrivi il test che fallisce**

Aggiungi in `test_engine.cpp`:

```cpp
void test_board_undo_restores_previous_value(void) {
    bool given[81]; givenMaskFromPuzzle(given);
    sudoku::Board b;
    b.reset(SOLUTION, given);
    TEST_ASSERT_FALSE(b.canUndo());          // niente da annullare all'inizio
    b.setValue(2, 4);
    b.setValue(2, 7);
    TEST_ASSERT_TRUE(b.canUndo());
    TEST_ASSERT_TRUE(b.undo());              // torna a 4
    TEST_ASSERT_EQUAL_UINT8(4, b.value(2));
    TEST_ASSERT_TRUE(b.undo());              // torna a 0
    TEST_ASSERT_EQUAL_UINT8(0, b.value(2));
    TEST_ASSERT_FALSE(b.canUndo());
    TEST_ASSERT_FALSE(b.undo());             // niente più da annullare
}
```

Registra in `main`:

```cpp
    RUN_TEST(test_board_undo_restores_previous_value);
```

- [ ] **Step 2: Esegui il test per verificare che fallisce**

Run: `pio test -e native`
Expected: FAIL di link (`canUndo`/`undo` non definite).

- [ ] **Step 3: Implementa undo in `sudoku_board.cpp`**

Sostituisci il commento `// canUndo/undo: Task 5.` con:

```cpp
bool Board::canUndo() const { return !history_.empty(); }

bool Board::undo() {
    if (history_.empty()) return false;
    Move m = history_.back();
    history_.pop_back();
    value_[m.idx] = m.oldVal;
    return true;
}
```

- [ ] **Step 4: Esegui il test per verificare che passa**

Run: `pio test -e native`
Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add lib/sudoku/sudoku_board.cpp test/test_engine/test_engine.cpp
git commit -m "feat(engine): Board undo stack"
```

---

## Task 6: Board — isComplete / isSolved

**Files:**
- Modify: `lib/sudoku/sudoku_board.cpp`
- Modify: `test/test_engine/test_engine.cpp`

- [ ] **Step 1: Scrivi il test che fallisce**

Aggiungi in `test_engine.cpp`:

```cpp
void test_board_solved_and_complete(void) {
    bool given[81]; givenMaskFromPuzzle(given);
    sudoku::Board b;
    b.reset(SOLUTION, given);
    TEST_ASSERT_FALSE(b.isComplete());
    TEST_ASSERT_FALSE(b.isSolved());

    // Riempi tutte le celle libere con la soluzione corretta -> vittoria.
    for (int i = 0; i < 81; i++) if (!b.isGiven(i)) b.setValue(i, SOLUTION[i]);
    TEST_ASSERT_TRUE(b.isComplete());
    TEST_ASSERT_TRUE(b.isSolved());
}

void test_board_complete_but_wrong_is_not_solved(void) {
    bool given[81]; givenMaskFromPuzzle(given);
    sudoku::Board b;
    b.reset(SOLUTION, given);
    for (int i = 0; i < 81; i++) if (!b.isGiven(i)) b.setValue(i, SOLUTION[i]);
    // Sporca una cella libera con un valore errato.
    int freeIdx = 2; // PUZZLE[2]=0
    uint8_t wrong = (SOLUTION[freeIdx] % 9) + 1;
    b.setValue(freeIdx, wrong);
    TEST_ASSERT_TRUE(b.isComplete());     // tutte piene
    TEST_ASSERT_FALSE(b.isSolved());      // ma non corretta
}
```

Registra in `main`:

```cpp
    RUN_TEST(test_board_solved_and_complete);
    RUN_TEST(test_board_complete_but_wrong_is_not_solved);
```

- [ ] **Step 2: Esegui il test per verificare che fallisce**

Run: `pio test -e native`
Expected: FAIL di link (`isComplete`/`isSolved` non definite).

- [ ] **Step 3: Implementa in `sudoku_board.cpp`**

Sostituisci il commento `// isComplete/isSolved: Task 6.` con:

```cpp
bool Board::isComplete() const {
    for (int i = 0; i < CELLS; i++) if (value_[i] == 0) return false;
    return true;
}

bool Board::isSolved() const {
    for (int i = 0; i < CELLS; i++) if (value_[i] != solution_[i]) return false;
    return true;
}
```

- [ ] **Step 4: Esegui il test per verificare che passa**

Run: `pio test -e native`
Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add lib/sudoku/sudoku_board.cpp test/test_engine/test_engine.cpp
git commit -m "feat(engine): Board isComplete/isSolved"
```

---

## Task 7: Board — conflicts

Rileva le celle in conflitto (duplicati in riga/colonna/blocco), usato dal lampeggio rosso al completamento errato.

**Files:**
- Modify: `lib/sudoku/sudoku_board.cpp`
- Modify: `test/test_engine/test_engine.cpp`

- [ ] **Step 1: Scrivi il test che fallisce**

Aggiungi in `test_engine.cpp`:

```cpp
void test_board_conflicts_detects_row_duplicate(void) {
    bool given[81]; givenMaskFromPuzzle(given);
    sudoku::Board b;
    b.reset(SOLUTION, given);
    // PUZZLE riga 0: 5,3,_,_,7,_,... -> metti un altro 5 nella cella libera idx 2.
    b.setValue(2, 5);
    bool out[81];
    int n = b.conflicts(out);
    TEST_ASSERT_TRUE(n >= 2);     // almeno la coppia in conflitto
    TEST_ASSERT_TRUE(out[0]);     // il 5 dato in (0,0)
    TEST_ASSERT_TRUE(out[2]);     // il 5 inserito in (0,2)
    TEST_ASSERT_FALSE(out[1]);    // il 3 non è in conflitto
}

void test_board_conflicts_none_on_valid_partial(void) {
    bool given[81]; givenMaskFromPuzzle(given);
    sudoku::Board b;
    b.reset(SOLUTION, given);
    bool out[81];
    TEST_ASSERT_EQUAL_INT(0, b.conflicts(out)); // il puzzle di partenza è valido
}
```

Registra in `main`:

```cpp
    RUN_TEST(test_board_conflicts_detects_row_duplicate);
    RUN_TEST(test_board_conflicts_none_on_valid_partial);
```

- [ ] **Step 2: Esegui il test per verificare che fallisce**

Run: `pio test -e native`
Expected: FAIL di link (`conflicts` non definita).

- [ ] **Step 3: Implementa in `sudoku_board.cpp`**

Sostituisci il commento `// conflicts: Task 7.` con:

```cpp
int Board::conflicts(bool out[CELLS]) const {
    for (int i = 0; i < CELLS; i++) out[i] = false;
    for (int idx = 0; idx < CELLS; idx++) {
        uint8_t v = value_[idx];
        if (v == 0) continue;
        int r = idx / 9, c = idx % 9;
        int br = (r / 3) * 3, bc = (c / 3) * 3;
        for (int k = 0; k < 9; k++) {
            int rowIdx = r * 9 + k;
            int colIdx = k * 9 + c;
            if (rowIdx != idx && value_[rowIdx] == v) { out[idx] = true; }
            if (colIdx != idx && value_[colIdx] == v) { out[idx] = true; }
        }
        for (int dr = 0; dr < 3; dr++)
            for (int dc = 0; dc < 3; dc++) {
                int bIdx = (br + dr) * 9 + (bc + dc);
                if (bIdx != idx && value_[bIdx] == v) { out[idx] = true; }
            }
    }
    int n = 0; for (int i = 0; i < CELLS; i++) if (out[i]) n++; return n;
}
```

- [ ] **Step 4: Esegui il test per verificare che passa**

Run: `pio test -e native`
Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add lib/sudoku/sudoku_board.cpp test/test_engine/test_engine.cpp
git commit -m "feat(engine): Board conflict detection"
```

---

## Task 8: Generator — `generateSolution()`

Genera una griglia 9×9 completa e valida con backtracking randomizzato.

**Files:**
- Create: `lib/sudoku/sudoku_generator.h`
- Create: `lib/sudoku/sudoku_generator.cpp`
- Modify: `test/test_engine/test_engine.cpp`

- [ ] **Step 1: Scrivi il test che fallisce**

Aggiungi in `test_engine.cpp` un RNG deterministico (LCG) e un validatore, poi i test:

```cpp
#include "sudoku_generator.h"

// LCG deterministico per test riproducibili.
static uint32_t g_seed = 12345u;
static uint32_t testRand(uint32_t maxExclusive) {
    g_seed = g_seed * 1664525u + 1013904223u;
    return (g_seed >> 8) % maxExclusive;
}

// Verifica che una griglia piena sia un Sudoku valido.
static bool isValidFullGrid(const uint8_t g[81]) {
    for (int i = 0; i < 81; i++) if (g[i] < 1 || g[i] > 9) return false;
    for (int u = 0; u < 9; u++) {
        bool row[10] = {false}, col[10] = {false}, box[10] = {false};
        for (int k = 0; k < 9; k++) {
            uint8_t rv = g[u * 9 + k];
            uint8_t cv = g[k * 9 + u];
            int br = (u / 3) * 3, bc = (u % 3) * 3;
            uint8_t bv = g[(br + k / 3) * 9 + (bc + k % 3)];
            if (row[rv] || col[cv] || box[bv]) return false;
            row[rv] = col[cv] = box[bv] = true;
        }
    }
    return true;
}

void test_generate_solution_is_valid_full_grid(void) {
    g_seed = 999u;
    uint8_t sol[81];
    sudoku::generateSolution(sol, testRand);
    TEST_ASSERT_TRUE(isValidFullGrid(sol));
}

void test_generate_solution_varies_with_seed(void) {
    uint8_t a[81], b[81];
    g_seed = 1u; sudoku::generateSolution(a, testRand);
    g_seed = 2u; sudoku::generateSolution(b, testRand);
    bool different = false;
    for (int i = 0; i < 81; i++) if (a[i] != b[i]) { different = true; break; }
    TEST_ASSERT_TRUE(different);
}
```

Registra in `main`:

```cpp
    RUN_TEST(test_generate_solution_is_valid_full_grid);
    RUN_TEST(test_generate_solution_varies_with_seed);
```

- [ ] **Step 2: Esegui il test per verificare che fallisce**

Run: `pio test -e native`
Expected: FAIL di compilazione (`sudoku_generator.h` mancante).

- [ ] **Step 3: Crea `lib/sudoku/sudoku_generator.h`**

```cpp
#pragma once
#include <cstdint>
#include "sudoku_types.h"

namespace sudoku {

// Genera una griglia 9×9 completa e valida in solution[81].
void generateSolution(uint8_t solution[CELLS], RandFn rnd);

// Genera un puzzle del livello dato: riempie solution[81] (soluzione completa)
// e given[81] (true dove la cella è un dato iniziale). Garantisce soluzione unica.
void generatePuzzle(Difficulty d, uint8_t solution[CELLS], bool given[CELLS], RandFn rnd);

} // namespace sudoku
```

- [ ] **Step 4: Crea `lib/sudoku/sudoku_generator.cpp` con `generateSolution()`**

```cpp
#include "sudoku_generator.h"
#include "sudoku_solver.h"

namespace sudoku {

static bool placeable(const uint8_t g[81], int idx, uint8_t v) {
    int r = idx / 9, c = idx % 9;
    for (int i = 0; i < 9; i++)
        if (g[r * 9 + i] == v || g[i * 9 + c] == v) return false;
    int br = (r / 3) * 3, bc = (c / 3) * 3;
    for (int dr = 0; dr < 3; dr++)
        for (int dc = 0; dc < 3; dc++)
            if (g[(br + dr) * 9 + (bc + dc)] == v) return false;
    return true;
}

static bool fillGrid(uint8_t g[81], RandFn rnd) {
    int idx = -1;
    for (int i = 0; i < 81; i++) if (g[i] == 0) { idx = i; break; }
    if (idx < 0) return true;
    uint8_t nums[9] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    for (int i = 8; i > 0; i--) {
        int j = (int)rnd((uint32_t)(i + 1));
        uint8_t t = nums[i]; nums[i] = nums[j]; nums[j] = t;
    }
    for (int k = 0; k < 9; k++) {
        uint8_t v = nums[k];
        if (placeable(g, idx, v)) {
            g[idx] = v;
            if (fillGrid(g, rnd)) return true;
            g[idx] = 0;
        }
    }
    return false;
}

void generateSolution(uint8_t solution[CELLS], RandFn rnd) {
    for (int i = 0; i < CELLS; i++) solution[i] = 0;
    fillGrid(solution, rnd);
}

// generatePuzzle: Task 9.

} // namespace sudoku
```

- [ ] **Step 5: Esegui il test per verificare che passa**

Run: `pio test -e native`
Expected: PASS dei due nuovi test.

- [ ] **Step 6: Commit**

```bash
git add lib/sudoku/sudoku_generator.h lib/sudoku/sudoku_generator.cpp test/test_engine/test_engine.cpp
git commit -m "feat(engine): generateSolution() randomized backtracking"
```

---

## Task 9: Generator — `generatePuzzle()` con unicità e difficoltà

**Files:**
- Modify: `lib/sudoku/sudoku_generator.cpp`
- Modify: `test/test_engine/test_engine.cpp`

- [ ] **Step 1: Scrivi i test che falliscono**

Aggiungi in `test_engine.cpp`:

```cpp
// Costruisce la griglia "puzzle" (solo i given) da solution+given.
static void puzzleFrom(const uint8_t sol[81], const bool given[81], uint8_t out[81]) {
    for (int i = 0; i < 81; i++) out[i] = given[i] ? sol[i] : 0;
}

void test_generate_puzzle_has_unique_solution(void) {
    g_seed = 4242u;
    uint8_t sol[81]; bool given[81];
    sudoku::generatePuzzle(sudoku::Difficulty::Medium, sol, given, testRand);
    TEST_ASSERT_TRUE(isValidFullGrid(sol));
    uint8_t puzzle[81]; puzzleFrom(sol, given, puzzle);
    TEST_ASSERT_EQUAL_INT(1, sudoku::countSolutions(puzzle, 2));
}

void test_generate_puzzle_givens_within_band(void) {
    g_seed = 7u;
    uint8_t sol[81]; bool given[81];
    auto countGivens = [&]() { int n = 0; for (int i = 0; i < 81; i++) if (given[i]) n++; return n; };

    sudoku::generatePuzzle(sudoku::Difficulty::Easy, sol, given, testRand);
    int easy = countGivens();
    TEST_ASSERT_TRUE(easy >= 40 && easy <= 50);

    sudoku::generatePuzzle(sudoku::Difficulty::Medium, sol, given, testRand);
    int medium = countGivens();
    TEST_ASSERT_TRUE(medium >= 32 && medium <= 45);

    sudoku::generatePuzzle(sudoku::Difficulty::Hard, sol, given, testRand);
    int hard = countGivens();
    TEST_ASSERT_TRUE(hard >= 26 && hard <= 42);
}
```

Registra in `main`:

```cpp
    RUN_TEST(test_generate_puzzle_has_unique_solution);
    RUN_TEST(test_generate_puzzle_givens_within_band);
```

- [ ] **Step 2: Esegui il test per verificare che fallisce**

Run: `pio test -e native`
Expected: FAIL di link (`generatePuzzle` non definita).

- [ ] **Step 3: Implementa `generatePuzzle()` in `sudoku_generator.cpp`**

Sostituisci il commento `// generatePuzzle: Task 9.` con:

```cpp
static int targetGivens(Difficulty d) {
    switch (d) {
        case Difficulty::Easy:   return 44;
        case Difficulty::Medium: return 34;
        case Difficulty::Hard:   return 28;
    }
    return 34;
}

void generatePuzzle(Difficulty d, uint8_t solution[CELLS], bool given[CELLS], RandFn rnd) {
    generateSolution(solution, rnd);
    for (int i = 0; i < CELLS; i++) given[i] = true;

    int order[CELLS];
    for (int i = 0; i < CELLS; i++) order[i] = i;
    for (int i = CELLS - 1; i > 0; i--) {
        int j = (int)rnd((uint32_t)(i + 1));
        int t = order[i]; order[i] = order[j]; order[j] = t;
    }

    int givens = CELLS;
    int target = targetGivens(d);
    for (int k = 0; k < CELLS && givens > target; k++) {
        int idx = order[k];
        if (!given[idx]) continue;
        given[idx] = false;
        uint8_t work[CELLS];
        for (int i = 0; i < CELLS; i++) work[i] = given[i] ? solution[i] : 0;
        if (countSolutions(work, 2) != 1) {
            given[idx] = true;   // la rimozione rompe l'unicità: ripristina
        } else {
            givens--;
        }
    }
}
```

- [ ] **Step 4: Esegui il test per verificare che passa**

Run: `pio test -e native`
Expected: PASS di tutti i test del motore.

- [ ] **Step 5: Commit**

```bash
git add lib/sudoku/sudoku_generator.cpp test/test_engine/test_engine.cpp
git commit -m "feat(engine): generatePuzzle() with unique-solution digging + difficulty bands"
```

---

## Done criteria (Piano A)

- `pio test -e native` esegue tutti i test verde.
- La libreria `lib/sudoku/` non include header Arduino/LVGL (resta portabile).
- API pronte per il firmware: `solve`, `countSolutions`, `Board` (reset/value/setValue/undo/isComplete/isSolved/conflicts/filledCount/givenCount), `generateSolution`, `generatePuzzle`.

Il **Piano B** (firmware/UI) verrà scritto dopo che questo piano è verde, e userà queste API per: copia dei file HW dallo Smart Home Panel, env device in `platformio.ini`, `storage` (NVS), `game_session` (cronometro + macchina a stati), schermate LVGL (menu/game/pause/win) e `main.cpp`.
