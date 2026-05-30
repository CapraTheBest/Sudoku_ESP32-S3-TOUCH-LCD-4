#pragma once
#include "game_session.h"

// UI LVGL del Sudoku: menu, gioco (layout C*), pausa, vittoria.
// Da chiamare con il lock LVGL gia' preso.
namespace ui {

// Inizializza la UI sulla sessione data e mostra il menu iniziale.
void init(sudoku::GameSession *session);

} // namespace ui
