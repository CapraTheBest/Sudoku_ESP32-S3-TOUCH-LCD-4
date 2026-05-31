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

uint32_t GameSession::elapsedMs() const {
    uint32_t e = accumMs_;
    if (state_ == GameState::Playing) e += clock_() - runStart_;
    return e;
}

void GameSession::pause() {
    if (state_ != GameState::Playing) return;
    accumMs_ += clock_() - runStart_;
    state_ = GameState::Paused;
}

void GameSession::resume() {
    if (state_ != GameState::Paused) return;
    runStart_ = clock_();
    state_ = GameState::Playing;
}
void GameSession::selectCell(int idx) {
    if (state_ != GameState::Playing) return;
    if (idx < -1 || idx >= CELLS) return;
    selected_ = idx;
}

void GameSession::enterValue(uint8_t v) {
    if (state_ != GameState::Playing) return;
    if (selected_ < 0) return;
    if (v > 9) return;
    if (!board_.setValue(selected_, v)) return;   // cella data o valore non valido
    if (board_.isComplete() && board_.isSolved()) {
        accumMs_ += clock_() - runStart_;          // congela il cronometro
        state_ = GameState::Won;
    }
}

void GameSession::eraseSelected() { enterValue(0); }

void GameSession::toggleNote(uint8_t d) {
    if (state_ != GameState::Playing) return;
    if (selected_ < 0) return;
    board_.toggleNote(selected_, d);
}

void GameSession::undo() {
    if (state_ != GameState::Playing) return;
    board_.undo();
}
GameSession::Snapshot GameSession::snapshot() const {
    Snapshot s;
    for (int i = 0; i < CELLS; i++) {
        s.solution[i] = board_.solutionAt(i);
        s.value[i]    = board_.value(i);
        s.given[i]    = board_.isGiven(i);
        s.notes[i]    = board_.notes(i);
    }
    s.elapsedMs   = elapsedMs();
    s.difficulty  = (uint8_t) diff_;
    s.valid       = (state_ == GameState::Playing || state_ == GameState::Paused);
    return s;
}

bool GameSession::restore(const Snapshot &s) {
    if (!s.valid) return false;
    board_.reset(s.solution, s.given);
    // Nota: i valori utente vengono riapplicati con setValue, quindi finiscono
    // nello stack di undo della board. Dopo il restore si e' in Paused e undo()
    // e' attivo solo in Playing, quindi non sorprende l'utente prima del resume.
    for (int i = 0; i < CELLS; i++)
        if (!s.given[i] && s.value[i] != 0) board_.setValue(i, s.value[i]);
    // Gli appunti vanno riapplicati DOPO i valori: setValue azzera gli appunti
    // della propria cella, quindi ripristinarli prima li perderebbe.
    for (int i = 0; i < CELLS; i++) board_.setNotesMask(i, s.notes[i]);
    diff_     = (Difficulty) s.difficulty;
    accumMs_  = s.elapsedMs;
    selected_ = -1;
    state_    = GameState::Paused;
    return true;
}

} // namespace sudoku
