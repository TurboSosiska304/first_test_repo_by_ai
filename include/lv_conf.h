/**
 * Минимальный lv_conf.h под наш проект (ST7789 240x280, esp32-s3).
 * Не полный шаблон LVGL - только ключевые defines. Остальное LVGL
 * заполнит своими дефолтами благодаря LV_CONF_SUPPRESS_DEFINE_CHECK.
 */
#ifndef LV_CONF_H
#define LV_CONF_H

/*-------------------
 * Color
 *-----------------*/
#define LV_COLOR_DEPTH 16

/*-------------------
 * Memory
 *-----------------*/
#define LV_MEM_SIZE (48 * 1024U)   // достаточно для нескольких простых виджетов
#define LV_MEM_CUSTOM 0

/*-------------------
 * HAL
 *-----------------*/
#define LV_DEF_REFR_PERIOD 16      // ~60 FPS целевой период обновления (мс)
#define LV_DPI_DEF 130

/*-------------------
 * Feature configuration
 *-----------------*/
#define LV_USE_PERF_MONITOR 0
#define LV_USE_MEM_MONITOR 0

#define LV_USE_LOG 1
#define LV_LOG_LEVEL LV_LOG_LEVEL_WARN
#define LV_LOG_PRINTF 1

/*-------------------
 * Widgets - включаем то, что используем в демо-интерфейсе
 *-----------------*/
#define LV_USE_ARC        1
#define LV_USE_BAR        1
#define LV_USE_BTN        1
#define LV_USE_BUTTON     1
#define LV_USE_LABEL      1
#define LV_USE_SLIDER     1
#define LV_USE_SWITCH     1
#define LV_USE_LINE       1
#define LV_USE_TABVIEW    1
#define LV_USE_IMG        1
#define LV_USE_ANIMIMG    0

/*-------------------
 * Themes
 *-----------------*/
#define LV_USE_THEME_DEFAULT 1
#define LV_THEME_DEFAULT_DARK 1
#define LV_THEME_DEFAULT_GROW 1
#define LV_THEME_DEFAULT_TRANSITION_TIME 80

/*-------------------
 * Fonts
 *-----------------*/
#define LV_FONT_MONTSERRAT_10 1  /* Обязательно должна быть 1 */
#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_24 1
#define LV_FONT_MONTSERRAT_32 1
#define LV_FONT_DEFAULT &lv_font_montserrat_14

/*-------------------
 * Misc
 *-----------------*/
#define LV_USE_SYSMON 0

#endif /*LV_CONF_H*/