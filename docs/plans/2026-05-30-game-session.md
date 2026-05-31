# Game Session Implementation Plan (Piano B di 3)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Costruire `GameSession`, la logica di una partita di Sudoku (macchina a stati, cronometro con pausa, input, rilevamento vittoria, snapshot per salvataggio/ripristino), in C++ puro e interamente testata con test nativi.

**Architecture:** `GameSession` vive nella stessa libreria portabile `lib/sudoku/` del motore (nessuna dipendenza Arduino/LVGL). Usa `Board` e `generatePuzzle` già esistenti. Il tempo è iniettato come `ClockFn` (function pointer che ritorna ms monotoni), così i test sono deterministici — stesso pattern del `RandFn` del motore. Il device passerà `millis()`. La persistenza NON è dentro la sessione: `GameSession` espone uno `Snapshot` serializzabile che il layer firmware (Piano C) salverà su NVS.

**Tech Stack:** C++17, PlatformIO `[env:native]`, Unity. Nessun hardware necessario per questo piano.

---

## File Structure

- `lib/sudoku/game_session.h` / `.cpp` — classe `GameSession` (stato partita + timer + input + snapshot).
- `test/test_session/test_session.cpp` — test Unity della sessione (nuovo programma di test, separato da `test_engine`).

`GameSession` dipende solo da `sudoku_board.h`, `sudoku_generator.h`, `sudoku_types.h`. Resta C++ puro (compila nativo e on-device).

---

## API di riferimento (cosa costruiremo)

```cpp
// lib/sudoku/game_session.h
#pragma once
#include <cstdint>
#include "sudoku_types.h"
#include "sudoku_board.h"

namespace sudoku {

// Clock monotono iniettabile (ms). Device: millis(). Test: contatore finto.
using ClockFn = uint32_t (*)();

enum class GameState : uint8_t { Menu, Playing, Paused, Won };

class GameSession {
public:
    GameSession(RandFn rnd, ClockFn clock);

    GameState  state() const;
    const Board& board() const;
    Difficulty difficulty() const;
    int        selectedCell() const;   // -1 = nessuna

    void newGame(Difficulty d);        // genera, timer a 0, stato Playing

    // Input (ignorato se non Playing)
    void selectCell(int idx);          // idx in [0, CELLS); -1 deseleziona
    void enterValue(uint8_t v);        // 1..9 nella cella selezionata; vittoria -> Won
    void eraseSelected();              // = enterValue(0)
    void undo();

    void pause();                      // Playing -> Paused (ferma il tempo)
    void resume();                     // Paused  -> Playing (riprende il tempo)

    uint32_t elapsedMs() const;        // tempo di gioco escluse le pause

    // Persistenza (gestita dal firmware nel Piano C)
    struct Snapshot {
        uint8_t  solution[CELLS];
        uint8_t  value[CELLS];
        bool     given[CELLS];
        uint32_t elapsedMs;
        uint8_t  difficulty;
        bool     valid;                // true solo se Playing/Paused
    };
    Snapshot snapshot() const;
    bool     restore(const Snapshot& s);  // -> Paused; false se snapshot non valido

private:
    RandFn     rnd_;
    ClockFn    clock_;
    Board      board_;
    GameState  state_;
    Difficulty diff_;
    int        selected_;
    uint32_t   accumMs_;   // tempo accumulato prima dell'ultimo resume
    uint32_t   runStart_;  // clock_() all'ultimo start/resume
};

} // namespace sudoku
```

---

## Task 1: GameSession — scaffolding, newGame, accessor

**Files:**
- Create: `lib/sudoku/game_session.h`
- Create: `lib/sudoku/game_session.cpp`
- Create: `test/test_session/test_session.cpp`

- [ ] **Step 1: Scrivi il test che fallisce**

Crea `test/test_session/test_session.cpp`:

```cpp
#include <unity.h>
#include "game_session.h"

// RNG e clock deterministici per i test.
static uint32_t s_seed = 12345u;
static uint32_t testRand(uint32_t maxExclusive) {
    s_seed = s_seed * 1664525u + 1013904223u;
    return (s_seed >> 8) % maxExclusive;
}
static uint32_t s_now = 0;
static uint32_t testClock() { return s_now; }

void setUp(void) { s_seed = 12345u; s_now = 0; }
void tearDown(void) {}

void test_newgame_enters_playing_with_valid_board(void) {
    sudoku::GameSession g(testRand, testClock);
    TEST_ASSERT_EQUAL(sudoku::GameState::Menu, g.state());
    g.newGame(sudoku::Difficulty::Easy);
    TEST_ASSERT_EQUAL(sudoku::GameState::Playing, g.state());
    TEST_ASSERT_EQUAL(sudoku::Difficulty::Easy, g.difficulty());
    TEST_ASSERT_EQUAL_INT(-1, g.selectedCell());
    // la board ha un numero di given plausibile (Sudoku unico: >= 17)
    int givens = g.board().givenCount();
    TEST_ASSERT_TRUE(givens >= 17 && givens <= 81);
}

int main(int, char **) {
    UNITY_BEGIN();
    RUN_TEST(test_newgame_enters_playing_with_valid_board);
    return UNITY_END();
}
```

- [ ] **Step 2: Esegui il test per verificare che fallisce**

Run: `pio test -e native`
Expected: FAIL di compilazione (`game_session.h` non esiste).

- [ ] **Step 3: Crea `lib/sudoku/game_session.h`**

(Esattamente il contenuto del blocco "API di riferimento" qui sopra.)

- [ ] **Step 4: Crea `lib/sudoku/game_session.cpp` con costruttore, newGame e accessor**

```cpp
#include "game_session.h"
#include "sudoku_generator.h"

namespace sudoku {

GameSession::GameSession(RandFn rnd, ClockFn clock)
    : rnd_(rnd), clock_(clock), state_(GameState::Menu),
      diff_(Difficulty::Medium), selected_(-1), accumMs_(0), runStart_(0) {}

GameState    GameSession::state() const       { return state_; }
const Board& GameSession::board() const        { return board_; }
Difficulty   GameSession::difficulty() const   { return diff_; }
int          GameSession::selectedCell() const { return selected_; }

void GameSession::newGame(Difficulty d) {
    uint8_t solution[CELLS];
    bool given[CELLS];
    generatePuzzle(d, solution, given, rnd_);
    board_.reset(solution, given);
    diff_ = d;
    selected_ = -1;
    accumMs_ = 0;
    runStart_ = clock_();
    state_ = GameState::Playing;
}

// elapsedMs/pause/resume: Task 2.
// selectCell/enterValue/eraseSelected/undo: Task 3.
// snapshot/restore: Task 4.

} // namespace sudoku
```

IMPORTANT: lascia i tre commenti segnaposto esattamente così: i task successivi li sostituiscono. I metodi dichiarati ma non ancora definiti non danno errore di link finché i test non li chiamano.

- [ ] **Step 5: Esegui il test per verificare che passa**

Run: `pio test -e native`
Expected: PASS `test_newgame_enters_playing_with_valid_board` (oltre ai test del motore già verdi).

- [ ] **Step 6: Commit**

```bash
git add lib/sudoku/game_session.h lib/sudoku/game_session.cpp test/test_session/test_session.cpp
git commit -m "feat(session): GameSession newGame + state accessors"
```
(Se il commit fallisce per identità git mancante: `git -c user.name="Dogfight84" -c user.email="ale.petrolati@gmail.com" commit -m "..."`.)

---

## Task 2: GameSession — cronometro (elapsed / pause / resume)

**Files:**
- Modify: `lib/sudoku/game_session.cpp`
- Modify: `test/test_session/test_session.cpp`

- [ ] **Step 1: Scrivi i test che falliscono**

Aggiungi in `test_session.cpp` (sopra `main`):

```cpp
void test_timer_counts_while_playing(void) {
    sudoku::GameSession g(testRand, testClock);
    s_now = 1000;
    g.newGame(sudoku::Difficulty::Easy);   // runStart = 1000
    TEST_ASSERT_EQUAL_UINT32(0, g.elapsedMs());
    s_now = 1500;
    TEST_ASSERT_EQUAL_UINT32(500, g.elapsedMs());
}

void test_pause_freezes_then_resume_continues(void) {
    sudoku::GameSession g(testRand, testClock);
    s_now = 0;
    g.newGame(sudoku::Difficulty::Easy);
    s_now = 2000;
    g.pause();                              // accumula 2000, stato Paused
    TEST_ASSERT_EQUAL(sudoku::GameState::Paused, g.state());
    s_now = 9000;                           // tempo passa mentre in pausa
    TEST_ASSERT_EQUAL_UINT32(2000, g.elapsedMs());  // congelato
    g.resume();                             // runStart = 9000, Playing
    TEST_ASSERT_EQUAL(sudoku::GameState::Playing, g.state());
    s_now = 9500;
    TEST_ASSERT_EQUAL_UINT32(2500, g.elapsedMs());  // 2000 + 500
}

void test_pause_resume_ignored_in_wrong_state(void) {
    sudoku::GameSession g(testRand, testClock);
    g.resume();                             // in Menu: no-op
    TEST_ASSERT_EQUAL(sudoku::GameState::Menu, g.state());
    g.newGame(sudoku::Difficulty::Easy);
    g.resume();                             // già Playing: no-op
    TEST_ASSERT_EQUAL(sudoku::GameState::Playing, g.state());
}
```

Registra in `main()`:

```cpp
    RUN_TEST(test_timer_counts_while_playing);
    RUN_TEST(test_pause_freezes_then_resume_continues);
    RUN_TEST(test_pause_resume_ignored_in_wrong_state);
```

- [ ] **Step 2: Esegui il test per verificare che fallisce**

Run: `pio test -e native`
Expected: FAIL di link (`elapsedMs`/`pause`/`resume` non definite).

- [ ] **Step 3: Implementa in `game_session.cpp`**

Sostituisci il commento `// elapsedMs/pause/resume: Task 2.` con:

```cpp
uint32_t GameSession::elapsedMs() const {
    uint32_t e = accumMs_;
    if (state_ == GameState::Playing) e += clock_() - runStart_;
    return e;
}

void GameSession::pause() {
    if (state_ != GameState::Playing) return;
    accumMs_ += clock_() - runStart_;
    state_ = GameState::Paused;
}

void GameSession::resume() {
    if (state_ != GameState::Paused) return;
    runStart_ = clock_();
    state_ = GameState::Playing;
}
```

- [ ] **Step 4: Esegui il test per verificare che passa**

Run: `pio test -e native`
Expected: PASS dei tre nuovi test.

- [ ] **Step 5: Commit**

```bash
git add lib/sudoku/game_session.cpp test/test_session/test_session.cpp
git commit -m "feat(session): game timer with pause/resume"
```

---

## Task 3: GameSession — input e rilevamento vittoria

**Files:**
- Modify: `lib/sudoku/game_session.cpp`
- Modify: `test/test_session/test_session.cpp`

- [ ] **Step 1: Scrivi i test che falliscono**

Aggiungi in `test_session.cpp` (sopra `main`). Helper per trovare una cella libera e completare la griglia:

```cpp
// Indice della prima cella non-given.
static int firstFreeCell(const sudoku::Board &b) {
    for (int i = 0; i < sudoku::CELLS; i++) if (!b.isGiven(i)) return i;
    return -1;
}

void test_enter_value_sets_cell_when_playing(void) {
    sudoku::GameSession g(testRand, testClock);
    g.newGame(sudoku::Difficulty::Easy);
    int free = firstFreeCell(g.board());
    uint8_t correct = g.board().solutionAt(free);
    g.selectCell(free);
    TEST_ASSERT_EQUAL_INT(free, g.selectedCell());
    g.enterValue(correct);
    TEST_ASSERT_EQUAL_UINT8(correct, g.board().value(free));
    g.eraseSelected();
    TEST_ASSERT_EQUAL_UINT8(0, g.board().value(free));
}

void test_input_ignored_without_selection_or_when_paused(void) {
    sudoku::GameSession g(testRand, testClock);
    g.newGame(sudoku::Difficulty::Easy);
    g.enterValue(5);                       // nessuna selezione: no-op
    int free = firstFreeCell(g.board());
    g.selectCell(free);
    g.pause();
    g.enterValue(g.board().solutionAt(free)); // in pausa: no-op
    TEST_ASSERT_EQUAL_UINT8(0, g.board().value(free));
}

void test_undo_reverts_last_move(void) {
    sudoku::GameSession g(testRand, testClock);
    g.newGame(sudoku::Difficulty::Easy);
    int free = firstFreeCell(g.board());
    g.selectCell(free);
    g.enterValue(1);
    g.enterValue(2);
    g.undo();
    TEST_ASSERT_EQUAL_UINT8(1, g.board().value(free));
}

void test_completing_correctly_wins_and_freezes_timer(void) {
    sudoku::GameSession g(testRand, testClock);
    s_now = 0;
    g.newGame(sudoku::Difficulty::Easy);
    // Riempi tutte le celle libere con la soluzione corretta.
    for (int i = 0; i < sudoku::CELLS; i++) {
        if (!g.board().isGiven(i)) {
            g.selectCell(i);
            s_now += 10;                   // un po' di tempo per mossa
            g.enterValue(g.board().solutionAt(i));
        }
    }
    TEST_ASSERT_EQUAL(sudoku::GameState::Won, g.state());
    uint32_t won = g.elapsedMs();
    s_now += 5000;                         // il tempo dopo la vittoria non conta
    TEST_ASSERT_EQUAL_UINT32(won, g.elapsedMs());
}

void test_completing_wrong_stays_playing(void) {
    sudoku::GameSession g(testRand, testClock);
    g.newGame(sudoku::Difficulty::Easy);
    // Riempi tutte le libere correttamente tranne l'ultima, che mettiamo errata.
    int lastFree = -1;
    for (int i = 0; i < sudoku::CELLS; i++) if (!g.board().isGiven(i)) lastFree = i;
    for (int i = 0; i < sudoku::CELLS; i++) {
        if (!g.board().isGiven(i)) {
            g.selectCell(i);
            if (i == lastFree) {
                uint8_t correct = g.board().solutionAt(i);
                g.enterValue((uint8_t)((correct % 9) + 1)); // valore sbagliato
            } else {
                g.enterValue(g.board().solutionAt(i));
            }
        }
    }
    TEST_ASSERT_TRUE(g.board().isComplete());
    TEST_ASSERT_EQUAL(sudoku::GameState::Playing, g.state()); // piena ma errata
}
```

Registra in `main()`:

```cpp
    RUN_TEST(test_enter_value_sets_cell_when_playing);
    RUN_TEST(test_input_ignored_without_selection_or_when_paused);
    RUN_TEST(test_undo_reverts_last_move);
    RUN_TEST(test_completing_correctly_wins_and_freezes_timer);
    RUN_TEST(test_completing_wrong_stays_playing);
```

- [ ] **Step 2: Esegui il test per verificare che fallisce**

Run: `pio test -e native`
Expected: FAIL di link (`selectCell`/`enterValue`/`eraseSelected`/`undo` non definite).

- [ ] **Step 3: Implementa in `game_session.cpp`**

Sostituisci il commento `// selectCell/enterValue/eraseSelected/undo: Task 3.` con:

```cpp
void GameSession::selectCell(int idx) {
    if (state_ != GameState::Playing) return;
    if (idx < -1 || idx >= CELLS) return;
    selected_ = idx;
}

void GameSession::enterValue(uint8_t v) {
    if (state_ != GameState::Playing) return;
    if (selected_ < 0) return;
    if (v > 9) return;
    if (!board_.setValue(selected_, v)) return;   // cella data o valore non valido
    if (board_.isComplete() && board_.isSolved()) {
        accumMs_ += clock_() - runStart_;          // congela il cronometro
        state_ = GameState::Won;
    }
}

void GameSession::eraseSelected() { enterValue(0); }

void GameSession::undo() {
    if (state_ != GameState::Playing) return;
    board_.undo();
}
```

- [ ] **Step 4: Esegui il test per verificare che passa**

Run: `pio test -e native`
Expected: PASS dei cinque nuovi test.

- [ ] **Step 5: Commit**

```bash
git add lib/sudoku/game_session.cpp test/test_session/test_session.cpp
git commit -m "feat(session): cell input, undo, win detection"
```

---

## Task 4: GameSession — snapshot / restore

**Files:**
- Modify: `lib/sudoku/game_session.cpp`
- Modify: `test/test_session/test_session.cpp`

- [ ] **Step 1: Scrivi i test che falliscono**

Aggiungi in `test_session.cpp` (sopra `main`):

```cpp
void test_snapshot_restore_roundtrip(void) {
    sudoku::GameSession g(testRand, testClock);
    s_now = 0;
    g.newGame(sudoku::Difficulty::Medium);
    int free = firstFreeCell(g.board());
    g.selectCell(free);
    g.enterValue(g.board().solutionAt(free));
    s_now = 3000;
    g.pause();   // elapsed = 3000

    sudoku::GameSession::Snapshot snap = g.snapshot();
    TEST_ASSERT_TRUE(snap.valid);
    TEST_ASSERT_EQUAL_UINT32(3000, snap.elapsedMs);

    // Ripristina in una nuova sessione.
    sudoku::GameSession g2(testRand, testClock);
    s_now = 50000;
    TEST_ASSERT_TRUE(g2.restore(snap));
    TEST_ASSERT_EQUAL(sudoku::GameState::Paused, g2.state());
    TEST_ASSERT_EQUAL(sudoku::Difficulty::Medium, g2.difficulty());
    TEST_ASSERT_EQUAL_UINT32(3000, g2.elapsedMs());        // congelato (Paused)
    TEST_ASSERT_EQUAL_UINT8(g.board().value(free), g2.board().value(free));
    TEST_ASSERT_EQUAL_INT(g.board().givenCount(), g2.board().givenCount());
    // riprendendo, il tempo riparte da dov'era
    g2.resume();
    s_now = 50500;
    TEST_ASSERT_EQUAL_UINT32(3500, g2.elapsedMs());
}

void test_restore_rejects_invalid_snapshot(void) {
    sudoku::GameSession g(testRand, testClock);
    sudoku::GameSession::Snapshot snap{};   // valid = false
    snap.valid = false;
    TEST_ASSERT_FALSE(g.restore(snap));
    TEST_ASSERT_EQUAL(sudoku::GameState::Menu, g.state());
}
```

Registra in `main()`:

```cpp
    RUN_TEST(test_snapshot_restore_roundtrip);
    RUN_TEST(test_restore_rejects_invalid_snapshot);
```

- [ ] **Step 2: Esegui il test per verificare che fallisce**

Run: `pio test -e native`
Expected: FAIL di link (`snapshot`/`restore` non definite).

- [ ] **Step 3: Implementa in `game_session.cpp`**

Sostituisci il commento `// snapshot/restore: Task 4.` con:

```cpp
GameSession::Snapshot GameSession::snapshot() const {
    Snapshot s;
    for (int i = 0; i < CELLS; i++) {
        s.solution[i] = board_.solutionAt(i);
        s.value[i]    = board_.value(i);
        s.given[i]    = board_.isGiven(i);
    }
    s.elapsedMs   = elapsedMs();
    s.difficulty  = (uint8_t) diff_;
    s.valid       = (state_ == GameState::Playing || state_ == GameState::Paused);
    return s;
}

bool GameSession::restore(const Snapshot &s) {
    if (!s.valid) return false;
    board_.reset(s.solution, s.given);
    for (int i = 0; i < CELLS; i++)
        if (!s.given[i] && s.value[i] != 0) board_.setValue(i, s.value[i]);
    diff_     = (Difficulty) s.difficulty;
    accumMs_  = s.elapsedMs;
    selected_ = -1;
    state_    = GameState::Paused;
    return true;
}
```

Nota: `restore` riapplica i valori utente con `setValue`, quindi finiscono nello stack di undo della board. È accettabile (dopo il ripristino si è in Paused; alla ripresa un eventuale undo torna verso la griglia salvata). Non è un problema di correttezza.

- [ ] **Step 4: Esegui il test per verificare che passa**

Run: `pio test -e native`
Expected: PASS di tutti i test (motore + sessione).

- [ ] **Step 5: Commit**

```bash
git add lib/sudoku/game_session.cpp test/test_session/test_session.cpp
git commit -m "feat(session): snapshot/restore for save-resume"
```

---

## Done criteria (Piano B)

- `pio test -e native` verde su tutti i test (motore + sessione).
- `GameSession` resta C++ puro (nessun include Arduino/LVGL): pronta sia per il device sia per i test.
- API pronte per il firmware (Piano C): `newGame`, `selectCell`/`enterValue`/`eraseSelected`/`undo`, `pause`/`resume`, `elapsedMs`, `state`/`board`/`difficulty`/`selectedCell`, `snapshot`/`restore`.

Il **Piano C** (integrazione firmware) userà queste API per: copia dei file HW dallo Smart Home Panel + env device in `platformio.ini`, `storage` (NVS: salva/carica `Snapshot` + miglior tempo per livello), schermate LVGL (menu/game/pause/win, layout C★, font Montserrat 48 per il timer e 28 per le celle), `main.cpp` (bring-up: IO expander TCA9554 con sequenza Waveshare, accensione backlight EXIO1, `lvgl_port_init`, loop con `lvgl_port_lock`/`millis()` come ClockFn). Verifica sul device reale.
