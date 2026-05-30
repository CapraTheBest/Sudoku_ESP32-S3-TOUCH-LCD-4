/*
 * Sudoku Panel
 * ============
 * Gioco del Sudoku su Waveshare ESP32-S3-Touch-LCD-4 (480x480, ST7701, GT911).
 * Schemi casuali generati on-device, cronometro, pausa, riprendi-partita.
 *
 * PARTE 1 (fondazione): bring-up display + smoke test motore/sessione + splash.
 * Le schermate di gioco (menu/game/pause/win) sono il blocco successivo.
 */

#include <Arduino.h>
#include <ESP_Panel_Library.h>
#include <ESP_IOExpander_Library.h>
#include <esp_random.h>
#include <lvgl.h>

#include "lvgl_port_v8.h"
#include "config.h"
#include "storage.h"
#include "game_session.h"

// ---------------------------------------------------------------------------
// IO expander - sequenza Waveshare per accendere LCD/backlight/touch
// ---------------------------------------------------------------------------
#define EXP_CHIP_NAME       TCA95xx_8bit
#define EXP_I2C_NUM         (1)
#define EXP_I2C_SDA_PIN     (8)
#define EXP_I2C_SCL_PIN     (9)

#define _EXP_CHIP_CLASS(name, ...) ESP_IOExpander_##name(__VA_ARGS__)
#define EXP_CHIP_CLASS(name, ...)  _EXP_CHIP_CLASS(name, ##__VA_ARGS__)

static ESP_IOExpander *expander = nullptr;
static ESP_Panel *panel = nullptr;

// RNG device per la generazione degli schemi.
static uint32_t deviceRand(uint32_t maxExclusive) {
    return maxExclusive ? (esp_random() % maxExclusive) : 0;
}
// Clock monotono per il cronometro della sessione.
static uint32_t clockMs() { return millis(); }

static sudoku::GameSession session(deviceRand, clockMs);

void setup() {
    Serial.begin(SERIAL_BAUD);
    delay(1200);
    Serial.println("\n\n=== Sudoku Panel boot ===");
    Serial.printf("Firmware: %s\n", SUDOKU_FW_VERSION);

    // -- [1] IO expander + sequenza accensione Waveshare -------------------
    expander = new EXP_CHIP_CLASS(EXP_CHIP_NAME,
                                  (i2c_port_t) EXP_I2C_NUM,
                                  ESP_IO_EXPANDER_I2C_TCA9554_ADDRESS_000,
                                  EXP_I2C_SCL_PIN, EXP_I2C_SDA_PIN);
    expander->init();
    expander->begin();

    // Touch INT pin LOW (per indirizzo I2C del GT911 = 0x5D)
    pinMode(16, OUTPUT);
    digitalWrite(16, LOW);

    // Sequenza reset: EXIO5 ON, reset TP/LCD, poi EXIO5 OFF e out-of-reset
    expander->pinMode(5, OUTPUT); expander->digitalWrite(5, HIGH);
    expander->pinMode(0, OUTPUT); expander->digitalWrite(0, LOW);   // TP_RST low
    expander->pinMode(2, OUTPUT); expander->digitalWrite(2, LOW);   // LCD_RST low
    delay(200);
    expander->digitalWrite(5, LOW);
    expander->digitalWrite(2, HIGH);  // LCD out of reset
    expander->digitalWrite(0, HIGH);  // TP  out of reset

    // Backlight ON via EXIO1 (solo on/off, niente PWM su questa board)
    expander->pinMode(1, OUTPUT);
    expander->digitalWrite(1, HIGH);

    // -- [2] Panel (LCD + touch) -------------------------------------------
    panel = new ESP_Panel();
    panel->init();
#if LVGL_PORT_AVOID_TEAR
    ESP_PanelBus_RGB *rgb_bus = static_cast<ESP_PanelBus_RGB *>(panel->getLcd()->getBus());
    rgb_bus->configRgbFrameBufferNumber(LVGL_PORT_DISP_BUFFER_NUM);
    rgb_bus->configRgbBounceBufferSize(LVGL_PORT_RGB_BOUNCE_BUFFER_SIZE);
#endif
    panel->begin();

    // -- [3] LVGL -----------------------------------------------------------
    lvgl_port_init(panel->getLcd(), panel->getTouch());

    // -- [4] Storage + smoke test motore/sessione --------------------------
    storage::begin();
    session.newGame(sudoku::Difficulty::Medium);
    int givens = session.board().givenCount();
    Serial.printf("[smoke] newGame Medium -> indizi=%d stato=%d\n",
                  givens, (int) session.state());

    // -- [5] Splash ---------------------------------------------------------
    lvgl_port_lock(-1);
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0f1115), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "SUDOKU");
    lv_obj_set_style_text_color(title, lv_color_hex(0xe8ebf0), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_48, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, -30);

    lv_obj_t *sub = lv_label_create(scr);
    lv_label_set_text_fmt(sub, "v%s  -  indizi: %d", SUDOKU_FW_VERSION, givens);
    lv_obj_set_style_text_color(sub, lv_color_hex(0x5b8cff), 0);
    lv_obj_set_style_text_font(sub, &lv_font_montserrat_18, 0);
    lv_obj_align(sub, LV_ALIGN_CENTER, 0, 30);
    lvgl_port_unlock();

    Serial.println("=== setup completato ===");
}

void loop() {
    // Il task LVGL del port esegue lv_timer_handler; qui basta non affamarlo.
    delay(100);
}
