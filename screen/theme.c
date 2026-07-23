#include QMK_KEYBOARD_H
#include <eeprom.h>
#include "theme.h"
#include "ui_elements.h"

// Define custom offset if EECONFIG_SIZE is inaccessible
#ifndef EECONFIG_SIZE
#    define CUSTOM_THEME_EEPROM_ADDR ((uint8_t *)32) // Offsets past core QMK eeconfig block
#else
#    define CUSTOM_THEME_EEPROM_ADDR ((uint8_t *)(EECONFIG_SIZE))
#endif

ui_styles_t theme_style;
ui_styles_t theme_style2;
ui_styles_t theme_style3;
ui_styles_t theme_style4;

ui_styles_t *themes[] = {&theme_style, &theme_style2, &theme_style3, &theme_style4};
#define TOTAL_THEMES (sizeof(themes) / sizeof(themes[0]))

ui_styles_t            current_style;
dilemma_config_theme_t dilemma_config_theme;

void load_themes(void) {
    // -----------------------------------------------------
    // Theme 1 (Green Accent)
    // -----------------------------------------------------
    lv_style_init(&(theme_style.mod_btn));
    lv_style_set_text_font(&(theme_style.mod_btn), &lv_font_montserrat_14);
    lv_style_set_radius(&(theme_style.mod_btn), 5);
    lv_style_set_bg_opa(&(theme_style.mod_btn), LV_OPA_COVER);
    lv_style_set_bg_color(&(theme_style.mod_btn), lv_color_make(41, 46, 45));
    lv_style_set_bg_grad_color(&(theme_style.mod_btn), lv_color_make(37, 42, 41));
    lv_style_set_bg_grad_dir(&(theme_style.mod_btn), LV_GRAD_DIR_VER);
    lv_style_set_border_color(&(theme_style.mod_btn), lv_color_make(29, 34, 33));
    lv_style_set_border_width(&(theme_style.mod_btn), 1);
    lv_style_set_text_color(&(theme_style.mod_btn), lv_color_make(255, 255, 255));
    lv_style_set_text_opa(&(theme_style.mod_btn), 184);
    lv_style_set_pad_top(&(theme_style.mod_btn), 11);
    lv_style_set_pad_bottom(&(theme_style.mod_btn), 11);
    lv_style_set_pad_left(&(theme_style.mod_btn), 0);
    lv_style_set_pad_right(&(theme_style.mod_btn), 0);
    lv_style_set_shadow_color(&(theme_style.mod_btn), lv_color_make(25, 30, 29));
    lv_style_set_shadow_width(&(theme_style.mod_btn), 4);
    lv_style_set_shadow_spread(&(theme_style.mod_btn), 0);
    lv_style_set_shadow_ofs_y(&(theme_style.mod_btn), 4);
    lv_style_set_shadow_ofs_x(&(theme_style.mod_btn), 0);
    lv_style_set_shadow_opa(&(theme_style.mod_btn), LV_OPA_COVER);

    lv_style_init(&(theme_style.mod_btn_pressed));
    lv_style_set_text_font(&(theme_style.mod_btn_pressed), &lv_font_montserrat_14);
    lv_style_set_radius(&(theme_style.mod_btn_pressed), 5);
    lv_style_set_bg_opa(&(theme_style.mod_btn_pressed), LV_OPA_COVER);
    lv_style_set_bg_color(&(theme_style.mod_btn_pressed), lv_color_make(23, 211, 41));
    lv_style_set_bg_grad_color(&(theme_style.mod_btn_pressed), lv_color_make(0, 181, 11));
    lv_style_set_bg_grad_dir(&(theme_style.mod_btn_pressed), LV_GRAD_DIR_VER);
    lv_style_set_border_color(&(theme_style.mod_btn_pressed), lv_color_make(43, 231, 61));
    lv_style_set_border_width(&(theme_style.mod_btn_pressed), 1);
    lv_style_set_text_color(&(theme_style.mod_btn_pressed), lv_color_make(255, 255, 255));
    lv_style_set_text_opa(&(theme_style.mod_btn_pressed), LV_OPA_COVER);
    lv_style_set_pad_top(&(theme_style.mod_btn_pressed), 11);
    lv_style_set_pad_bottom(&(theme_style.mod_btn_pressed), 11);
    lv_style_set_pad_left(&(theme_style.mod_btn_pressed), 0);
    lv_style_set_pad_right(&(theme_style.mod_btn_pressed), 0);
    lv_style_set_translate_y(&(theme_style.mod_btn_pressed), 2);
    lv_style_set_shadow_color(&(theme_style.mod_btn_pressed), lv_color_make(23, 211, 41));
    lv_style_set_shadow_width(&(theme_style.mod_btn_pressed), 12);
    lv_style_set_shadow_spread(&(theme_style.mod_btn_pressed), 2);
    lv_style_set_shadow_ofs_y(&(theme_style.mod_btn_pressed), 0);
    lv_style_set_shadow_ofs_x(&(theme_style.mod_btn_pressed), 0);
    lv_style_set_shadow_opa(&(theme_style.mod_btn_pressed), 20);

    lv_style_init(&(theme_style.bar));
    lv_style_set_radius(&(theme_style.bar), 1);
    lv_style_set_bg_color(&(theme_style.bar), lv_color_make(23, 211, 41));
    lv_style_set_bg_grad_color(&(theme_style.bar), lv_color_make(0, 181, 11));
    lv_style_set_bg_grad_dir(&(theme_style.bar), LV_GRAD_DIR_VER);
    lv_style_set_bg_opa(&(theme_style.bar), LV_OPA_COVER);
    lv_style_set_border_color(&(theme_style.bar), lv_color_make(0, 0, 0));
    lv_style_set_border_width(&(theme_style.bar), 0);
    lv_style_set_height(&(theme_style.bar), 9);
    lv_style_set_shadow_color(&(theme_style.bar), lv_color_make(43, 231, 61));
    lv_style_set_shadow_width(&(theme_style.bar), 5);
    lv_style_set_shadow_opa(&(theme_style.bar), 31);

    lv_style_init(&(theme_style.bar_background));
    lv_style_set_radius(&(theme_style.bar_background), 1);
    lv_style_set_bg_color(&(theme_style.bar_background), lv_color_make(33, 38, 37));
    lv_style_set_bg_grad_color(&(theme_style.bar_background), lv_color_make(33, 38, 37));
    lv_style_set_bg_grad_dir(&(theme_style.bar_background), LV_GRAD_DIR_VER);
    lv_style_set_bg_opa(&(theme_style.bar_background), LV_OPA_COVER);
    lv_style_set_border_color(&(theme_style.bar_background), lv_color_make(30, 35, 34));
    lv_style_set_border_width(&(theme_style.bar_background), 0);
    lv_style_set_shadow_color(&(theme_style.bar_background), lv_color_make(6, 8, 16));
    lv_style_set_shadow_width(&(theme_style.bar_background), 0);
    lv_style_set_shadow_opa(&(theme_style.bar_background), LV_OPA_TRANSP);
    lv_style_set_height(&(theme_style.bar_background), 9);

    lv_style_init(&(theme_style.layer_name));
    lv_style_set_text_font(&(theme_style.layer_name), &lv_font_montserrat_14);
    lv_style_set_radius(&(theme_style.layer_name), 5);
    lv_style_set_bg_opa(&(theme_style.layer_name), LV_OPA_COVER);
    lv_style_set_bg_color(&(theme_style.layer_name), lv_color_make(41, 46, 45));
    lv_style_set_bg_grad_color(&(theme_style.layer_name), lv_color_make(37, 42, 41));
    lv_style_set_bg_grad_dir(&(theme_style.layer_name), LV_GRAD_DIR_VER);
    lv_style_set_border_color(&(theme_style.layer_name), lv_color_make(29, 34, 33));
    lv_style_set_border_width(&(theme_style.layer_name), 1);
    lv_style_set_text_color(&(theme_style.layer_name), lv_color_make(255, 255, 255));
    lv_style_set_text_opa(&(theme_style.layer_name), 184);
    lv_style_set_pad_top(&(theme_style.layer_name), 11);
    lv_style_set_pad_bottom(&(theme_style.layer_name), 11);
    lv_style_set_pad_left(&(theme_style.layer_name), 0);
    lv_style_set_pad_right(&(theme_style.layer_name), 0);
    lv_style_set_shadow_color(&(theme_style.layer_name), lv_color_make(25, 30, 29));
    lv_style_set_shadow_width(&(theme_style.layer_name), 4);
    lv_style_set_shadow_spread(&(theme_style.layer_name), 0);
    lv_style_set_shadow_ofs_y(&(theme_style.layer_name), 4);
    lv_style_set_shadow_ofs_x(&(theme_style.layer_name), 0);
    lv_style_set_shadow_opa(&(theme_style.layer_name), LV_OPA_COVER);

    lv_style_init(&(theme_style.secondary_labels));
    lv_style_set_text_font(&(theme_style.secondary_labels), &lv_font_montserrat_14);
    lv_style_set_text_color(&(theme_style.secondary_labels), lv_color_make(139, 233, 148));

    lv_style_init(&(theme_style.value_labels));
    lv_style_set_text_color(&(theme_style.value_labels), lv_color_make(245, 245, 245));

    lv_style_init(&(theme_style.flex_container));
    lv_style_set_bg_color(&(theme_style.flex_container), lv_color_make(41, 46, 45));
    lv_style_set_bg_opa(&(theme_style.flex_container), LV_OPA_COVER);
    lv_style_set_border_width(&(theme_style.flex_container), 0);

    // -----------------------------------------------------
    // Theme 2 (Purple Accent)
    // -----------------------------------------------------
    lv_style_init(&(theme_style2.mod_btn));
    lv_style_set_text_font(&(theme_style2.mod_btn), &lv_font_montserrat_14);
    lv_style_set_radius(&(theme_style2.mod_btn), 5);
    lv_style_set_bg_opa(&(theme_style2.mod_btn), LV_OPA_COVER);
    lv_style_set_bg_color(&(theme_style2.mod_btn), lv_color_make(45, 42, 45));
    lv_style_set_bg_grad_color(&(theme_style2.mod_btn), lv_color_make(41, 38, 41));
    lv_style_set_bg_grad_dir(&(theme_style2.mod_btn), LV_GRAD_DIR_VER);
    lv_style_set_border_color(&(theme_style2.mod_btn), lv_color_make(33, 30, 33));
    lv_style_set_border_width(&(theme_style2.mod_btn), 1);
    lv_style_set_text_color(&(theme_style2.mod_btn), lv_color_make(255, 255, 255));
    lv_style_set_text_opa(&(theme_style2.mod_btn), 184);
    lv_style_set_pad_top(&(theme_style2.mod_btn), 11);
    lv_style_set_pad_bottom(&(theme_style2.mod_btn), 11);
    lv_style_set_pad_left(&(theme_style2.mod_btn), 0);
    lv_style_set_pad_right(&(theme_style2.mod_btn), 0);
    lv_style_set_shadow_color(&(theme_style2.mod_btn), lv_color_make(29, 26, 29));
    lv_style_set_shadow_width(&(theme_style2.mod_btn), 4);
    lv_style_set_shadow_spread(&(theme_style2.mod_btn), 0);
    lv_style_set_shadow_ofs_y(&(theme_style2.mod_btn), 4);
    lv_style_set_shadow_ofs_x(&(theme_style2.mod_btn), 0);
    lv_style_set_shadow_opa(&(theme_style2.mod_btn), LV_OPA_COVER);

    lv_style_init(&(theme_style2.mod_btn_pressed));
    lv_style_set_text_font(&(theme_style2.mod_btn_pressed), &lv_font_montserrat_14);
    lv_style_set_radius(&(theme_style2.mod_btn_pressed), 5);
    lv_style_set_bg_opa(&(theme_style2.mod_btn_pressed), LV_OPA_COVER);
    lv_style_set_bg_color(&(theme_style2.mod_btn_pressed), lv_color_make(143, 49, 186));
    lv_style_set_bg_grad_color(&(theme_style2.mod_btn_pressed), lv_color_make(113, 19, 156));
    lv_style_set_bg_grad_dir(&(theme_style2.mod_btn_pressed), LV_GRAD_DIR_VER);
    lv_style_set_border_color(&(theme_style2.mod_btn_pressed), lv_color_make(163, 69, 206));
    lv_style_set_border_width(&(theme_style2.mod_btn_pressed), 1);
    lv_style_set_text_color(&(theme_style2.mod_btn_pressed), lv_color_make(255, 255, 255));
    lv_style_set_text_opa(&(theme_style2.mod_btn_pressed), LV_OPA_COVER);
    lv_style_set_pad_top(&(theme_style2.mod_btn_pressed), 11);
    lv_style_set_pad_bottom(&(theme_style2.mod_btn_pressed), 11);
    lv_style_set_pad_left(&(theme_style2.mod_btn_pressed), 0);
    lv_style_set_pad_right(&(theme_style2.mod_btn_pressed), 0);
    lv_style_set_translate_y(&(theme_style2.mod_btn_pressed), 2);
    lv_style_set_shadow_color(&(theme_style2.mod_btn_pressed), lv_color_make(143, 49, 186));
    lv_style_set_shadow_width(&(theme_style2.mod_btn_pressed), 12);
    lv_style_set_shadow_spread(&(theme_style2.mod_btn_pressed), 2);
    lv_style_set_shadow_ofs_y(&(theme_style2.mod_btn_pressed), 0);
    lv_style_set_shadow_ofs_x(&(theme_style2.mod_btn_pressed), 0);
    lv_style_set_shadow_opa(&(theme_style2.mod_btn_pressed), 20);

    lv_style_init(&(theme_style2.bar));
    lv_style_set_radius(&(theme_style2.bar), 1);
    lv_style_set_bg_color(&(theme_style2.bar), lv_color_make(143, 49, 186));
    lv_style_set_bg_grad_color(&(theme_style2.bar), lv_color_make(113, 19, 156));
    lv_style_set_bg_grad_dir(&(theme_style2.bar), LV_GRAD_DIR_VER);
    lv_style_set_bg_opa(&(theme_style2.bar), LV_OPA_COVER);
    lv_style_set_border_color(&(theme_style2.bar), lv_color_make(0, 0, 0));
    lv_style_set_border_width(&(theme_style2.bar), 0);
    lv_style_set_height(&(theme_style2.bar), 9);
    lv_style_set_shadow_color(&(theme_style2.bar), lv_color_make(163, 69, 206));
    lv_style_set_shadow_width(&(theme_style2.bar), 5);
    lv_style_set_shadow_opa(&(theme_style2.bar), 31);

    lv_style_init(&(theme_style2.bar_background));
    lv_style_set_radius(&(theme_style2.bar_background), 1);
    lv_style_set_bg_color(&(theme_style2.bar_background), lv_color_make(37, 34, 37));
    lv_style_set_bg_grad_color(&(theme_style2.bar_background), lv_color_make(37, 34, 37));
    lv_style_set_bg_grad_dir(&(theme_style2.bar_background), LV_GRAD_DIR_VER);
    lv_style_set_bg_opa(&(theme_style2.bar_background), LV_OPA_COVER);
    lv_style_set_border_color(&(theme_style2.bar_background), lv_color_make(34, 31, 34));
    lv_style_set_border_width(&(theme_style2.bar_background), 0);
    lv_style_set_shadow_color(&(theme_style2.bar_background), lv_color_make(6, 8, 16));
    lv_style_set_shadow_width(&(theme_style2.bar_background), 0);
    lv_style_set_shadow_opa(&(theme_style2.bar_background), LV_OPA_TRANSP);
    lv_style_set_height(&(theme_style2.bar_background), 9);

    lv_style_init(&(theme_style2.layer_name));
    lv_style_set_text_font(&(theme_style2.layer_name), &lv_font_montserrat_14);
    lv_style_set_radius(&(theme_style2.layer_name), 5);
    lv_style_set_bg_opa(&(theme_style2.layer_name), LV_OPA_COVER);
    lv_style_set_bg_color(&(theme_style2.layer_name), lv_color_make(45, 42, 45));
    lv_style_set_bg_grad_color(&(theme_style2.layer_name), lv_color_make(41, 38, 41));
    lv_style_set_bg_grad_dir(&(theme_style2.layer_name), LV_GRAD_DIR_VER);
    lv_style_set_border_color(&(theme_style2.layer_name), lv_color_make(33, 30, 33));
    lv_style_set_border_width(&(theme_style2.layer_name), 1);
    lv_style_set_text_color(&(theme_style2.layer_name), lv_color_make(255, 255, 255));
    lv_style_set_text_opa(&(theme_style2.layer_name), 184);
    lv_style_set_pad_top(&(theme_style2.layer_name), 11);
    lv_style_set_pad_bottom(&(theme_style2.layer_name), 11);
    lv_style_set_pad_left(&(theme_style2.layer_name), 0);
    lv_style_set_pad_right(&(theme_style2.layer_name), 0);
    lv_style_set_shadow_color(&(theme_style2.layer_name), lv_color_make(29, 26, 29));
    lv_style_set_shadow_width(&(theme_style2.layer_name), 4);
    lv_style_set_shadow_spread(&(theme_style2.layer_name), 0);
    lv_style_set_shadow_ofs_y(&(theme_style2.layer_name), 4);
    lv_style_set_shadow_ofs_x(&(theme_style2.layer_name), 0);
    lv_style_set_shadow_opa(&(theme_style2.layer_name), LV_OPA_COVER);

    lv_style_init(&(theme_style2.secondary_labels));
    lv_style_set_text_font(&(theme_style2.secondary_labels), &lv_font_montserrat_14);
    lv_style_set_text_color(&(theme_style2.secondary_labels), lv_color_make(199, 152, 221));

    lv_style_init(&(theme_style2.value_labels));
    lv_style_set_text_color(&(theme_style2.value_labels), lv_color_make(245, 245, 245));

    lv_style_init(&(theme_style2.flex_container));
    lv_style_set_bg_color(&(theme_style2.flex_container), lv_color_make(45, 42, 45));
    lv_style_set_bg_opa(&(theme_style2.flex_container), LV_OPA_COVER);
    lv_style_set_border_width(&(theme_style2.flex_container), 0);

    // -----------------------------------------------------
    // Theme 3 (Gruvbox Orange Accent)
    // -----------------------------------------------------
    lv_style_init(&(theme_style3.mod_btn));
    lv_style_set_text_font(&(theme_style3.mod_btn), &lv_font_montserrat_14);
    lv_style_set_radius(&(theme_style3.mod_btn), 5);
    lv_style_set_bg_opa(&(theme_style3.mod_btn), LV_OPA_COVER);
    lv_style_set_bg_color(&(theme_style3.mod_btn), lv_color_make(40, 40, 40));
    lv_style_set_bg_grad_color(&(theme_style3.mod_btn), lv_color_make(36, 36, 36));
    lv_style_set_bg_grad_dir(&(theme_style3.mod_btn), LV_GRAD_DIR_VER);
    lv_style_set_border_color(&(theme_style3.mod_btn), lv_color_make(28, 28, 28));
    lv_style_set_border_width(&(theme_style3.mod_btn), 1);
    lv_style_set_text_color(&(theme_style3.mod_btn), lv_color_make(235, 219, 178));
    lv_style_set_text_opa(&(theme_style3.mod_btn), 184);
    lv_style_set_pad_top(&(theme_style3.mod_btn), 11);
    lv_style_set_pad_bottom(&(theme_style3.mod_btn), 11);
    lv_style_set_pad_left(&(theme_style3.mod_btn), 0);
    lv_style_set_pad_right(&(theme_style3.mod_btn), 0);
    lv_style_set_shadow_color(&(theme_style3.mod_btn), lv_color_make(24, 24, 24));
    lv_style_set_shadow_width(&(theme_style3.mod_btn), 4);
    lv_style_set_shadow_spread(&(theme_style3.mod_btn), 0);
    lv_style_set_shadow_ofs_y(&(theme_style3.mod_btn), 4);
    lv_style_set_shadow_ofs_x(&(theme_style3.mod_btn), 0);
    lv_style_set_shadow_opa(&(theme_style3.mod_btn), LV_OPA_COVER);

    lv_style_init(&(theme_style3.mod_btn_pressed));
    lv_style_set_text_font(&(theme_style3.mod_btn_pressed), &lv_font_montserrat_14);
    lv_style_set_radius(&(theme_style3.mod_btn_pressed), 5);
    lv_style_set_bg_opa(&(theme_style3.mod_btn_pressed), LV_OPA_COVER);
    lv_style_set_bg_color(&(theme_style3.mod_btn_pressed), lv_color_make(214, 93, 14));
    lv_style_set_bg_grad_color(&(theme_style3.mod_btn_pressed), lv_color_make(184, 63, 0));
    lv_style_set_bg_grad_dir(&(theme_style3.mod_btn_pressed), LV_GRAD_DIR_VER);
    lv_style_set_border_color(&(theme_style3.mod_btn_pressed), lv_color_make(234, 113, 34));
    lv_style_set_border_width(&(theme_style3.mod_btn_pressed), 1);
    lv_style_set_text_color(&(theme_style3.mod_btn_pressed), lv_color_make(235, 219, 178));
    lv_style_set_text_opa(&(theme_style3.mod_btn_pressed), LV_OPA_COVER);
    lv_style_set_pad_top(&(theme_style3.mod_btn_pressed), 11);
    lv_style_set_pad_bottom(&(theme_style3.mod_btn_pressed), 11);
    lv_style_set_pad_left(&(theme_style3.mod_btn_pressed), 0);
    lv_style_set_pad_right(&(theme_style3.mod_btn_pressed), 0);
    lv_style_set_translate_y(&(theme_style3.mod_btn_pressed), 2);
    lv_style_set_shadow_color(&(theme_style3.mod_btn_pressed), lv_color_make(214, 93, 14));
    lv_style_set_shadow_width(&(theme_style3.mod_btn_pressed), 12);
    lv_style_set_shadow_spread(&(theme_style3.mod_btn_pressed), 2);
    lv_style_set_shadow_ofs_y(&(theme_style3.mod_btn_pressed), 0);
    lv_style_set_shadow_ofs_x(&(theme_style3.mod_btn_pressed), 0);
    lv_style_set_shadow_opa(&(theme_style3.mod_btn_pressed), 20);

    lv_style_init(&(theme_style3.bar));
    lv_style_set_radius(&(theme_style3.bar), 1);
    lv_style_set_bg_color(&(theme_style3.bar), lv_color_make(214, 93, 14));
    lv_style_set_bg_grad_color(&(theme_style3.bar), lv_color_make(184, 63, 0));
    lv_style_set_bg_grad_dir(&(theme_style3.bar), LV_GRAD_DIR_VER);
    lv_style_set_bg_opa(&(theme_style3.bar), LV_OPA_COVER);
    lv_style_set_border_color(&(theme_style3.bar), lv_color_make(0, 0, 0));
    lv_style_set_border_width(&(theme_style3.bar), 0);
    lv_style_set_height(&(theme_style3.bar), 9);
    lv_style_set_shadow_color(&(theme_style3.bar), lv_color_make(234, 113, 34));
    lv_style_set_shadow_width(&(theme_style3.bar), 5);
    lv_style_set_shadow_opa(&(theme_style3.bar), 31);

    lv_style_init(&(theme_style3.bar_background));
    lv_style_set_radius(&(theme_style3.bar_background), 1);
    lv_style_set_bg_color(&(theme_style3.bar_background), lv_color_make(32, 32, 32));
    lv_style_set_bg_grad_color(&(theme_style3.bar_background), lv_color_make(32, 32, 32));
    lv_style_set_bg_grad_dir(&(theme_style3.bar_background), LV_GRAD_DIR_VER);
    lv_style_set_bg_opa(&(theme_style3.bar_background), LV_OPA_COVER);
    lv_style_set_border_color(&(theme_style3.bar_background), lv_color_make(29, 29, 29));
    lv_style_set_border_width(&(theme_style3.bar_background), 0);
    lv_style_set_shadow_color(&(theme_style3.bar_background), lv_color_make(6, 8, 16));
    lv_style_set_shadow_width(&(theme_style3.bar_background), 0);
    lv_style_set_shadow_opa(&(theme_style3.bar_background), LV_OPA_TRANSP);
    lv_style_set_height(&(theme_style3.bar_background), 9);

    lv_style_init(&(theme_style3.layer_name));
    lv_style_set_text_font(&(theme_style3.layer_name), &lv_font_montserrat_14);
    lv_style_set_radius(&(theme_style3.layer_name), 5);
    lv_style_set_bg_opa(&(theme_style3.layer_name), LV_OPA_COVER);
    lv_style_set_bg_color(&(theme_style3.layer_name), lv_color_make(40, 40, 40));
    lv_style_set_bg_grad_color(&(theme_style3.layer_name), lv_color_make(36, 36, 36));
    lv_style_set_bg_grad_dir(&(theme_style3.layer_name), LV_GRAD_DIR_VER);
    lv_style_set_border_color(&(theme_style3.layer_name), lv_color_make(28, 28, 28));
    lv_style_set_border_width(&(theme_style3.layer_name), 1);
    lv_style_set_text_color(&(theme_style3.layer_name), lv_color_make(235, 219, 178));
    lv_style_set_text_opa(&(theme_style3.layer_name), 184);
    lv_style_set_pad_top(&(theme_style3.layer_name), 11);
    lv_style_set_pad_bottom(&(theme_style3.layer_name), 11);
    lv_style_set_pad_left(&(theme_style3.layer_name), 0);
    lv_style_set_pad_right(&(theme_style3.layer_name), 0);
    lv_style_set_shadow_color(&(theme_style3.layer_name), lv_color_make(24, 24, 24));
    lv_style_set_shadow_width(&(theme_style3.layer_name), 4);
    lv_style_set_shadow_spread(&(theme_style3.layer_name), 0);
    lv_style_set_shadow_ofs_y(&(theme_style3.layer_name), 4);
    lv_style_set_shadow_ofs_x(&(theme_style3.layer_name), 0);
    lv_style_set_shadow_opa(&(theme_style3.layer_name), LV_OPA_COVER);

    lv_style_init(&(theme_style3.secondary_labels));
    lv_style_set_text_font(&(theme_style3.secondary_labels), &lv_font_montserrat_14);
    lv_style_set_text_color(&(theme_style3.secondary_labels), lv_color_make(225, 156, 96));

    lv_style_init(&(theme_style3.value_labels));
    lv_style_set_text_color(&(theme_style3.value_labels), lv_color_make(225, 209, 168));

    lv_style_init(&(theme_style3.flex_container));
    lv_style_set_bg_color(&(theme_style3.flex_container), lv_color_make(40, 40, 40));
    lv_style_set_bg_opa(&(theme_style3.flex_container), LV_OPA_COVER);
    lv_style_set_border_width(&(theme_style3.flex_container), 0);

    // -----------------------------------------------------
    // Theme 4 (Nord Frost Accent)
    // -----------------------------------------------------
    lv_style_init(&(theme_style4.mod_btn));
    lv_style_set_text_font(&(theme_style4.mod_btn), &lv_font_montserrat_14);
    lv_style_set_radius(&(theme_style4.mod_btn), 5);
    lv_style_set_bg_opa(&(theme_style4.mod_btn), LV_OPA_COVER);
    lv_style_set_bg_color(&(theme_style4.mod_btn), lv_color_make(46, 52, 64));
    lv_style_set_bg_grad_color(&(theme_style4.mod_btn), lv_color_make(42, 48, 60));
    lv_style_set_bg_grad_dir(&(theme_style4.mod_btn), LV_GRAD_DIR_VER);
    lv_style_set_border_color(&(theme_style4.mod_btn), lv_color_make(34, 40, 52));
    lv_style_set_border_width(&(theme_style4.mod_btn), 1);
    lv_style_set_text_color(&(theme_style4.mod_btn), lv_color_make(236, 239, 244));
    lv_style_set_text_opa(&(theme_style4.mod_btn), 184);
    lv_style_set_pad_top(&(theme_style4.mod_btn), 11);
    lv_style_set_pad_bottom(&(theme_style4.mod_btn), 11);
    lv_style_set_pad_left(&(theme_style4.mod_btn), 0);
    lv_style_set_pad_right(&(theme_style4.mod_btn), 0);
    lv_style_set_shadow_color(&(theme_style4.mod_btn), lv_color_make(30, 36, 48));
    lv_style_set_shadow_width(&(theme_style4.mod_btn), 4);
    lv_style_set_shadow_spread(&(theme_style4.mod_btn), 0);
    lv_style_set_shadow_ofs_y(&(theme_style4.mod_btn), 4);
    lv_style_set_shadow_ofs_x(&(theme_style4.mod_btn), 0);
    lv_style_set_shadow_opa(&(theme_style4.mod_btn), LV_OPA_COVER);

    lv_style_init(&(theme_style4.mod_btn_pressed));
    lv_style_set_text_font(&(theme_style4.mod_btn_pressed), &lv_font_montserrat_14);
    lv_style_set_radius(&(theme_style4.mod_btn_pressed), 5);
    lv_style_set_bg_opa(&(theme_style4.mod_btn_pressed), LV_OPA_COVER);
    lv_style_set_bg_color(&(theme_style4.mod_btn_pressed), lv_color_make(136, 192, 208));
    lv_style_set_bg_grad_color(&(theme_style4.mod_btn_pressed), lv_color_make(106, 162, 178));
    lv_style_set_bg_grad_dir(&(theme_style4.mod_btn_pressed), LV_GRAD_DIR_VER);
    lv_style_set_border_color(&(theme_style4.mod_btn_pressed), lv_color_make(156, 212, 228));
    lv_style_set_border_width(&(theme_style4.mod_btn_pressed), 1);
    lv_style_set_text_color(&(theme_style4.mod_btn_pressed), lv_color_make(236, 239, 244));
    lv_style_set_text_opa(&(theme_style4.mod_btn_pressed), LV_OPA_COVER);
    lv_style_set_pad_top(&(theme_style4.mod_btn_pressed), 11);
    lv_style_set_pad_bottom(&(theme_style4.mod_btn_pressed), 11);
    lv_style_set_pad_left(&(theme_style4.mod_btn_pressed), 0);
    lv_style_set_pad_right(&(theme_style4.mod_btn_pressed), 0);
    lv_style_set_translate_y(&(theme_style4.mod_btn_pressed), 2);
    lv_style_set_shadow_color(&(theme_style4.mod_btn_pressed), lv_color_make(136, 192, 208));
    lv_style_set_shadow_width(&(theme_style4.mod_btn_pressed), 12);
    lv_style_set_shadow_spread(&(theme_style4.mod_btn_pressed), 2);
    lv_style_set_shadow_ofs_y(&(theme_style4.mod_btn_pressed), 0);
    lv_style_set_shadow_ofs_x(&(theme_style4.mod_btn_pressed), 0);
    lv_style_set_shadow_opa(&(theme_style4.mod_btn_pressed), 20);

    lv_style_init(&(theme_style4.bar));
    lv_style_set_radius(&(theme_style4.bar), 1);
    lv_style_set_bg_color(&(theme_style4.bar), lv_color_make(136, 192, 208));
    lv_style_set_bg_grad_color(&(theme_style4.bar), lv_color_make(106, 162, 178));
    lv_style_set_bg_grad_dir(&(theme_style4.bar), LV_GRAD_DIR_VER);
    lv_style_set_bg_opa(&(theme_style4.bar), LV_OPA_COVER);
    lv_style_set_border_color(&(theme_style4.bar), lv_color_make(0, 0, 0));
    lv_style_set_border_width(&(theme_style4.bar), 0);
    lv_style_set_height(&(theme_style4.bar), 9);
    lv_style_set_shadow_color(&(theme_style4.bar), lv_color_make(156, 212, 228));
    lv_style_set_shadow_width(&(theme_style4.bar), 5);
    lv_style_set_shadow_opa(&(theme_style4.bar), 31);

    lv_style_init(&(theme_style4.bar_background));
    lv_style_set_radius(&(theme_style4.bar_background), 1);
    lv_style_set_bg_color(&(theme_style4.bar_background), lv_color_make(38, 44, 56));
    lv_style_set_bg_grad_color(&(theme_style4.bar_background), lv_color_make(38, 44, 56));
    lv_style_set_bg_grad_dir(&(theme_style4.bar_background), LV_GRAD_DIR_VER);
    lv_style_set_bg_opa(&(theme_style4.bar_background), LV_OPA_COVER);
    lv_style_set_border_color(&(theme_style4.bar_background), lv_color_make(35, 41, 53));
    lv_style_set_border_width(&(theme_style4.bar_background), 0);
    lv_style_set_shadow_color(&(theme_style4.bar_background), lv_color_make(6, 8, 16));
    lv_style_set_shadow_width(&(theme_style4.bar_background), 0);
    lv_style_set_shadow_opa(&(theme_style4.bar_background), LV_OPA_TRANSP);
    lv_style_set_height(&(theme_style4.bar_background), 9);

    lv_style_init(&(theme_style4.layer_name));
    lv_style_set_text_font(&(theme_style4.layer_name), &lv_font_montserrat_14);
    lv_style_set_radius(&(theme_style4.layer_name), 5);
    lv_style_set_bg_opa(&(theme_style4.layer_name), LV_OPA_COVER);
    lv_style_set_bg_color(&(theme_style4.layer_name), lv_color_make(46, 52, 64));
    lv_style_set_bg_grad_color(&(theme_style4.layer_name), lv_color_make(42, 48, 60));
    lv_style_set_bg_grad_dir(&(theme_style4.layer_name), LV_GRAD_DIR_VER);
    lv_style_set_border_color(&(theme_style4.layer_name), lv_color_make(34, 40, 52));
    lv_style_set_border_width(&(theme_style4.layer_name), 1);
    lv_style_set_text_color(&(theme_style4.layer_name), lv_color_make(236, 239, 244));
    lv_style_set_text_opa(&(theme_style4.layer_name), 184);
    lv_style_set_pad_top(&(theme_style4.layer_name), 11);
    lv_style_set_pad_bottom(&(theme_style4.layer_name), 11);
    lv_style_set_pad_left(&(theme_style4.layer_name), 0);
    lv_style_set_pad_right(&(theme_style4.layer_name), 0);
    lv_style_set_shadow_color(&(theme_style4.layer_name), lv_color_make(30, 36, 48));
    lv_style_set_shadow_width(&(theme_style4.layer_name), 4);
    lv_style_set_shadow_spread(&(theme_style4.layer_name), 0);
    lv_style_set_shadow_ofs_y(&(theme_style4.layer_name), 4);
    lv_style_set_shadow_ofs_x(&(theme_style4.layer_name), 0);
    lv_style_set_shadow_opa(&(theme_style4.layer_name), LV_OPA_COVER);

    lv_style_init(&(theme_style4.secondary_labels));
    lv_style_set_text_font(&(theme_style4.secondary_labels), &lv_font_montserrat_14);
    lv_style_set_text_color(&(theme_style4.secondary_labels), lv_color_make(143, 188, 187));

    lv_style_init(&(theme_style4.value_labels));
    lv_style_set_text_color(&(theme_style4.value_labels), lv_color_make(236, 239, 244));

    lv_style_init(&(theme_style4.flex_container));
    lv_style_set_bg_color(&(theme_style4.flex_container), lv_color_make(46, 52, 64));
    lv_style_set_bg_opa(&(theme_style4.flex_container), LV_OPA_COVER);
    lv_style_set_border_width(&(theme_style4.flex_container), 0);
}

void init_styles(void) {
    load_themes();
    load_dilemma_theme_config_from_eeprom();
    update_styles_from_current_theme();
}

uint8_t get_current_theme_id(void) {
    return dilemma_config_theme.current_theme_id;
}

void set_current_theme_id(uint8_t id) {
    if (id >= TOTAL_THEMES) {
        id = 0;
    }
    dilemma_config_theme.current_theme_id = id;
}

ui_styles_t *get_current_ui_styles(void) {
    return &current_style;
}

void update_styles_from_current_theme(void) {
    uint8_t id = get_current_theme_id();
    if (id >= TOTAL_THEMES) {
        id = 0;
    }
    current_style = *themes[id];
}

void read_dilemma_theme_config_from_eeprom(dilemma_config_theme_t *config) {
    config->raw = eeprom_read_byte(CUSTOM_THEME_EEPROM_ADDR);
}

void write_dilemma_theme_config_to_eeprom(dilemma_config_theme_t *config) {
    eeprom_update_byte(CUSTOM_THEME_EEPROM_ADDR, config->raw);
}

void load_dilemma_theme_config_from_eeprom(void) {
    read_dilemma_theme_config_from_eeprom(&dilemma_config_theme);
    if (dilemma_config_theme.current_theme_id >= TOTAL_THEMES) {
        dilemma_config_theme.current_theme_id = 0;
        write_dilemma_theme_config_to_eeprom(&dilemma_config_theme);
    }
}
