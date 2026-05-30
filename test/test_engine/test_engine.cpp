#include <unity.h>
#include "sudoku_types.h"
#include "sudoku_solver.h"
#include "sudoku_board.h"
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

int main(int, char **) {
    UNITY_BEGIN();
    RUN_TEST(test_scaffolding_builds);
    RUN_TEST(test_board_reset_sets_given_values);
    RUN_TEST(test_board_setvalue_respects_given);
    RUN_TEST(test_solve_fills_known_puzzle);
    RUN_TEST(test_count_unique_puzzle_is_one);
    RUN_TEST(test_count_empty_grid_is_capped_at_limit);
    RUN_TEST(test_board_undo_restores_previous_value);
    RUN_TEST(test_board_solved_and_complete);
    RUN_TEST(test_board_complete_but_wrong_is_not_solved);
    RUN_TEST(test_board_conflicts_detects_row_duplicate);
    RUN_TEST(test_board_conflicts_none_on_valid_partial);
    RUN_TEST(test_generate_solution_is_valid_full_grid);
    RUN_TEST(test_generate_solution_varies_with_seed);
    return UNITY_END();
}
