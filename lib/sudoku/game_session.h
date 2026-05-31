#pragma once
#include <cstdint>
#include "sudoku_types.h"
#include "sudoku_board.h"

namespace sudoku {

// Clock monotono iniettabile (ms). Device: millis(). Test: contatore finto.
using ClockFn = uint32_t (*)();

enum class GameState : uint8_t { Menu, Playing, Paused, Won };

class GameSession {
public:
    GameSession(RandFn rnd, ClockFn clock);

    GameState  state() const;
    const Board& board() const;
    Difficulty difficulty() const;
    int        selectedCell() const;   // -1 = nessuna

    void newGame(Difficulty d);        // genera, timer a 0, stato Playing

    // Input (ignorato se non Playing)
    void selectCell(int idx);          // idx in [0, CELLS); -1 deseleziona
    void enterValue(uint8_t v);        // 1..9 nella cella selezionata; vittoria -> Won
    void eraseSelected();              // = enterValue(0)
    void toggleNote(uint8_t d);        // appunto d (1..9) nella cella selezionata
    void undo();

    void pause();                      // Playing -> Paused (ferma il tempo)
    void resume();                     // Paused  -> Playing (riprende il tempo)

    uint32_t elapsedMs() const;        // tempo di gioco escluse le pause

    // Persistenza (gestita dal firmware nel Piano C)
    struct Snapshot {
        uint8_t  solution[CELLS];
        uint8_t  value[CELLS];
        bool     given[CELLS];
        uint16_t notes[CELLS];
        uint32_t elapsedMs;
        uint8_t  difficulty;
        bool     valid;                // true solo se Playing/Paused
    };
    // NB: `valid` e' un flag del PRODUTTORE ("c'era una partita da salvare"),
    // NON un controllo di integrita' del contenuto. Il firmware che
    // deserializza uno Snapshot da NVS (Piano C) deve validare il payload
    // (cifre 1..9, given coerenti, difficulty in range) prima di fidarsi:
    // restore() non e' un confine di sicurezza.
    Snapshot snapshot() const;
    bool     restore(const Snapshot& s);  // -> Paused; false se snapshot non valido

private:
    RandFn     rnd_;
    ClockFn    clock_;
    Board      board_;
    GameState  state_;
    Difficulty diff_;
    int        selected_;
    uint32_t   accumMs_;   // tempo accumulato prima dell'ultimo resume
    uint32_t   runStart_;  // clock_() all'ultimo start/resume
};

} // namespace sudoku
