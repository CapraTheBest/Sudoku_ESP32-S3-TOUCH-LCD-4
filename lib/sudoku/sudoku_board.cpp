#include "sudoku_board.h"

namespace sudoku {

Board::Board() {
    for (int i = 0; i < CELLS; i++) { solution_[i] = 0; value_[i] = 0; given_[i] = false; }
}

void Board::reset(const uint8_t solution[CELLS], const bool given[CELLS]) {
    for (int i = 0; i < CELLS; i++) {
        solution_[i] = solution[i];
        given_[i] = given[i];
        value_[i] = given[i] ? solution[i] : 0;
    }
    history_.clear();
}

uint8_t Board::value(int idx) const      { return value_[idx]; }
uint8_t Board::solutionAt(int idx) const { return solution_[idx]; }
bool    Board::isGiven(int idx) const    { return given_[idx]; }

bool Board::setValue(int idx, uint8_t val) {
    if (given_[idx]) return false;
    if (val > 9) return false;
    history_.push_back({idx, value_[idx]});
    value_[idx] = val;
    return true;
}

// canUndo/undo: Task 5.
// isComplete/isSolved: Task 6.
// conflicts: Task 7.

int Board::filledCount() const {
    int n = 0; for (int i = 0; i < CELLS; i++) if (value_[i] != 0) n++; return n;
}
int Board::givenCount() const {
    int n = 0; for (int i = 0; i < CELLS; i++) if (given_[i]) n++; return n;
}

} // namespace sudoku
