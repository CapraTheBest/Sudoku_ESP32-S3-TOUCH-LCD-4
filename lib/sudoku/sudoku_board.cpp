#include "sudoku_board.h"

namespace sudoku {

Board::Board() {
    for (int i = 0; i < CELLS; i++) {
        solution_[i] = 0; value_[i] = 0; given_[i] = false; notes_[i] = 0;
    }
}

void Board::reset(const uint8_t solution[CELLS], const bool given[CELLS]) {
    for (int i = 0; i < CELLS; i++) {
        solution_[i] = solution[i];
        given_[i] = given[i];
        value_[i] = given[i] ? solution[i] : 0;
        notes_[i] = 0;
    }
    history_.clear();
}

static inline bool inRange(int idx) { return idx >= 0 && idx < CELLS; }

uint8_t Board::value(int idx) const      { return inRange(idx) ? value_[idx]    : 0; }
uint8_t Board::solutionAt(int idx) const { return inRange(idx) ? solution_[idx] : 0; }
bool    Board::isGiven(int idx) const    { return inRange(idx) ? given_[idx]    : false; }

bool Board::setValue(int idx, uint8_t val) {
    if (!inRange(idx)) return false;
    if (given_[idx]) return false;
    if (val > 9) return false;
    history_.push_back({idx, value_[idx]});
    value_[idx] = val;
    if (val >= 1) notes_[idx] = 0;   // inserire una cifra cancella gli appunti
    return true;
}

uint16_t Board::notes(int idx) const { return inRange(idx) ? notes_[idx] : 0; }

void Board::setNotesMask(int idx, uint16_t mask) {
    if (!inRange(idx)) return;
    notes_[idx] = mask & 0x1FF;   // solo i 9 bit candidati
}

bool Board::toggleNote(int idx, uint8_t d) {
    if (!inRange(idx)) return false;
    if (d < 1 || d > 9) return false;
    if (given_[idx] || value_[idx] != 0) return false;  // solo celle vuote libere
    notes_[idx] ^= (uint16_t)(1u << (d - 1));
    return true;
}

bool Board::canUndo() const { return !history_.empty(); }

bool Board::undo() {
    if (history_.empty()) return false;
    Move m = history_.back();
    history_.pop_back();
    value_[m.idx] = m.oldVal;
    return true;
}
bool Board::isComplete() const {
    for (int i = 0; i < CELLS; i++) if (value_[i] == 0) return false;
    return true;
}

bool Board::isSolved() const {
    for (int i = 0; i < CELLS; i++) if (value_[i] != solution_[i]) return false;
    return true;
}
bool Board::isDigitComplete(uint8_t d) const {
    if (d < 1 || d > 9) return false;
    int n = 0;
    for (int i = 0; i < CELLS; i++) {
        if (value_[i] != d) continue;
        if (value_[i] != solution_[i]) return false;  // piazzata ma sbagliata
        n++;
    }
    return n == 9;
}

int Board::conflicts(bool out[CELLS]) const {
    for (int i = 0; i < CELLS; i++) out[i] = false;
    for (int idx = 0; idx < CELLS; idx++) {
        uint8_t v = value_[idx];
        if (v == 0) continue;
        int r = idx / 9, c = idx % 9;
        int br = (r / 3) * 3, bc = (c / 3) * 3;
        for (int k = 0; k < 9; k++) {
            int rowIdx = r * 9 + k;
            int colIdx = k * 9 + c;
            if (rowIdx != idx && value_[rowIdx] == v) { out[idx] = true; }
            if (colIdx != idx && value_[colIdx] == v) { out[idx] = true; }
        }
        for (int dr = 0; dr < 3; dr++)
            for (int dc = 0; dc < 3; dc++) {
                int bIdx = (br + dr) * 9 + (bc + dc);
                if (bIdx != idx && value_[bIdx] == v) { out[idx] = true; }
            }
    }
    int n = 0; for (int i = 0; i < CELLS; i++) if (out[i]) n++; return n;
}

int Board::filledCount() const {
    int n = 0; for (int i = 0; i < CELLS; i++) if (value_[i] != 0) n++; return n;
}
int Board::givenCount() const {
    int n = 0; for (int i = 0; i < CELLS; i++) if (given_[i]) n++; return n;
}

} // namespace sudoku
