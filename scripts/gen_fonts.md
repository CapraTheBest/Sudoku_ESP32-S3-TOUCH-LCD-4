# Rigenerare i font LVGL (`src/font_eraser.c`, `src/font_jp56.c`)

I file C dei font sono committati (servono alla build). I TTF sorgente NO
(`scripts/fonts/*.ttf` è in `.gitignore`): si riscaricano al bisogno.

Servono **Node.js** e **lv_font_conv** (`npx lv_font_conv`).

## 1. Scarica i TTF sorgente

```bash
mkdir -p scripts/fonts && cd scripts/fonts
# Material Design Icons (per il glifo mdi-eraser U+F01FE)
curl -sSL -o mdi.ttf "https://github.com/Templarian/MaterialDesign-Webfont/raw/master/fonts/materialdesignicons-webfont.ttf"
# Font giapponese con i kanji 数独 (Sawarabi Gothic)
curl -sSL -o jp.ttf "https://cdn.jsdelivr.net/gh/google/fonts@main/ofl/sawarabigothic/SawarabiGothic-Regular.ttf"
```

## 2. Genera i C font

```bash
# Icona gomma: glifo mdi-eraser (U+F01FE) a 24px
npx --yes lv_font_conv --font mdi.ttf -r 0xF01FE --size 24 --bpp 4 \
    --format lvgl --no-compress --lv-include lvgl.h -o ../../src/font_eraser.c

# Kanji 数独 (U+6570 U+72EC) a 56px per la splash
npx --yes lv_font_conv --font jp.ttf -r 0x6570 -r 0x72EC --size 56 --bpp 4 \
    --format lvgl --no-compress --lv-include lvgl.h -o ../../src/font_jp56.c
```

Le variabili risultanti sono `font_eraser` e `font_jp56` (dichiarate in
`include/fonts_extra.h`). Il flag di build `-DLV_LVGL_H_INCLUDE_SIMPLE`
fa sì che i file includano `"lvgl.h"`.
