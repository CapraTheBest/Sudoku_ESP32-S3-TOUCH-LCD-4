# Firmware Integration Implementation Plan (Piano C di 3)

> **For agentic workers:** REQUIRED SUB-SKILL: superpowers:subagent-driven-development. Steps use checkbox (`- [ ]`) syntax.
> **Verifica:** questo piano si compila con la toolchain pioarduino (prima build ~30 min) e si verifica sul **device reale** (flash USB-C + ispezione visiva). I worker possono al massimo verificare la **compilazione** con `pio run -e esp32-s3-touch-lcd-4`.

**Goal:** Integrare motore + `GameSession` in un firmware giocabile sul Waveshare ESP32-S3-Touch-LCD-4: bring-up display, persistenza NVS, UI LVGL (menu/game/pause/win, layout C★).

**Architecture:** Riusa `lib/sudoku/` (motore + sessione, già testati). Il layer hardware/display è copiato dallo Smart Home Panel (`lvgl_port_v8`, header `ESP_Panel_*`, `lv_conf.h`, `partitions.csv`). `GameSession` riceve `millis` come `ClockFn` ed un RNG basato su `esp_random`. La UI legge lo stato dalla sessione e le inoltra gli input. `storage` serializza lo `Snapshot` su NVS.

**Tech Stack:** Arduino-ESP32 3.0.7 (pioarduino 51.03.07), LVGL 8.4, ESP32_Display_Panel 0.1.8, ESP32_IO_Expander 0.0.4, Preferences (NVS).

---

## File Structure
- `platformio.ini` — aggiungere `[env:esp32-s3-touch-lcd-4]` (lasciare `[env:native]`).
- HW copiati: `src/lvgl_port_v8.cpp`, `include/lvgl_port_v8.h`, `include/ESP_Panel_Board_*.h`, `include/ESP_Panel_Conf.h`, `include/lv_conf.h`, `partitions.csv`.
- `include/config.h` — versione, baud, namespace NVS.
- `src/main.cpp` — bring-up (IO expander + backlight EXIO1 + lvgl_port_init), orchestrazione schermate, loop.
- `include/storage.h` + `src/storage.cpp` — NVS: save/load Snapshot (con validazione), best time per livello.
- `include/ui_theme.h` (+ `src/ui_theme.cpp`) — palette scura + font.
- `src/ui_menu.cpp` / `ui_game.cpp` / `ui_pause.cpp` / `ui_win.cpp` (+ header) — schermate.

---

## PARTE 1 — Fondazione device (implementata in questa sessione)

### Task 1: Scaffolding + bring-up + smoke test
- [x] Copiare i file HW dallo Smart Home Panel (vedi File Structure).
- [x] `platformio.ini`: env device (pioarduino 51.03.07, PSRAM opi/qio_opi, flash 16MB qio, partitions.csv, build_flags BOARD_HAS_PSRAM/USB_CDC/USB_MODE/LV_CONF_*, `-I include`; lib_deps lvgl@^8.4.0 + ESP32_Display_Panel#v0.1.8 + ESP32_IO_Expander#v0.0.4).
- [x] `include/config.h`.
- [x] `src/main.cpp`: sequenza Waveshare (TCA9554 I2C SDA8/SCL9 addr_000; touch INT GPIO16 LOW; EXIO5 HIGH, EXIO0/2 LOW reset, delay 200, EXIO5 LOW, EXIO2/0 HIGH; **EXIO1 HIGH = backlight**); `ESP_Panel` init/begin (+config RGB se avoid-tear); `lvgl_port_init`; splash "SUDOKU" + versione; **smoke test**: crea `GameSession(esp_random-rnd, millis)`, `newGame(Medium)`, mostra il numero di indizi generati (prova che motore+sessione linkano e girano on-device).
- [ ] **Verifica:** `pio run -e esp32-s3-touch-lcd-4` compila; flash; sul pannello si accende, mostra lo splash e "Indizi: N".

### Task 2: storage (NVS)
- [x] `storage`: `begin()`, `hasSavedGame()`, `saveGame(Snapshot)`, `loadGame(Snapshot&)` **con validazione del payload** (valid, difficulty<3, solution 1..9, value 0..9), `clearSavedGame()`, `bestTime(d)`/`setBestTime(d,sec)`. Namespace `sudoku_v1`, chiavi `game_blob`, `best_easy|medium|hard`.
- [ ] **Verifica:** compila; funzionalmente provato con la UI (Parte 2).

---

## PARTE 2 — UI di gioco (blocco successivo)

> Da dettagliare ed eseguire dopo che la Parte 1 compila/parte. Schema layout: **C★** (mockup in `mockups/02-layout-v2.html`). Font: timer Montserrat 48, celle 28, etichette 18/24 (già in `lv_conf.h`).

### Task 3: ui_theme — palette scura centralizzata (colori sfondo/celle/given/user/accento/conflitto) + helper font.

### Task 4: ui_game — schermata di gioco (layout C★)
- Top bar: label tempo `mm:ss` (aggiornata da `lv_timer` ogni ~250ms da `session.elapsedMs()`), bottoni ⏸ pausa e ＋ nuova.
- Griglia 9×9: 81 celle (label in container), separatori 3×3 marcati, given non toccabili e distinti, evidenziazione cella selezionata + riga/colonna/blocco + numeri uguali.
- Tastierino fisso 1–9 + ⌫.
- Eventi → `session.selectCell/enterValue/eraseSelected`. Dopo ogni inserimento: se `state()==Won` → schermata vittoria; se griglia piena ma non risolta → flash rosso celle in conflitto (`board().conflicts`).
- Funzione `refresh()` che ridisegna dalla board.

### Task 5: ui_menu — titolo, 3 bottoni difficoltà (→ `session.newGame` + schermata gioco), bottone "Riprendi" (se `storage::hasSavedGame()` → `loadGame`+`session.restore` → schermata gioco in pausa), record per livello.

### Task 6: ui_pause — overlay che oscura la griglia, "In pausa — tocca per riprendere"; tap → `session.resume()` + torna al gioco.

### Task 7: ui_win — tempo finale, "Nuovo record!" se `elapsedMs/1000 < bestTime`, aggiorna `setBestTime`, bottone nuova partita / menu.

### Task 8: main wiring — macchina schermate (Menu/Game/Pause/Won), salva `snapshot()` su NVS a pausa e all'uscita, cancella il salvataggio a vittoria; ClockFn=millis.

---

## Done criteria (Piano C)
- `pio run -e esp32-s3-touch-lcd-4` compila senza errori.
- Sul device: menu → scelta livello → partita giocabile col dito; timer scorre; pausa oscura e ferma; vittoria mostra tempo+record; riavvio offre "Riprendi". Verifica visiva dell'utente.
