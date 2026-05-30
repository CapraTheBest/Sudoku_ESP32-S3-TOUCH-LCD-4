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

int main(int, char **) {
    UNITY_BEGIN();
    RUN_TEST(test_newgame_enters_playing_with_valid_board);
    RUN_TEST(test_timer_counts_while_playing);
    RUN_TEST(test_pause_freezes_then_resume_continues);
    RUN_TEST(test_pause_resume_ignored_in_wrong_state);
    return UNITY_END();
}
