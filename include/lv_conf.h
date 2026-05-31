/**
 * lv_conf.h - configurazione LVGL 8.4 per HA_Panel.
 *
 * Tenuto nella cartella sketch: Arduino IDE lo include automaticamente.
 * Per disabilitare la copia di sistema (quella eventualmente nella libreria
 * lvgl installata), rinominala o rimuovila; in alternativa lascia che
 * questa "vinca" grazie all'include path locale dello sketch.
 */

/* Se serve disabilitare temporaneamente questo file, mettere 0 sotto */
#if 1
#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/* ============================================================
 *                COLOR & MEMORY
 * ============================================================ */
#define LV_COLOR_DEPTH              16
#define LV_COLOR_16_SWAP            0

/* Usa heap LVGL su PSRAM (8 MB disponibili) */
#define LV_MEM_CUSTOM               0
#define LV_MEM_SIZE                 (128U * 1024U)        /* 128 KB heap LVGL */
#define LV_MEM_ADR                  0
#define LV_MEM_BUF_MAX_NUM          16

#define LV_MEMCPY_MEMSET_STD        0

/* ============================================================
 *                HAL
 * ============================================================ */
#define LV_TICK_CUSTOM              1
#define LV_TICK_CUSTOM_INCLUDE      "Arduino.h"
#define LV_TICK_CUSTOM_SYS_TIME_EXPR (millis())

#define LV_DPI_DEF                  130

/* ============================================================
 *                FEATURE CONFIG
 * ============================================================ */
#define LV_USE_PERF_MONITOR         0
#define LV_USE_MEM_MONITOR          0
#define LV_USE_REFR_DEBUG           0
#define LV_USE_LOG                  0
#define LV_USE_ASSERT_NULL          1
#define LV_USE_ASSERT_MALLOC        1
#define LV_USE_ASSERT_STYLE         0
#define LV_USE_ASSERT_MEM_INTEGRITY 0
#define LV_USE_ASSERT_OBJ           0

#define LV_USE_USER_DATA            1
#define LV_ENABLE_GC                0

/* Anti-alias + opacita' (i bottoni con angoli arrotondati richiedono AA) */
#define LV_DRAW_COMPLEX             1
#define LV_SHADOW_CACHE_SIZE        0
#define LV_CIRCLE_CACHE_SIZE        4
#define LV_LAYER_SIMPLE_BUF_SIZE    (24 * 1024)
#define LV_IMG_CACHE_DEF_SIZE       0
#define LV_GRADIENT_MAX_STOPS       2
#define LV_DISP_DEF_REFR_PERIOD     16                /* ms (~60 fps) */
#define LV_INDEV_DEF_READ_PERIOD    10                /* ms: touch campionato piu' spesso = piu' reattivo */
#define LV_INDEV_DEF_LONG_PRESS_TIME 400
#define LV_INDEV_DEF_LONG_PRESS_REP_TIME 100
#define LV_INDEV_DEF_GESTURE_LIMIT  50
#define LV_INDEV_DEF_GESTURE_MIN_VELOCITY 3

/* ============================================================
 *                FONTS
 * ============================================================ */
#define LV_FONT_MONTSERRAT_12       1
#define LV_FONT_MONTSERRAT_14       1
#define LV_FONT_MONTSERRAT_16       1
#define LV_FONT_MONTSERRAT_18       1
#define LV_FONT_MONTSERRAT_20       0
#define LV_FONT_MONTSERRAT_22       0
#define LV_FONT_MONTSERRAT_24       1
#define LV_FONT_MONTSERRAT_26       0
#define LV_FONT_MONTSERRAT_28       1
#define LV_FONT_MONTSERRAT_30       0
#define LV_FONT_MONTSERRAT_32       0
#define LV_FONT_MONTSERRAT_40       0
#define LV_FONT_MONTSERRAT_48       1

/* Font monospaziato per gli appunti/note del Sudoku (colonne 3x3 allineate) */
#define LV_FONT_UNSCII_8            1

#define LV_FONT_DEFAULT             &lv_font_montserrat_14
#define LV_FONT_FMT_TXT_LARGE       0
#define LV_USE_FONT_COMPRESSED      0
#define LV_USE_FONT_SUBPX           0

/* ============================================================
 *                TEXT / BIDI
 * ============================================================ */
#define LV_TXT_ENC                  LV_TXT_ENC_UTF8
#define LV_TXT_BREAK_CHARS          " ,.;:-_"
#define LV_TXT_LINE_BREAK_LONG_LEN  0
#define LV_USE_BIDI                 0
#define LV_USE_ARABIC_PERSIAN_CHARS 0

/* ============================================================
 *                WIDGETS  (LVGL 8 li abilita tutti di default)
 * ============================================================ */
#define LV_USE_ARC          1
#define LV_USE_BAR          1
#define LV_USE_BTN          1
#define LV_USE_BTNMATRIX    1
#define LV_USE_CANVAS       1
#define LV_USE_CHECKBOX     1
#define LV_USE_DROPDOWN     1
#define LV_USE_IMG          1
#define LV_USE_LABEL        1
#define LV_LABEL_TEXT_SELECTION 1
#define LV_LABEL_LONG_TXT_HINT  1
#define LV_USE_LINE         1
#define LV_USE_ROLLER       1
#define LV_USE_SLIDER       1
#define LV_USE_SWITCH       1
#define LV_USE_TEXTAREA     1
#define LV_TEXTAREA_DEF_PWD_SHOW_TIME 1500
#define LV_USE_TABLE        1
#define LV_USE_OBJ          1

/* extra components che useremo per la tastiera QWERTY e la configurazione */
#define LV_USE_KEYBOARD     1
#define LV_USE_LIST         1
#define LV_USE_MSGBOX       1
#define LV_USE_SPINBOX      1
#define LV_USE_SPINNER      1
#define LV_USE_TABVIEW      1
#define LV_USE_TILEVIEW     1
#define LV_USE_WIN          1
#define LV_USE_CHART        0
#define LV_USE_METER        0
#define LV_USE_ANIMIMG      0
#define LV_USE_CALENDAR     0
#define LV_USE_COLORWHEEL   0
#define LV_USE_IMGBTN       0
#define LV_USE_LED          0
#define LV_USE_MENU         0
#define LV_USE_SCALE        0
#define LV_USE_SPAN         0

/* ============================================================
 *                THEMES
 * ============================================================ */
#define LV_USE_THEME_DEFAULT        1
#define LV_THEME_DEFAULT_DARK       1
#define LV_THEME_DEFAULT_GROW       1
#define LV_THEME_DEFAULT_TRANSITION_TIME 0
#define LV_USE_THEME_BASIC          1
#define LV_USE_THEME_MONO           0

/* ============================================================
 *                LAYOUTS
 * ============================================================ */
#define LV_USE_FLEX                 1
#define LV_USE_GRID                 1

/* ============================================================
 *                FILE SYSTEM / IMG DECODER (disabilitati)
 * ============================================================ */
#define LV_USE_FS_STDIO             0
#define LV_USE_FS_POSIX             0
#define LV_USE_FS_WIN32             0
#define LV_USE_FS_FATFS             0
#define LV_USE_PNG                  0
#define LV_USE_BMP                  0
#define LV_USE_SJPG                 0
#define LV_USE_GIF                  0
#define LV_USE_QRCODE               1
#define LV_USE_FREETYPE             0
#define LV_USE_RLOTTIE              0
#define LV_USE_FFMPEG               0

/* ============================================================
 *                OTHERS
 * ============================================================ */
#define LV_BUILD_EXAMPLES           0
#define LV_USE_DEMO_WIDGETS         0
#define LV_USE_DEMO_KEYPAD_AND_ENCODER 0
#define LV_USE_DEMO_BENCHMARK       0
#define LV_USE_DEMO_STRESS          0
#define LV_USE_DEMO_MUSIC           0

#endif /* LV_CONF_H */
#endif /* "Content enable" */
