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
