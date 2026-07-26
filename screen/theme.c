#include QMK_KEYBOARD_H
#include "theme.h"
#include "ui_elements.h"

// Match the extern declaration in theme.h
ui_styles_t current_style;

// Theme color definition (Default / Accent)
typedef struct {
    lv_color_t bg;
    lv_color_t bg_dark; // Pre-calculated darker shade for shadows
    lv_color_t bg_grad;
    lv_color_t border;
    lv_color_t text;
    lv_color_t secondary_text;
    lv_color_t accent;
    lv_color_t accent_grad;
    lv_color_t accent_border;
} theme_palette_t;

// Struct initialization works fine with LV_COLOR_MAKE even on 16-bit
static const theme_palette_t palette = {.bg = LV_COLOR_MAKE(41, 46, 45), .bg_dark = LV_COLOR_MAKE(31, 36, 35), .bg_grad = LV_COLOR_MAKE(37, 42, 41), .border = LV_COLOR_MAKE(29, 34, 33), .text = LV_COLOR_MAKE(255, 255, 255), .secondary_text = LV_COLOR_MAKE(139, 233, 148), .accent = LV_COLOR_MAKE(23, 211, 41), .accent_grad = LV_COLOR_MAKE(0, 181, 11), .accent_border = LV_COLOR_MAKE(43, 231, 61)};

void init_styles(void) {
    // Modifier Button Default
    lv_style_init(&(current_style.mod_btn));
    lv_style_set_text_font(&(current_style.mod_btn), &lv_font_montserrat_14);
    lv_style_set_radius(&(current_style.mod_btn), 5);
    lv_style_set_bg_opa(&(current_style.mod_btn), LV_OPA_COVER);
    lv_style_set_bg_color(&(current_style.mod_btn), palette.bg);
    lv_style_set_bg_grad_color(&(current_style.mod_btn), palette.bg_grad);
    lv_style_set_bg_grad_dir(&(current_style.mod_btn), LV_GRAD_DIR_VER);
    lv_style_set_border_color(&(current_style.mod_btn), palette.border);
    lv_style_set_border_width(&(current_style.mod_btn), 1);
    lv_style_set_text_color(&(current_style.mod_btn), palette.text);
    lv_style_set_text_opa(&(current_style.mod_btn), 184);
    lv_style_set_pad_top(&(current_style.mod_btn), 11);
    lv_style_set_pad_bottom(&(current_style.mod_btn), 11);
    lv_style_set_pad_left(&(current_style.mod_btn), 0);
    lv_style_set_pad_right(&(current_style.mod_btn), 0);
    lv_style_set_shadow_color(&(current_style.mod_btn), palette.bg_dark);
    lv_style_set_shadow_width(&(current_style.mod_btn), 4);
    lv_style_set_shadow_spread(&(current_style.mod_btn), 0);
    lv_style_set_shadow_ofs_y(&(current_style.mod_btn), 4);
    lv_style_set_shadow_ofs_x(&(current_style.mod_btn), 0);
    lv_style_set_shadow_opa(&(current_style.mod_btn), LV_OPA_COVER);

    // Modifier Button Pressed
    lv_style_init(&(current_style.mod_btn_pressed));
    lv_style_set_text_font(&(current_style.mod_btn_pressed), &lv_font_montserrat_14);
    lv_style_set_radius(&(current_style.mod_btn_pressed), 5);
    lv_style_set_bg_opa(&(current_style.mod_btn_pressed), LV_OPA_COVER);
    lv_style_set_bg_color(&(current_style.mod_btn_pressed), palette.accent);
    lv_style_set_bg_grad_color(&(current_style.mod_btn_pressed), palette.accent_grad);
    lv_style_set_bg_grad_dir(&(current_style.mod_btn_pressed), LV_GRAD_DIR_VER);
    lv_style_set_border_color(&(current_style.mod_btn_pressed), palette.accent_border);
    lv_style_set_border_width(&(current_style.mod_btn_pressed), 1);
    lv_style_set_text_color(&(current_style.mod_btn_pressed), palette.text);
    lv_style_set_text_opa(&(current_style.mod_btn_pressed), LV_OPA_COVER);
    lv_style_set_pad_top(&(current_style.mod_btn_pressed), 11);
    lv_style_set_pad_bottom(&(current_style.mod_btn_pressed), 11);
    lv_style_set_pad_left(&(current_style.mod_btn_pressed), 0);
    lv_style_set_pad_right(&(current_style.mod_btn_pressed), 0);
    lv_style_set_translate_y(&(current_style.mod_btn_pressed), 2);
    lv_style_set_shadow_color(&(current_style.mod_btn_pressed), palette.accent);
    lv_style_set_shadow_width(&(current_style.mod_btn_pressed), 12);
    lv_style_set_shadow_spread(&(current_style.mod_btn_pressed), 2);
    lv_style_set_shadow_ofs_y(&(current_style.mod_btn_pressed), 0);
    lv_style_set_shadow_ofs_x(&(current_style.mod_btn_pressed), 0);
    lv_style_set_shadow_opa(&(current_style.mod_btn_pressed), 20);

    // Bar Indicator
    lv_style_init(&(current_style.bar));
    lv_style_set_radius(&(current_style.bar), 1);
    lv_style_set_bg_color(&(current_style.bar), palette.accent);
    lv_style_set_bg_grad_color(&(current_style.bar), palette.accent_grad);
    lv_style_set_bg_grad_dir(&(current_style.bar), LV_GRAD_DIR_VER);
    lv_style_set_bg_opa(&(current_style.bar), LV_OPA_COVER);
    // Fixed: Replaced LV_COLOR_MAKE(0, 0, 0) with lv_color_black()
    lv_style_set_border_color(&(current_style.bar), lv_color_black());
    lv_style_set_border_width(&(current_style.bar), 0);
    lv_style_set_height(&(current_style.bar), 9);
    lv_style_set_shadow_color(&(current_style.bar), palette.accent_border);
    lv_style_set_shadow_width(&(current_style.bar), 5);
    lv_style_set_shadow_opa(&(current_style.bar), 31);

    // Bar Background
    lv_style_init(&(current_style.bar_background));
    lv_style_set_radius(&(current_style.bar_background), 1);
    lv_style_set_bg_color(&(current_style.bar_background), palette.bg_grad);
    lv_style_set_bg_grad_color(&(current_style.bar_background), palette.bg_grad);
    lv_style_set_bg_grad_dir(&(current_style.bar_background), LV_GRAD_DIR_VER);
    lv_style_set_bg_opa(&(current_style.bar_background), LV_OPA_COVER);
    lv_style_set_border_color(&(current_style.bar_background), palette.border);
    lv_style_set_border_width(&(current_style.bar_background), 0);
    // Fixed: Replaced LV_COLOR_MAKE(6, 8, 16) with lv_color_hex(0x060810)
    lv_style_set_shadow_color(&(current_style.bar_background), lv_color_hex(0x060810));
    lv_style_set_shadow_width(&(current_style.bar_background), 0);
    lv_style_set_shadow_opa(&(current_style.bar_background), LV_OPA_TRANSP);
    lv_style_set_height(&(current_style.bar_background), 9);

    // Layer Name
    lv_style_init(&(current_style.layer_name));
    lv_style_set_text_font(&(current_style.layer_name), &lv_font_montserrat_14);
    lv_style_set_radius(&(current_style.layer_name), 5);
    lv_style_set_bg_opa(&(current_style.layer_name), LV_OPA_COVER);
    lv_style_set_bg_color(&(current_style.layer_name), palette.bg);
    lv_style_set_bg_grad_color(&(current_style.layer_name), palette.bg_grad);
    lv_style_set_bg_grad_dir(&(current_style.layer_name), LV_GRAD_DIR_VER);
    lv_style_set_border_color(&(current_style.layer_name), palette.border);
    lv_style_set_border_width(&(current_style.layer_name), 1);
    lv_style_set_text_color(&(current_style.layer_name), palette.text);
    lv_style_set_text_opa(&(current_style.layer_name), 184);
    lv_style_set_pad_top(&(current_style.layer_name), 11);
    lv_style_set_pad_bottom(&(current_style.layer_name), 11);
    lv_style_set_pad_left(&(current_style.layer_name), 0);
    lv_style_set_pad_right(&(current_style.layer_name), 0);
    lv_style_set_shadow_color(&(current_style.layer_name), palette.bg_dark);
    lv_style_set_shadow_width(&(current_style.layer_name), 4);
    lv_style_set_shadow_spread(&(current_style.layer_name), 0);
    lv_style_set_shadow_ofs_y(&(current_style.layer_name), 4);
    lv_style_set_shadow_ofs_x(&(current_style.layer_name), 0);
    lv_style_set_shadow_opa(&(current_style.layer_name), LV_OPA_COVER);

    // Secondary Labels
    lv_style_init(&(current_style.secondary_labels));
    lv_style_set_text_font(&(current_style.secondary_labels), &lv_font_montserrat_14);
    lv_style_set_text_color(&(current_style.secondary_labels), palette.secondary_text);

    // Value Labels
    lv_style_init(&(current_style.value_labels));
    lv_style_set_text_color(&(current_style.value_labels), palette.text);

    // Flex Container
    lv_style_init(&(current_style.flex_container));
    lv_style_set_bg_color(&(current_style.flex_container), palette.bg);
    lv_style_set_bg_opa(&(current_style.flex_container), LV_OPA_COVER);
    lv_style_set_border_width(&(current_style.flex_container), 0);

    // History Item Container (Individual Card/Item styling)
    lv_style_init(&(current_style.history_item_container));
    lv_style_set_bg_color(&(current_style.history_item_container), palette.bg);
    lv_style_set_bg_grad_color(&(current_style.history_item_container), palette.bg_grad);
    lv_style_set_bg_grad_dir(&(current_style.history_item_container), LV_GRAD_DIR_VER);
    lv_style_set_bg_opa(&(current_style.history_item_container), LV_OPA_COVER);
    lv_style_set_border_color(&(current_style.history_item_container), palette.border);
    lv_style_set_border_width(&(current_style.history_item_container), 1);
    lv_style_set_radius(&(current_style.history_item_container), 6);
    lv_style_set_pad_all(&(current_style.history_item_container), 6); // Inner padding for child elements
    lv_style_set_shadow_color(&(current_style.history_item_container), palette.bg_dark);
    lv_style_set_shadow_width(&(current_style.history_item_container), 4);
    lv_style_set_shadow_ofs_y(&(current_style.history_item_container), 2);
    lv_style_set_shadow_opa(&(current_style.history_item_container), LV_OPA_COVER);
}

ui_styles_t *get_current_ui_styles(void) {
    return &current_style;
}
