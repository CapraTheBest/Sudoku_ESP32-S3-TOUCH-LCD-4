#pragma once

#include <cstdint>
#include "game_session.h"
#include "sudoku_types.h"

// Persistenza su NVS (Preferences): partita in corso + miglior tempo per livello.
// Namespace: vedi NVS_NAMESPACE in config.h.
namespace storage {

void begin();

// --- Partita in corso ---
bool hasSavedGame();
void saveGame(const sudoku::GameSession::Snapshot &s);
// Carica e VALIDA il payload (cifre, difficolta', flag). False se assente/corrotto.
bool loadGame(sudoku::GameSession::Snapshot &out);
void clearSavedGame();

// --- Record (secondi; 0 = nessun record) ---
uint32_t bestTime(sudoku::Difficulty d);
void     setBestTime(sudoku::Difficulty d, uint32_t seconds);

// --- Lingua (0 = EN, 1 = IT; 0xFF = non impostata) ---
uint8_t language();
void    setLanguage(uint8_t lang);

} // namespace storage
