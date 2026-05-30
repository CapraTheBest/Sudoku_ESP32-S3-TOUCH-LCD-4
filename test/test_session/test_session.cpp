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

int main(int, char **) {
    UNITY_BEGIN();
    RUN_TEST(test_newgame_enters_playing_with_valid_board);
    return UNITY_END();
}
