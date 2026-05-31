#include "ui.h"

#include <lvgl.h>
#include <cstdint>
#include <cstdio>

#include "ui_theme.h"
#include "storage.h"

namespace ui {

static sudoku::GameSession *S = nullptr;

// forward
static void showMenu();
static void showGame();
static void showPause();
static void showWin();

// ===================== stato schermata di gioco =====================
static lv_obj_t *g_cell[81];
static lv_obj_t *g_cellLabel[81];
static lv_obj_t *g_numBtn[10];          // tasti tastierino 1..9 (indice = cifra)
static lv_obj_t *g_notesBtn = nullptr;  // toggle modalita' appunti
static bool      g_notesMode = false;   // true = i tasti scrivono appunti
static lv_obj_t *g_timerLabel = nullptr;
static lv_timer_t *g_tick = nullptr;
static lv_timer_t *g_flash = nullptr;

static const int CELL = 38;       // lato cella (px)
static const int BLOCKGAP = 4;    // gap extra ai confini 3x3
static int cellX(int c) { return c * CELL + (c / 3) * BLOCKGAP; }
static int cellY(int r) { return r * CELL + (r / 3) * BLOCKGAP; }

static void fmtTime(uint32_t ms, char *buf, size_t n) {
    uint32_t s = ms / 1000;
    snprintf(buf, n, "%02u:%02u", (unsigned)(s / 60), (unsigned)(s % 60));
}

static const char *difName(sudoku::Difficulty d) {
    switch (d) {
        case sudoku::Difficulty::Easy:   return "Facile";
        case sudoku::Difficulty::Medium: return "Medio";
        case sudoku::Difficulty::Hard:   return "Difficile";
    }
    return "Medio";
}

static void killTimers() {
    if (g_tick)  { lv_timer_del(g_tick);  g_tick = nullptr; }
    if (g_flash) { lv_timer_del(g_flash); g_flash = nullptr; }
}

// stile comune: niente bordo/pad/scroll, raggio piccolo
static void plainStyle(lv_obj_t *o, lv_color_t bg, int radius) {
    lv_obj_set_style_bg_color(o, bg, 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(o, 0, 0);
    lv_obj_set_style_radius(o, radius, 0);
    lv_obj_set_style_pad_all(o, 0, 0);
    lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE);
}

// Costruisce la stringa 3x3 degli appunti ("1 2 3\n4 5 6\n7 8 9", assente = spazio).
static void buildNotesText(uint16_t mask, char *buf) {
    int p = 0;
    for (int r = 0; r < 3; r++) {
        for (int c = 0; c < 3; c++) {
            int d = r * 3 + c + 1;
            buf[p++] = (mask & (1u << (d - 1))) ? (char)('0' + d) : ' ';
            if (c < 2) buf[p++] = ' ';
        }
        if (r < 2) buf[p++] = '\n';
    }
    buf[p] = 0;
}

// ===================== refresh griglia =====================
static void refreshGame() {
    if (!S) return;
    int sel = S->selectedCell();
    int selR = sel >= 0 ? sel / 9 : -1;
    int selC = sel >= 0 ? sel % 9 : -1;
    uint8_t selVal = sel >= 0 ? S->board().value(sel) : 0;

    char t[2] = {0, 0};
    char notesBuf[16];
    for (int i = 0; i < 81; i++) {
        uint8_t v = S->board().value(i);
        uint16_t notes = S->board().notes(i);
        if (v) {
            t[0] = (char)('0' + v);
            lv_label_set_text(g_cellLabel[i], t);
            lv_obj_set_style_text_font(g_cellLabel[i], &lv_font_montserrat_28, 0);
            lv_obj_set_style_text_color(g_cellLabel[i],
                S->board().isGiven(i) ? theme::ink() : theme::userNum(), 0);
        } else if (notes) {
            buildNotesText(notes, notesBuf);
            lv_label_set_text(g_cellLabel[i], notesBuf);
            lv_obj_set_style_text_font(g_cellLabel[i], &lv_font_montserrat_12, 0);
            lv_obj_set_style_text_line_space(g_cellLabel[i], -1, 0);
            lv_obj_set_style_text_color(g_cellLabel[i], theme::noteInk(), 0);
        } else {
            lv_label_set_text(g_cellLabel[i], "");
        }

        int r = i / 9, c = i % 9;
        lv_color_t bg = theme::cell();
        bool peer = (sel >= 0) &&
                    (r == selR || c == selC ||
                     ((r / 3) == (selR / 3) && (c / 3) == (selC / 3)));
        bool same = (selVal > 0 && v == selVal);
        if (peer) bg = theme::peer();
        if (same) bg = theme::same();
        if (i == sel) bg = theme::cellSel();
        lv_obj_set_style_bg_color(g_cell[i], bg, 0);
    }

    // Tasti del tastierino: spegni la cifra completata (9 piazzamenti corretti).
    for (int d = 1; d <= 9; d++) {
        if (!g_numBtn[d]) continue;
        bool done = S->board().isDigitComplete((uint8_t)d);
        lv_obj_set_style_opa(g_numBtn[d], done ? LV_OPA_40 : LV_OPA_COVER, 0);
        if (done) lv_obj_clear_flag(g_numBtn[d], LV_OBJ_FLAG_CLICKABLE);
        else      lv_obj_add_flag(g_numBtn[d], LV_OBJ_FLAG_CLICKABLE);
    }
}

// ===================== callbacks =====================
static void tick_cb(lv_timer_t *) {
    if (!g_timerLabel || !S) return;
    char b[8];
    fmtTime(S->elapsedMs(), b, sizeof(b));
    lv_label_set_text(g_timerLabel, b);
}

static void flash_clear_cb(lv_timer_t *t) {
    refreshGame();
    lv_timer_del(t);
    g_flash = nullptr;
}

static void cell_cb(lv_event_t *e) {
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    S->selectCell(idx);
    refreshGame();
}

static void notes_cb(lv_event_t *) {
    g_notesMode = !g_notesMode;
    if (g_notesBtn)
        lv_obj_set_style_bg_color(g_notesBtn,
            g_notesMode ? theme::accent() : theme::accent2(), 0);
}

static void num_cb(lv_event_t *e) {
    int d = (int)(intptr_t)lv_event_get_user_data(e);   // 0 = cancella

    if (g_notesMode) {
        int sel = S->selectedCell();
        if (sel >= 0) {
            if (d == 0) {
                // backspace in modalita' appunti: svuota gli appunti della cella
                uint16_t m = S->board().notes(sel);
                for (int n = 1; n <= 9; n++)
                    if (m & (1u << (n - 1))) S->toggleNote((uint8_t)n);
            } else {
                S->toggleNote((uint8_t)d);
            }
        }
        refreshGame();
        return;
    }

    S->enterValue((uint8_t)d);
    if (S->state() == sudoku::GameState::Won) { showWin(); return; }
    refreshGame();
    // griglia piena ma non risolta -> lampeggio rosso sulle celle in conflitto
    if (S->board().isComplete()) {
        bool conf[81];
        S->board().conflicts(conf);
        for (int i = 0; i < 81; i++)
            if (conf[i]) lv_obj_set_style_bg_color(g_cell[i], theme::danger(), 0);
        if (g_flash) lv_timer_del(g_flash);
        g_flash = lv_timer_create(flash_clear_cb, 1400, nullptr);
    }
}

static void pause_cb(lv_event_t *) {
    S->pause();
    storage::saveGame(S->snapshot());   // salva per il "riprendi"
    showPause();
}

static void new_cb(lv_event_t *) { showMenu(); }

static void resume_cb(lv_event_t *) {
    S->resume();
    showGame();
}

static void diff_cb(lv_event_t *e) {
    int d = (int)(intptr_t)lv_event_get_user_data(e);
    storage::clearSavedGame();          // la partita salvata diventa obsoleta
    S->newGame((sudoku::Difficulty)d);
    showGame();
}

static void resumeSaved_cb(lv_event_t *) {
    sudoku::GameSession::Snapshot snap;
    if (storage::loadGame(snap) && S->restore(snap)) {
        S->resume();
        showGame();
    }
}

static void toMenu_cb(lv_event_t *) { showMenu(); }

// ===================== helper bottoni =====================
static lv_obj_t *makeButton(lv_obj_t *parent, const char *txt, const lv_font_t *font,
                            lv_color_t bg, lv_event_cb_t cb, int udata) {
    lv_obj_t *b = lv_btn_create(parent);
    plainStyle(b, bg, 10);
    lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, (void *)(intptr_t)udata);
    lv_obj_t *l = lv_label_create(b);
    lv_label_set_text(l, txt);
    lv_obj_set_style_text_color(l, theme::ink(), 0);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_center(l);
    return b;
}

// ===================== MENU =====================
static void showMenu() {
    killTimers();
    lv_obj_t *scr = lv_obj_create(nullptr);
    plainStyle(scr, theme::bg(), 0);

    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "SUDOKU");
    lv_obj_set_style_text_color(title, theme::ink(), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_48, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 40);

    static const sudoku::Difficulty diffs[3] = {
        sudoku::Difficulty::Easy, sudoku::Difficulty::Medium, sudoku::Difficulty::Hard };
    int y = 130;
    for (int i = 0; i < 3; i++) {
        lv_obj_t *b = makeButton(scr, difName(diffs[i]), &lv_font_montserrat_28,
                                 theme::accent2(), diff_cb, (int)diffs[i]);
        lv_obj_set_size(b, 300, 56);
        lv_obj_align(b, LV_ALIGN_TOP_MID, 0, y);
        y += 66;
    }

    // Riprendi (solo se c'e' una partita salvata valida)
    if (storage::hasSavedGame()) {
        lv_obj_t *b = makeButton(scr, "Riprendi partita", &lv_font_montserrat_24,
                                 theme::accent(), resumeSaved_cb, 0);
        lv_obj_set_size(b, 300, 50);
        lv_obj_align(b, LV_ALIGN_TOP_MID, 0, y + 6);
    }

    // Record per livello
    char rec[96];
    char e[8], m[8], h[8];
    uint32_t re = storage::bestTime(sudoku::Difficulty::Easy);
    uint32_t rm = storage::bestTime(sudoku::Difficulty::Medium);
    uint32_t rh = storage::bestTime(sudoku::Difficulty::Hard);
    fmtTime(re * 1000, e, sizeof(e));
    fmtTime(rm * 1000, m, sizeof(m));
    fmtTime(rh * 1000, h, sizeof(h));
    snprintf(rec, sizeof(rec), "Record  F %s   M %s   D %s",
             re ? e : "--:--", rm ? m : "--:--", rh ? h : "--:--");
    lv_obj_t *recL = lv_label_create(scr);
    lv_label_set_text(recL, rec);
    lv_obj_set_style_text_color(recL, theme::muted(), 0);
    lv_obj_set_style_text_font(recL, &lv_font_montserrat_16, 0);
    lv_obj_align(recL, LV_ALIGN_BOTTOM_MID, 0, -16);

    lv_scr_load_anim(scr, LV_SCR_LOAD_ANIM_FADE_IN, 150, 0, true);
}

// ===================== GIOCO =====================
static void showGame() {
    killTimers();
    lv_obj_t *scr = lv_obj_create(nullptr);
    plainStyle(scr, theme::bg(), 0);

    // --- top bar ---
    lv_obj_t *bar = lv_obj_create(scr);
    plainStyle(bar, theme::panel(), 0);
    lv_obj_set_size(bar, 480, 50);
    lv_obj_align(bar, LV_ALIGN_TOP_MID, 0, 0);

    g_timerLabel = lv_label_create(bar);
    lv_label_set_text(g_timerLabel, "00:00");
    lv_obj_set_style_text_color(g_timerLabel, theme::ink(), 0);
    lv_obj_set_style_text_font(g_timerLabel, &lv_font_montserrat_28, 0);
    lv_obj_align(g_timerLabel, LV_ALIGN_LEFT_MID, 14, 0);

    lv_obj_t *bNew = makeButton(bar, LV_SYMBOL_PLUS, &lv_font_montserrat_24,
                                theme::accent2(), new_cb, 0);
    lv_obj_set_size(bNew, 46, 38);
    lv_obj_align(bNew, LV_ALIGN_RIGHT_MID, -8, 0);

    lv_obj_t *bPause = makeButton(bar, LV_SYMBOL_PAUSE, &lv_font_montserrat_24,
                                  theme::accent2(), pause_cb, 0);
    lv_obj_set_size(bPause, 46, 38);
    lv_obj_align(bPause, LV_ALIGN_RIGHT_MID, -62, 0);

    // toggle appunti (matita): evidenziato quando attivo
    g_notesMode = false;
    g_notesBtn = makeButton(bar, LV_SYMBOL_EDIT, &lv_font_montserrat_24,
                            theme::accent2(), notes_cb, 0);
    lv_obj_set_size(g_notesBtn, 46, 38);
    lv_obj_align(g_notesBtn, LV_ALIGN_RIGHT_MID, -116, 0);

    // --- griglia ---
    int gw = cellX(8) + CELL;   // larghezza/altezza totale griglia
    lv_obj_t *grid = lv_obj_create(scr);
    plainStyle(grid, theme::thick(), 2);
    lv_obj_set_size(grid, gw, gw);
    lv_obj_align(grid, LV_ALIGN_TOP_MID, 0, 56);

    for (int i = 0; i < 81; i++) {
        int r = i / 9, c = i % 9;
        lv_obj_t *cell = lv_obj_create(grid);
        plainStyle(cell, theme::cell(), 2);
        lv_obj_set_size(cell, CELL - 2, CELL - 2);
        lv_obj_set_pos(cell, cellX(c) + 1, cellY(r) + 1);
        lv_obj_add_flag(cell, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(cell, cell_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        g_cell[i] = cell;

        lv_obj_t *lab = lv_label_create(cell);
        lv_obj_set_style_text_font(lab, &lv_font_montserrat_28, 0);
        lv_obj_center(lab);
        g_cellLabel[i] = lab;
    }

    // --- tastierino ---
    int n = 10;            // 1..9 + cancella
    int bw = 44, gap = 4;
    int totalW = n * bw + (n - 1) * gap;
    int startX = (480 - totalW) / 2;
    int padY = 56 + gw + 10;            // sotto la griglia
    for (int i = 0; i < 10; i++) g_numBtn[i] = nullptr;
    for (int k = 0; k < n; k++) {
        const char *txt;
        char digit[2] = {0, 0};
        int udata;
        if (k < 9) { digit[0] = (char)('1' + k); txt = digit; udata = k + 1; }
        else       { txt = LV_SYMBOL_BACKSPACE;  udata = 0; }
        lv_obj_t *b = makeButton(scr, txt, &lv_font_montserrat_28,
                                 theme::accent2(), num_cb, udata);
        lv_obj_set_size(b, bw, 58);
        lv_obj_set_pos(b, startX + k * (bw + gap), padY);
        if (k < 9) g_numBtn[k + 1] = b;
    }

    refreshGame();
    g_tick = lv_timer_create(tick_cb, 250, nullptr);

    lv_scr_load_anim(scr, LV_SCR_LOAD_ANIM_FADE_IN, 120, 0, true);
}

// ===================== PAUSA =====================
static void showPause() {
    killTimers();
    lv_obj_t *scr = lv_obj_create(nullptr);
    plainStyle(scr, theme::bg(), 0);
    lv_obj_add_flag(scr, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(scr, resume_cb, LV_EVENT_CLICKED, nullptr);

    lv_obj_t *t = lv_label_create(scr);
    lv_label_set_text(t, "In pausa");
    lv_obj_set_style_text_color(t, theme::ink(), 0);
    lv_obj_set_style_text_font(t, &lv_font_montserrat_48, 0);
    lv_obj_align(t, LV_ALIGN_CENTER, 0, -20);

    lv_obj_t *s = lv_label_create(scr);
    lv_label_set_text(s, "tocca per riprendere");
    lv_obj_set_style_text_color(s, theme::muted(), 0);
    lv_obj_set_style_text_font(s, &lv_font_montserrat_18, 0);
    lv_obj_align(s, LV_ALIGN_CENTER, 0, 40);

    lv_scr_load_anim(scr, LV_SCR_LOAD_ANIM_FADE_IN, 120, 0, true);
}

// ===================== VITTORIA =====================
static void showWin() {
    killTimers();
    uint32_t secs = S->elapsedMs() / 1000;
    sudoku::Difficulty d = S->difficulty();
    uint32_t best = storage::bestTime(d);
    bool record = (best == 0 || secs < best);
    if (record) storage::setBestTime(d, secs);
    storage::clearSavedGame();          // partita finita

    lv_obj_t *scr = lv_obj_create(nullptr);
    plainStyle(scr, theme::bg(), 0);

    lv_obj_t *t = lv_label_create(scr);
    lv_label_set_text(t, "Hai vinto!");
    lv_obj_set_style_text_color(t, theme::good(), 0);
    lv_obj_set_style_text_font(t, &lv_font_montserrat_48, 0);
    lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 80);

    char tbuf[8];
    fmtTime(secs * 1000, tbuf, sizeof(tbuf));
    lv_obj_t *time = lv_label_create(scr);
    lv_label_set_text_fmt(time, "%s  -  %s", difName(d), tbuf);
    lv_obj_set_style_text_color(time, theme::ink(), 0);
    lv_obj_set_style_text_font(time, &lv_font_montserrat_28, 0);
    lv_obj_align(time, LV_ALIGN_CENTER, 0, -10);

    if (record) {
        lv_obj_t *rec = lv_label_create(scr);
        lv_label_set_text(rec, "Nuovo record!");
        lv_obj_set_style_text_color(rec, theme::good(), 0);
        lv_obj_set_style_text_font(rec, &lv_font_montserrat_24, 0);
        lv_obj_align(rec, LV_ALIGN_CENTER, 0, 40);
    }

    lv_obj_t *b = makeButton(scr, "Menu", &lv_font_montserrat_28,
                             theme::accent(), toMenu_cb, 0);
    lv_obj_set_size(b, 220, 56);
    lv_obj_align(b, LV_ALIGN_BOTTOM_MID, 0, -40);

    lv_scr_load_anim(scr, LV_SCR_LOAD_ANIM_FADE_IN, 150, 0, true);
}

// ===================== init =====================
void init(sudoku::GameSession *session) {
    S = session;
    showMenu();
}

} // namespace ui
