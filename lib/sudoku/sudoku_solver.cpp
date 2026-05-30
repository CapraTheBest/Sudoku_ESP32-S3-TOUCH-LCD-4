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

} // namespace sudoku
