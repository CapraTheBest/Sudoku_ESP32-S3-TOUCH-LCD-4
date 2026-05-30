#pragma once
#include <cstdint>

namespace sudoku {

// Risolve la griglia in place (grid[81], 0 = vuoto). True se trovata soluzione.
bool solve(uint8_t grid[81]);

// Conta le soluzioni fino a `limit` (early-exit). Per l'unicità usare limit=2.
int countSolutions(const uint8_t grid[81], int limit);

} // namespace sudoku
