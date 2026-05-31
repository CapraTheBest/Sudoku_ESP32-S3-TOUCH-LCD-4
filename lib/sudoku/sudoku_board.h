#pragma once
#include <cstdint>
#include <vector>
#include "sudoku_types.h"

namespace sudoku {

// Stato di una partita: soluzione completa, maschera dei dati iniziali,
// valori correnti dell'utente e cronologia per l'undo.
class Board {
public:
    Board();

    // Inizializza: value = solution dove given, 0 altrove. Svuota l'undo.
    void reset(const uint8_t solution[CELLS], const bool given[CELLS]);

    uint8_t value(int idx) const;        // 0 = vuota
    uint8_t solutionAt(int idx) const;
    bool    isGiven(int idx) const;

    // Imposta value[idx] (val 0..9, 0 = cancella). No-op su cella data.
    // Ritorna true se la mossa è stata applicata (e registrata per l'undo).
    // Inserire una cifra (val 1..9) azzera gli appunti di quella cella.
    bool setValue(int idx, uint8_t val);

    // --- Appunti (pencil marks) -------------------------------------------
    // Maschera di bit dei candidati: bit (d-1) acceso = candidato d presente.
    uint16_t notes(int idx) const;
    void     setNotesMask(int idx, uint16_t mask);   // usato dal restore
    // Inverte il candidato d (1..9) nella cella. Solo su cella vuota non-data.
    // Ritorna true se l'appunto è stato modificato.
    bool     toggleNote(int idx, uint8_t d);

    bool canUndo() const;
    bool undo();                          // ripristina l'ultima mossa

    bool isComplete() const;              // tutte le celle non-zero
    bool isSolved() const;                // completa e uguale a solution

    // true se la cifra d (1..9) è piazzata correttamente in tutte e 9 le sue
    // posizioni (9 celle con valore d, ognuna uguale alla soluzione). Usato
    // per disabilitare il tasto del tastierino quando la cifra è completata.
    bool isDigitComplete(uint8_t d) const;

    // Marca in out[81] le celle in conflitto (duplicato in riga/colonna/blocco).
    // Ritorna il numero di celle in conflitto.
    int conflicts(bool out[CELLS]) const;

    int filledCount() const;
    int givenCount() const;

private:
    uint8_t  solution_[CELLS];
    uint8_t  value_[CELLS];
    bool     given_[CELLS];
    uint16_t notes_[CELLS];
    struct Move { int idx; uint8_t oldVal; };
    std::vector<Move> history_;
};

} // namespace sudoku
