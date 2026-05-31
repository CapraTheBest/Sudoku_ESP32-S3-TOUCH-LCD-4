#pragma once
#include <lvgl.h>

// Palette scura centralizzata per la UI del Sudoku.
namespace theme {

inline lv_color_t bg()      { return lv_color_hex(0x0f1115); } // sfondo schermo
inline lv_color_t panel()   { return lv_color_hex(0x171a21); } // barre/pannelli
inline lv_color_t cell()    { return lv_color_hex(0x222732); } // cella normale
inline lv_color_t peer()    { return lv_color_hex(0x2a3142); } // riga/colonna/blocco della selezione (slate neutro)
inline lv_color_t same()    { return lv_color_hex(0x4a3d6b); } // stesso numero selezionato (viola, distinto)
inline lv_color_t cellSel() { return lv_color_hex(0x3b6fe0); } // cella selezionata (blu acceso)
inline lv_color_t grid()    { return lv_color_hex(0x384055); } // linee griglia
inline lv_color_t thick()   { return lv_color_hex(0x5a6f9a); } // separatori 3x3
inline lv_color_t ink()     { return lv_color_hex(0xe8ebf0); } // testo chiaro / dati iniziali
inline lv_color_t userNum() { return lv_color_hex(0x6db0ff); } // numeri inseriti dall'utente
inline lv_color_t muted()   { return lv_color_hex(0x8b93a3); } // testo secondario
inline lv_color_t noteInk() { return lv_color_hex(0x9aa3b5); } // appunti (pencil marks)
inline lv_color_t accent()  { return lv_color_hex(0x5b8cff); } // accento
inline lv_color_t accent2() { return lv_color_hex(0x23304d); } // bottoni soft
inline lv_color_t danger()  { return lv_color_hex(0xe5484d); } // conflitti / errori
inline lv_color_t good()    { return lv_color_hex(0x2ecc71); } // vittoria / record

} // namespace theme
