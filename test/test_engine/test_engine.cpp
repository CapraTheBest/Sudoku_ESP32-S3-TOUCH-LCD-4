#include <unity.h>
#include "sudoku_types.h"
#include "sudoku_solver.h"

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
    RUN_TEST(test_solve_fills_known_puzzle);
    RUN_TEST(test_count_unique_puzzle_is_one);
    RUN_TEST(test_count_empty_grid_is_capped_at_limit);
    return UNITY_END();
}
