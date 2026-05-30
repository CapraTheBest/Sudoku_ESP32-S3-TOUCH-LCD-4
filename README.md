# Sudoku Panel

Gioco del **Sudoku** per il pannello touch **Waveshare ESP32-S3-Touch-LCD-4** (480×480).
Schemi casuali generati **on-device** a soluzione unica, cronometro con pausa,
salvataggio della partita e tempi record — il tutto in un firmware standalone,
senza WiFi né dipendenze cloud.

> Stesso display del progetto *Smart Home Panel*, ma progetto e repository separati.

---

## Caratteristiche

- 🎲 **Schemi infiniti on-device** — generati a runtime con **soluzione unica garantita** (backtracking + verifica di unicità). Nessun puzzle pre-caricato, nessuna ripetizione.
- 🎚️ **Tre livelli** — Facile / Medio / Difficile (calibrati sul numero di indizi).
- ⏱️ **Cronometro** — conta solo il tempo di gioco; si ferma in pausa e alla vittoria.
- ⏸️ **Pausa** — oscura la griglia e ferma il tempo ("tocca per riprendere"), così non si può sbirciare a cronometro fermo.
- 💾 **Riprendi partita** — lo stato viene salvato in NVS: dopo uno spegnimento puoi riprendere da dove eri.
- 🏆 **Tempi record** — miglior tempo memorizzato per ogni livello.
- ↩️ **Annulla** e **evidenziazione** della cella selezionata (riga / colonna / blocco) e dei numeri uguali.
- 🌙 **Tema scuro**, layout ottimizzato per il touch capacitivo (griglia massimizzata + tastierino fisso).

## Hardware

| | |
|---|---|
| Board | Waveshare **ESP32-S3-Touch-LCD-4** (ESP32-S3 N16R8) |
| Display | 480×480 IPS, controller **ST7701** (bus RGB) |
| Touch | **GT911** capacitivo (I²C) |
| IO expander | **TCA9554** (reset LCD/touch, backlight) |
| Memoria | 16 MB Flash QIO · 8 MB PSRAM OPI |

## Architettura

Separazione netta tra **logica pura** (C++ portabile, testata su PC) e **firmware** (Arduino/LVGL):

```
lib/sudoku/            motore + sessione (C++ puro, NIENTE Arduino/LVGL)
  sudoku_solver        risolutore backtracking + conteggio soluzioni (unicità)
  sudoku_generator     generazione griglia piena + scavo a soluzione unica
  sudoku_board         stato griglia: dati iniziali, valori, undo, conflitti, vittoria
  game_session         macchina a stati + cronometro + input + snapshot save/restore
src/
  main.cpp             bring-up hardware (IO expander, backlight, LVGL) + loop
  ui.cpp               UI LVGL: menu, gioco, pausa, vittoria
  storage.cpp          persistenza NVS (partita + record), con validazione del payload
  lvgl_port_v8.cpp     porting LVGL ↔ display RGB (da Espressif/Waveshare)
include/               header (config, tema, board ESP_Panel, lv_conf)
test/                  test unitari Unity del motore e della sessione (ambiente nativo)
```

La logica di gioco non dipende dall'hardware: gira sia sul PC (per i test) sia sul device.

## Build & flash

Richiede [PlatformIO](https://platformio.org/). La build del device usa il fork
**pioarduino** (Arduino-ESP32 3.0.7 / ESP-IDF v5.1), richiesto da `ESP32_Display_Panel`.

```bash
# Compila il firmware
pio run -e esp32-s3-touch-lcd-4

# Compila e flasha sul pannello (USB-C)
pio run -e esp32-s3-touch-lcd-4 -t upload

# Monitor seriale
pio device monitor -b 115200
```

> La prima build scarica la toolchain (~200 MB) e può richiedere parecchi minuti;
> le successive sono nell'ordine dei ~2 minuti.

## Test (su PC, senza hardware)

Il motore e la sessione sono coperti da test unitari **Unity** eseguibili nativamente:

```bash
pio test -e native
```

Serve un compilatore C++ host nel PATH (es. MinGW-w64 / `g++`).

## Comandi (sintesi)

| Azione | Tocco |
|---|---|
| Scegliere il livello | bottoni nel menu |
| Selezionare una cella | tocca la cella |
| Inserire un numero | tasti `1`–`9` del tastierino |
| Cancellare | tasto ⌫ |
| Pausa / nuova partita | ⏸ / ＋ nella barra in alto |
| Riprendere dopo la pausa | tocca lo schermo |

## Struttura del progetto

- `docs/superpowers/specs/` — documento di design
- `docs/superpowers/plans/` — piani di implementazione (motore, sessione, firmware)
- `mockups/` — mockup HTML dei layout esplorati in fase di design (layout scelto: **C★**)
- `scripts/sermon.py` — cattura "bounded" del monitor seriale (utile per debug)

## Licenza

Codice del progetto rilasciato sotto licenza **MIT** — vedi [LICENSE](LICENSE).

Il layer di porting del display (`src/lvgl_port_v8.cpp`, `include/lvgl_port_v8.h`,
`include/ESP_Panel_*.h`) deriva dagli esempi Espressif/Waveshare ed è soggetto alle
rispettive licenze (CC0-1.0 / Apache-2.0) indicate nelle intestazioni dei file.
[LVGL](https://lvgl.io) è distribuito sotto licenza MIT.
