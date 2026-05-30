#include <unity.h>
#include "sudoku_types.h"
#include "sudoku_solver.h"
#include "sudoku_board.h"

void setUp(void) {}
void tearDown(void) {}

void test_scaffolding_builds(void) {
    TEST_ASSERT_EQUAL_INT(81, sudoku::CELLS);
}

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

void test_solve_fills_known_puzzle(void) {
    uint8_t g[81];
    for (int i = 0; i < 81; i++) g[i] = PUZZLE[i];
    TEST_ASSERT_TRUE(sudoku::solve(g));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(SOLUTION, g, 81);
}

void test_count_unique_puzzle_is_one(void) {
    TEST_ASSERT_EQUAL_INT(1, sudoku::countSolutions(PUZZLE, 2));
}

void test_count_empty_grid_is_capped_at_limit(void) {
    uint8_t empty[81] = {0};
    // Una griglia vuota ha moltissime soluzioni: l'early-exit deve fermarsi a 2.
    TEST_ASSERT_EQUAL_INT(2, sudoku::countSolutions(empty, 2));
}

int main(int, char **) {
    UNITY_BEGIN();
    RUN_TEST(test_scaffolding_builds);
    RUN_TEST(test_board_reset_sets_given_values);
    RUN_TEST(test_board_setvalue_respects_given);
    RUN_TEST(test_solve_fills_known_puzzle);
    RUN_TEST(test_count_unique_puzzle_is_one);
    RUN_TEST(test_count_empty_grid_is_capped_at_limit);
    return UNITY_END();
}
