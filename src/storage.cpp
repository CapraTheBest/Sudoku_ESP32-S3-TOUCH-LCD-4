#include "storage.h"

#include <Arduino.h>
#include <Preferences.h>

#include "config.h"

namespace storage {

using Snapshot = sudoku::GameSession::Snapshot;

static Preferences prefs;

static const char *KEY_GAME       = "game_blob";
static const char *KEY_BEST_EASY  = "best_easy";
static const char *KEY_BEST_MED   = "best_medium";
static const char *KEY_BEST_HARD  = "best_hard";

void begin() {
    // RW. Il namespace viene creato al primo accesso.
    prefs.begin(NVS_NAMESPACE, false);
}

// --- Validazione del payload letto da NVS ---------------------------------
// Snapshot::valid e' solo un flag del produttore: un blob corrotto potrebbe
// averlo a true. Validiamo qui il contenuto prima di fidarci.
static bool isValidSnapshot(const Snapshot &s) {
    if (!s.valid) return false;
    if (s.difficulty > (uint8_t) sudoku::Difficulty::Hard) return false;
    for (int i = 0; i < sudoku::CELLS; i++) {
        if (s.solution[i] < 1 || s.solution[i] > 9) return false;  // soluzione piena
        if (s.value[i] > 9) return false;                          // 0 = vuota
        if (s.given[i] && s.value[i] != s.solution[i]) return false; // given coerenti
    }
    return true;
}

bool hasSavedGame() {
    Snapshot tmp;
    return loadGame(tmp);
}

void saveGame(const Snapshot &s) {
    prefs.putBytes(KEY_GAME, &s, sizeof(Snapshot));
}

bool loadGame(Snapshot &out) {
    if (!prefs.isKey(KEY_GAME)) return false;
    if (prefs.getBytesLength(KEY_GAME) != sizeof(Snapshot)) return false;
    size_t n = prefs.getBytes(KEY_GAME, &out, sizeof(Snapshot));
    if (n != sizeof(Snapshot)) return false;
    return isValidSnapshot(out);
}

void clearSavedGame() {
    if (prefs.isKey(KEY_GAME)) prefs.remove(KEY_GAME);
}

// --- Record ----------------------------------------------------------------
static const char *bestKey(sudoku::Difficulty d) {
    switch (d) {
        case sudoku::Difficulty::Easy:   return KEY_BEST_EASY;
        case sudoku::Difficulty::Medium: return KEY_BEST_MED;
        case sudoku::Difficulty::Hard:   return KEY_BEST_HARD;
    }
    return KEY_BEST_MED;
}

uint32_t bestTime(sudoku::Difficulty d) {
    return prefs.getUInt(bestKey(d), 0);
}

void setBestTime(sudoku::Difficulty d, uint32_t seconds) {
    prefs.putUInt(bestKey(d), seconds);
}

} // namespace storage
