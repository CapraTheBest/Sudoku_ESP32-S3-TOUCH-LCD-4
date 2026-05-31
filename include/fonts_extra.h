#pragma once
#include <lvgl.h>

// Font generati con lv_font_conv (vedi scripts/fonts/ e scripts/gen_fonts.*).
// I .c sono compilati come C: dichiarazioni in extern "C" per il linking da C++.
#ifdef __cplusplus
extern "C" {
#endif

extern const lv_font_t font_eraser;   // glifo mdi-eraser (U+F01FE), 24px
extern const lv_font_t font_jp56;     // kanji 数独 (U+6570 U+72EC), 56px

#ifdef __cplusplus
}
#endif

// UTF-8 del glifo mdi-eraser (U+F01FE) e dei kanji 数独
#define SYM_ERASER "\xF3\xB0\x87\xBE"
#define SYM_KANJI_SUDOKU "\xE6\x95\xB0\xE7\x8B\xAC"
