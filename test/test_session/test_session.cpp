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
    int givens = g.board().givenCount();
    TEST_ASSERT_TRUE(givens >= 17 && givens <= 81);
}

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

void test_input_ignored_after_won(void) {
    sudoku::GameSession g(testRand, testClock);
    g.newGame(sudoku::Difficulty::Easy);
    for (int i = 0; i < sudoku::CELLS; i++) {
        if (!g.board().isGiven(i)) { g.selectCell(i); g.enterValue(g.board().solutionAt(i)); }
    }
    TEST_ASSERT_EQUAL(sudoku::GameState::Won, g.state());
    // Dopo la vittoria ogni input e' ignorato: selectCell non cambia la selezione,
    // enterValue/undo non toccano la board, lo stato resta Won.
    int before = g.selectedCell();
    g.selectCell(0);
    TEST_ASSERT_EQUAL_INT(before, g.selectedCell());   // selectCell ignorato (non Playing)
    g.enterValue(1);
    g.undo();
    TEST_ASSERT_EQUAL(sudoku::GameState::Won, g.state());
}

void test_selectcell_range_and_deselect(void) {
    sudoku::GameSession g(testRand, testClock);
    g.newGame(sudoku::Difficulty::Easy);
    int free = firstFreeCell(g.board());
    g.selectCell(free);
    g.selectCell(999);                              // fuori range: selezione invariata
    TEST_ASSERT_EQUAL_INT(free, g.selectedCell());
    g.selectCell(-5);                               // < -1: invariata
    TEST_ASSERT_EQUAL_INT(free, g.selectedCell());
    g.selectCell(-1);                               // -1: deseleziona
    TEST_ASSERT_EQUAL_INT(-1, g.selectedCell());
}

void test_snapshot_in_menu_is_invalid(void) {
    sudoku::GameSession g(testRand, testClock);
    TEST_ASSERT_FALSE(g.snapshot().valid);          // Menu: niente da salvare
    g.newGame(sudoku::Difficulty::Easy);
    for (int i = 0; i < sudoku::CELLS; i++) {
        if (!g.board().isGiven(i)) { g.selectCell(i); g.enterValue(g.board().solutionAt(i)); }
    }
    TEST_ASSERT_EQUAL(sudoku::GameState::Won, g.state());
    TEST_ASSERT_FALSE(g.snapshot().valid);          // Won: partita finita, non salvabile
}

int main(int, char **) {
    UNITY_BEGIN();
    RUN_TEST(test_newgame_enters_playing_with_valid_board);
    RUN_TEST(test_timer_counts_while_playing);
    RUN_TEST(test_pause_freezes_then_resume_continues);
    RUN_TEST(test_pause_resume_ignored_in_wrong_state);
    RUN_TEST(test_enter_value_sets_cell_when_playing);
    RUN_TEST(test_input_ignored_without_selection_or_when_paused);
    RUN_TEST(test_undo_reverts_last_move);
    RUN_TEST(test_completing_correctly_wins_and_freezes_timer);
    RUN_TEST(test_completing_wrong_stays_playing);
    RUN_TEST(test_snapshot_restore_roundtrip);
    RUN_TEST(test_restore_rejects_invalid_snapshot);
    RUN_TEST(test_input_ignored_after_won);
    RUN_TEST(test_selectcell_range_and_deselect);
    RUN_TEST(test_snapshot_in_menu_is_invalid);
    return UNITY_END();
}
