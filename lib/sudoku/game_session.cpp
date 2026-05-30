#include "game_session.h"
#include "sudoku_generator.h"

namespace sudoku {

GameSession::GameSession(RandFn rnd, ClockFn clock)
    : rnd_(rnd), clock_(clock), state_(GameState::Menu),
      diff_(Difficulty::Medium), selected_(-1), accumMs_(0), runStart_(0) {}

GameState    GameSession::state() const       { return state_; }
const Board& GameSession::board() const        { return board_; }
Difficulty   GameSession::difficulty() const   { return diff_; }
int          GameSession::selectedCell() const { return selected_; }

void GameSession::newGame(Difficulty d) {
    uint8_t solution[CELLS];
    bool given[CELLS];
    generatePuzzle(d, solution, given, rnd_);
    board_.reset(solution, given);
    diff_ = d;
    selected_ = -1;
    accumMs_ = 0;
    runStart_ = clock_();
    state_ = GameState::Playing;
}

// elapsedMs/pause/resume: Task 2.
// selectCell/enterValue/eraseSelected/undo: Task 3.
// snapshot/restore: Task 4.

} // namespace sudoku
