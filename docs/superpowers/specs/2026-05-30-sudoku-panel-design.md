# Sudoku Panel — Documento di design

- **Data:** 2026-05-30
- **Autore:** Alessandro Petrolati
- **Stato:** approvato (brainstorming)
- **Board:** Waveshare ESP32-S3-Touch-LCD-4

> **Regola di progetto:** questo è un progetto separato. Vive solo nella cartella
> `Sudoku_ESP32-S3-TOUCH-LCD-4` (repo git proprio). I file condivisi con lo Smart
> Home Panel vengono **copiati**, mai referenziati o modificati nell'altro progetto.

## 1. Obiettivo e scope

Firmware standalone per giocare a **Sudoku** su pannello touch ESP32-S3. Schemi
generati casualmente sul dispositivo, cronometro di gioco e funzione di pausa.

**Hardware:** ESP32-S3 (dual core 240 MHz), 8 MB PSRAM OPI, 16 MB Flash QIO,
LCD 480×480 RGB ST7701, touch GT911, IO expander TCA9554.

**Fuori scope (esclusi esplicitamente):** WiFi, web server, Home Assistant,
qualunque connettività di rete. Niente note/matita, niente evidenziazione errori
in tempo reale, niente suggerimenti (hint).

## 2. Riuso dallo Smart Home Panel

Copiati così com'è (layer hardware/display):

- `lvgl_port_v8.cpp` / `lvgl_port_v8.h` — porting LVGL su display RGB ESP32-S3
- `ESP_Panel_Board_Custom.h`, `ESP_Panel_Board_Supported.h`, `ESP_Panel_Conf.h`
- `lv_conf.h` — configurazione LVGL 8.4
- `partitions.csv` — layout flash 16 MB

`platformio.ini` ricreato (non copiato) con:

- platform pioarduino `51.03.07` (vincolo noto: `ESP32_Display_Panel` v0.1.8 richiede
  Arduino-ESP32 3.0.7 + ESP-IDF v5.1)
- stessi flag board (PSRAM OPI, flash QIO 16 MB, partizioni)
- `lib_deps` ridotto: `lvgl @ ^8.4.0`, `ESP32_Display_Panel #v0.1.8`,
  `ESP32_IO_Expander #v0.0.4`
- **rimossi:** ArduinoJson, WebSockets, ESPAsyncWebServer, AsyncTCP

Backlight: ON via TCA9554 EXIO1 (no PWM, vincolo HW noto). Nessun dimming.

Senza lo stack di rete l'heap è abbondante: nessuno dei problemi di memoria del
progetto Smart Home Panel.

## 3. Architettura a moduli

Separazione netta tra **motore di gioco** (C++ puro, nessuna dipendenza LVGL/Arduino,
testabile su PC) e **UI** (LVGL).

### Motore (logica pura)

- **`sudoku_solver`** — risolutore backtracking; conta le soluzioni (early-exit a 2)
  per verificare l'unicità.
- **`sudoku_generator`** — genera una griglia piena valida (backtracking randomizzato),
  poi rimuove celle finché la soluzione resta unica, calibrando sul livello.
- **`sudoku_board`** — stato della griglia:
  - `solution[81]` (uint8) — soluzione completa
  - maschera `given[81]` (bool) — celle iniziali non modificabili
  - `value[81]` (uint8, 0 = vuota) — valori correnti
  - rilevamento conflitti riga/colonna/blocco (per il lampeggio rosso al completamento)
  - `isComplete()` (tutte piene), `isSolved()` (== solution)
  - **stack di undo**: lista di `(index, oldValue)`

### Sessione / controller

- **`game_session`** — macchina a stati `MENU → PLAYING → PAUSED → WON`; cronometro
  basato su `millis()` con accumulo del tempo e stop in pausa; gestione input
  (seleziona cella, inserisci numero, cancella, undo, pausa, nuova partita).

### Persistenza

- **`storage`** (NVS via `Preferences`) — salva/carica la partita in corso
  (solution, given, value, tempo trascorso, livello) e il miglior tempo per livello.

### UI (LVGL) — uno schermo per file

- **`ui_theme`** — palette scura centralizzata (sfondo scuro, numeri chiari, accento blu).
- **`ui_menu`** — menu iniziale.
- **`ui_game`** — schermata di gioco (layout C★).
- **`ui_pause`** — overlay di pausa.
- **`ui_win`** — schermata di vittoria.

### main

- **`main.cpp`** — init hardware (panel + IO expander + backlight), init LVGL port,
  init NVS; mostra il menu; nel `loop()` esegue `lv_timer_handler()` + tick cronometro.

## 4. Generazione schemi e difficoltà

Generazione **on-device, infinita**, con **soluzione unica garantita**.

Algoritmo:
1. Riempi la griglia con backtracking randomizzato → griglia piena valida (= soluzione).
2. Scava: ordina le celle in modo casuale; per ciascuna prova a svuotarla e verifica
   con `sudoku_solver` che la soluzione resti **unica**; se sì mantieni vuota, altrimenti
   ripristina. Continua fino al numero di givens bersaglio o all'esaurimento delle celle.

Difficoltà per numero di celle iniziali:

| Livello    | Givens (circa) |
|------------|----------------|
| Facile     | 40–45          |
| Medio      | 32–36          |
| Difficile  | 26–30          |

**Trade-off:** uso il numero di givens come proxy della difficoltà, non la
classificazione delle tecniche umane (naked/hidden singles, X-wing, ...). È più
semplice e robusto e correla bene con la difficoltà percepita. Lo scavo ha un tetto
di tentativi: se per "Difficile" non si scende sotto soglia mantenendo l'unicità,
ci si ferma a qualche given in più (lo schema resta valido e a soluzione unica).

Durante la generazione la UI mostra un breve indicatore "Genero…" (tipicamente <1 s).

## 5. Schermata di gioco (layout C★)

- **Top bar**: tempo `mm:ss` a sinistra; ⏸ pausa e ＋ nuova partita a destra.
- **Griglia 9×9** massimizzata (celle ~40 px), separatori 3×3 marcati, dati iniziali
  graficamente distinti e non modificabili.
- **Tastierino fisso** in basso: `1–9` + ⌫ (cancella). Niente popup, niente occlusione.

Interazione **cella-prima**: si tocca la cella (evidenziata con riga/colonna/blocco),
poi si tocca il numero. La cella iniziale non è modificabile.

Aiuti attivi:
- **Annulla (undo)**: annulla l'ultima mossa (inserimento o cancellazione).
- **Evidenzia uguali**: selezionando una cella con valore N, tutte le celle con N
  vengono evidenziate.

Esclusi: evidenziazione errori in tempo reale, note/matita, suggerimenti.

## 6. Vittoria e verifica

Il controllo scatta **solo a griglia completa** (81 celle piene):

- se `value == solution` → stato **WON**: schermata vittoria con tempo finale; se è
  il miglior tempo per quel livello, evidenzia "Nuovo record!".
- se piena ma con errori → messaggio transitorio **"Ci sono errori"** e le celle in
  conflitto **lampeggiano in rosso** per ~1–2 s, poi si continua a giocare.

Questo è l'unico feedback sugli errori (coerente con la scelta "Sudoku puro": nessun
evidenziatore permanente).

## 7. Pausa, menu, persistenza

- **Pausa**: ferma il cronometro e **oscura la griglia** (overlay "In pausa — tocca per
  riprendere"). Impedisce di ragionare/sbirciare a tempo fermo.
- **Menu iniziale** (all'avvio): "Nuova partita" → scelta Facile/Medio/Difficile;
  "Riprendi partita" se presente uno stato salvato; visualizzazione dei record per livello.
- **Riprendi dopo spegnimento**: alla pausa e all'uscita dalla partita, lo stato viene
  salvato in NVS (solution, given, value, tempo trascorso, livello). Al riavvio il menu
  offre "Riprendi".
- **Tema scuro**, palette centralizzata in `ui_theme`.

### Schema NVS

Namespace: `sudoku_v1`

- `game_blob` — stato partita in corso (struct serializzata: solution[81], given bitmask,
  value[81], elapsed_ms, difficulty, valid flag)
- `best_easy`, `best_medium`, `best_hard` — miglior tempo (secondi, uint32; 0 = nessun record)

## 8. Test

Il motore (`sudoku_solver`, `sudoku_generator`, `sudoku_board`) è C++ puro e viene
testato con l'ambiente PlatformIO **`[env:native]`** (Unity) eseguibile su PC senza
hardware:

- il solver risolve correttamente griglie note;
- ogni schema generato ha **soluzione unica** (il conteggio soluzioni del solver dà 1);
- il numero di givens rientra nella fascia del livello richiesto;
- la logica di undo ripristina lo stato corretto;
- `isSolved()` riconosce vittoria e respinge griglie complete ma errate.

La UI LVGL si verifica sul dispositivo.

## 9. Struttura del progetto (prevista)

```
Sudoku_ESP32-S3-TOUCH-LCD-4/
├── platformio.ini
├── partitions.csv
├── include/
│   ├── lv_conf.h
│   ├── ESP_Panel_Board_Custom.h
│   ├── ESP_Panel_Board_Supported.h
│   ├── ESP_Panel_Conf.h
│   ├── lvgl_port_v8.h
│   ├── config.h
│   ├── sudoku_solver.h / sudoku_generator.h / sudoku_board.h
│   ├── game_session.h
│   ├── storage.h
│   └── ui_theme.h / ui_menu.h / ui_game.h / ui_pause.h / ui_win.h
├── src/
│   ├── main.cpp
│   ├── lvgl_port_v8.cpp
│   ├── sudoku_solver.cpp / sudoku_generator.cpp / sudoku_board.cpp
│   ├── game_session.cpp
│   ├── storage.cpp
│   └── ui_theme.cpp / ui_menu.cpp / ui_game.cpp / ui_pause.cpp / ui_win.cpp
├── test/
│   └── test_engine/      (Unity, env:native)
├── mockups/              (mockup HTML del brainstorming)
└── docs/
```
