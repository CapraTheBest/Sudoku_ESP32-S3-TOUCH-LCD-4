#pragma once
#include <cstdint>
#include "sudoku_types.h"

namespace sudoku {

// Genera una griglia 9x9 completa e valida in solution[81].
void generateSolution(uint8_t solution[CELLS], RandFn rnd);

// Genera un puzzle del livello dato: riempie solution[81] (soluzione completa)
// e given[81] (true dove la cella e' un dato iniziale). Garantisce soluzione unica.
void generatePuzzle(Difficulty d, uint8_t solution[CELLS], bool given[CELLS], RandFn rnd);

} // namespace sudoku
