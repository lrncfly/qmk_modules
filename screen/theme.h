#ifndef THEME_H
#define THEME_H

#include "lvgl.h"

typedef union {
    uint8_t raw;
    struct {
        uint8_t current_theme_id;
    } __attribute__((packed));
} dilemma_config_theme_t;

LV_FONT_DECLARE(lv_font_montserrat_24);

extern dilemma_config_theme_t dilemma_config_theme;

typedef struct {
    lv_style_t mod_btn;
    lv_style_t mod_btn_pressed;
    lv_style_t bar;
    lv_style_t bar_background;
    lv_style_t layer_name;
    lv_style_t line;
    lv_style_t line_background;
    lv_style_t secondary_labels;
    lv_style_t value_labels;
    lv_style_t flex_container;
    lv_style_t active_combo_label;
    lv_style_t history_item_container;
} ui_styles_t;

extern ui_styles_t current_style;

void read_dilemma_theme_config_from_eeprom(dilemma_config_theme_t *config);
void load_dilemma_theme_config_from_eeprom(void);
void write_dilemma_theme_config_to_eeprom(dilemma_config_theme_t *config);

void         init_styles(void);
void         update_styles_from_current_theme(void);
uint8_t      get_current_theme_id(void);
void set_current_theme_id(uint8_t id);
ui_styles_t *get_current_ui_styles(void);

#endif
