#include QMK_KEYBOARD_H
#include "screen.h"
#include "lvgl.h"
#include "qp.h"
#include "ui_elements.h"
#include <stdio.h>
#include <string.h>

#define HISTORY_DEPTH 6 // History items displayed on the lower screen

static lv_obj_t *ui_screen;

// 1. Live Active Line
static char      live_buffer[32] = "-";

// 2. History Lines
static lv_obj_t *label_history[HISTORY_DEPTH];
static char      history_buffers[HISTORY_DEPTH][32];

// Core State Tracking Flags
static bool update_needed     = true;
static bool chord_has_content = false;
static bool combo_executed    = false; // Prevents trailing bare-mod releases from clogging history

// View containers for full-screen state toggling
static lv_obj_t *cont_active_fullscreen  = NULL;
static lv_obj_t *cont_history_fullscreen = NULL;
static lv_obj_t *mod_flex_container      = NULL; // Declared here so render_modifier_badges can access it

painter_device_t lcd;

/**
 * @brief External/internal handler to update active key strings and trigger a display refresh
 */
void update_active_modifiers_display(const char *new_live_str) {
    if (new_live_str != NULL) {
        strncpy(live_buffer, new_live_str, sizeof(live_buffer) - 1);
        live_buffer[sizeof(live_buffer) - 1] = '\0';
        update_needed                        = true;
    }
}

/**
 * @brief Clears and dynamically populates badge containers for each modifier key in live_buffer
 */
static void render_modifier_badges(const char *buffer) {
    if (!mod_flex_container) return;

    // Clear previous badge objects from container
    lv_obj_clean(mod_flex_container);

    if (buffer == NULL || buffer[0] == '\0' || strcmp(buffer, "-") == 0) return;

    ui_styles_t *styles = get_current_ui_styles();

    // Copy buffer so strtok does not mutate original string
    char buf_copy[32];
    strncpy(buf_copy, buffer, sizeof(buf_copy) - 1);
    buf_copy[sizeof(buf_copy) - 1] = '\0';

    char *token = strtok(buf_copy, "+");
    bool  first = true;

    while (token != NULL) {
        // --- 1. Render '+' Separator Box ---
        if (!first) {
            lv_obj_t *plus_cnt = lv_obj_create(mod_flex_container);
            lv_obj_remove_style_all(plus_cnt);
            lv_obj_set_size(plus_cnt, LV_SIZE_CONTENT, LV_SIZE_CONTENT);

            lv_obj_t *plus_lbl = lv_label_create(plus_cnt);
            lv_label_set_text(plus_lbl, "+");
            // Apply active_combo_label style so text color (palette.text) and font are inherited
            lv_obj_add_style(plus_lbl, &(styles->active_combo_label), LV_PART_MAIN);
        }

        // --- 2. Render Modifier / Key Badge Container ---
        lv_obj_t *badge_cnt = lv_obj_create(mod_flex_container);
        lv_obj_add_style(badge_cnt, &(styles->history_item_container), LV_PART_MAIN);
        lv_obj_set_size(badge_cnt, LV_SIZE_CONTENT, LV_SIZE_CONTENT);

        lv_obj_t *badge_lbl = lv_label_create(badge_cnt);
        lv_label_set_text(badge_lbl, token);

        // Apply active_combo_label style for text color & 24pt font
        lv_obj_add_style(badge_lbl, &(styles->active_combo_label), LV_PART_MAIN);

        first = false;
        token = strtok(NULL, "+");
    }
}

// Map standard keycodes to readable short strings
static void append_keycode_name(char *buf, uint16_t keycode, size_t buf_size) {
    if (keycode >= KC_A && keycode <= KC_Z) {
        char key_str[2] = {'A' + (keycode - KC_A), '\0'};
        strncat(buf, key_str, buf_size - strlen(buf) - 1);
    } else if (keycode >= KC_1 && keycode <= KC_0) {
        char num_map[]  = "1234567890";
        char key_str[2] = {num_map[keycode - KC_1], '\0'};
        strncat(buf, key_str, buf_size - strlen(buf) - 1);
    } else {
        switch (keycode) {
            case KC_ENTER:
                strncat(buf, "RET", buf_size - strlen(buf) - 1);
                break;
            case KC_ESCAPE:
                strncat(buf, "ESC", buf_size - strlen(buf) - 1);
                break;
            case KC_BACKSPACE:
                strncat(buf, "BSPC", buf_size - strlen(buf) - 1);
                break;
            case KC_TAB:
                strncat(buf, "TAB", buf_size - strlen(buf) - 1);
                break;
            case KC_SPACE:
                strncat(buf, "SPC", buf_size - strlen(buf) - 1);
                break;
            case KC_DELETE:
                strncat(buf, "DEL", buf_size - strlen(buf) - 1);
                break;
            case KC_DOT:
                strncat(buf, ".", buf_size - strlen(buf) - 1);
                break;
            case KC_COMMA:
                strncat(buf, ",", buf_size - strlen(buf) - 1);
                break;
            case KC_SLASH:
                strncat(buf, "/", buf_size - strlen(buf) - 1);
                break;
            case KC_BACKSLASH:
                strncat(buf, "\\", buf_size - strlen(buf) - 1);
                break;
            case KC_SEMICOLON:
                strncat(buf, ";", buf_size - strlen(buf) - 1);
                break;
            case KC_QUOTE:
                strncat(buf, "'", buf_size - strlen(buf) - 1);
                break;
            case KC_GRAVE:
                strncat(buf, "`", buf_size - strlen(buf) - 1);
                break;
            case KC_MINUS:
                strncat(buf, "-", buf_size - strlen(buf) - 1);
                break;
            case KC_EQUAL:
                strncat(buf, "=", buf_size - strlen(buf) - 1);
                break;
            case KC_LBRC:
                strncat(buf, "[", buf_size - strlen(buf) - 1);
                break;
            case KC_RBRC:
                strncat(buf, "]", buf_size - strlen(buf) - 1);
                break;
            default:
                strncat(buf, "KEY", buf_size - strlen(buf) - 1);
                break;
        }
    }
}

// Convert keycode directly into a modifier mask
static uint8_t keycode_to_mod_mask(uint16_t keycode) {
    uint8_t mod_mask = 0;
    switch (keycode) {
        case KC_LCTL:
        case KC_RCTL:
            return (MOD_BIT(KC_LCTL) | MOD_BIT(KC_RCTL));
        case KC_LSFT:
        case KC_RSFT:
            return (MOD_BIT(KC_LSFT) | MOD_BIT(KC_RSFT));
        case KC_LALT:
        case KC_RALT:
            return (MOD_BIT(KC_LALT) | MOD_BIT(KC_RALT));
        case KC_LGUI:
        case KC_RGUI:
            return (MOD_BIT(KC_LGUI) | MOD_BIT(KC_RGUI));
    }

    if (keycode >= QK_MOD_TAP && keycode <= QK_MOD_TAP_MAX) {
        uint8_t mod_type = (keycode >> 8) & 0x1F;
        if (mod_type & 0x01) mod_mask |= (MOD_BIT(KC_LCTL) | MOD_BIT(KC_RCTL));
        if (mod_type & 0x02) mod_mask |= (MOD_BIT(KC_LSFT) | MOD_BIT(KC_RSFT));
        if (mod_type & 0x04) mod_mask |= (MOD_BIT(KC_LALT) | MOD_BIT(KC_RALT));
        if (mod_type & 0x08) mod_mask |= (MOD_BIT(KC_LGUI) | MOD_BIT(KC_RGUI));
    }
    return mod_mask;
}

// Build string for active modifiers
static void build_modifier_string(char *buf, uint8_t mods, size_t buf_size) {
    buf[0] = '\0';
    if (mods & MOD_MASK_CTRL) strncat(buf, "CTL+", buf_size - strlen(buf) - 1);
    if (mods & MOD_MASK_ALT) strncat(buf, "ALT+", buf_size - strlen(buf) - 1);
    if (mods & MOD_MASK_SHIFT) strncat(buf, "SFT+", buf_size - strlen(buf) - 1);
    if (mods & MOD_MASK_GUI) strncat(buf, "GUI+", buf_size - strlen(buf) - 1);
}

// Push item down the history stack
static void push_to_history(const char *entry) {
    if (entry == NULL || strlen(entry) == 0 || strcmp(entry, "-") == 0) return;

    for (int i = HISTORY_DEPTH - 1; i > 0; i--) {
        strncpy(history_buffers[i], history_buffers[i - 1], sizeof(history_buffers[i]));
    }
    strncpy(history_buffers[0], entry, sizeof(history_buffers[0]));
}

// --- Press-Driven Key Capture Hook ---
bool process_record_custom_dashboard(uint16_t keycode, keyrecord_t *record) {
    bool is_tap_hold = (keycode >= QK_MOD_TAP && keycode <= QK_MOD_TAP_MAX) || (keycode >= QK_LAYER_TAP && keycode <= QK_LAYER_TAP_MAX);

    uint16_t basic_keycode = keycode;
    if (is_tap_hold || (keycode >= QK_MODS && keycode <= QK_MODS_MAX)) {
        basic_keycode = keycode & 0xFF;
    }

    bool is_bare_mod = (basic_keycode == KC_LCTL || basic_keycode == KC_RCTL || basic_keycode == KC_LSFT || basic_keycode == KC_RSFT || basic_keycode == KC_LALT || basic_keycode == KC_RALT || basic_keycode == KC_LGUI || basic_keycode == KC_RGUI);

    uint8_t key_mod_mask = keycode_to_mod_mask(keycode);
    uint8_t real_mods    = get_mods();

    // -------------------------------------------------------------
    // EVENT 1: KEY RELEASED
    // -------------------------------------------------------------
    if (!record->event.pressed) {
        uint8_t remaining_mods = real_mods & ~key_mod_mask;

        // Fast tapping of home-row mod keys (typing 'f' without holding)
        if (is_tap_hold && record->tap.count > 0 && remaining_mods == 0) {
            update_active_modifiers_display("-");
            chord_has_content = false;
            return true;
        }

        if (chord_has_content) {
            if (remaining_mods > 0) {
                char mod_buf[32] = "";
                build_modifier_string(mod_buf, remaining_mods, sizeof(mod_buf));
                if (strlen(mod_buf) > 0 && mod_buf[strlen(mod_buf) - 1] == '+') {
                    mod_buf[strlen(mod_buf) - 1] = '\0';
                }
                update_active_modifiers_display(mod_buf);
            } else {
                if (is_bare_mod && !combo_executed) {
                    push_to_history(live_buffer);
                }

                update_active_modifiers_display("-");
                chord_has_content = false;
                combo_executed    = false;
            }
        }
        return true;
    }

    // -------------------------------------------------------------
    // EVENT 2: KEY PRESSED
    // -------------------------------------------------------------
    uint8_t active_mods = real_mods;
    if (is_bare_mod || (is_tap_hold && record->tap.count == 0)) {
        active_mods |= key_mod_mask;
    }

    // 2A. Bare Modifier press OR Tap-Hold key held long enough
    if (is_bare_mod || (is_tap_hold && record->tap.count == 0)) {
        char mod_buf[32] = "";

        if (keycode >= QK_LAYER_TAP && keycode <= QK_LAYER_TAP_MAX) {
            uint8_t layer = (keycode >> 8) & 0x0F;
            snprintf(mod_buf, sizeof(mod_buf), "LAYER %d", layer);
        } else {
            build_modifier_string(mod_buf, active_mods, sizeof(mod_buf));
        }

        if (strlen(mod_buf) > 0) {
            if (mod_buf[strlen(mod_buf) - 1] == '+') {
                mod_buf[strlen(mod_buf) - 1] = '\0';
            }
            update_active_modifiers_display(mod_buf);
            chord_has_content = true;

            if (active_mods == key_mod_mask) {
                combo_executed = false;
            }
        }
        return true;
    }

    // 2B. Non-modifier key pressed while modifiers are active (INSTANT LOGGING)
    if (active_mods > 0) {
        char chord_buf[32] = "";
        build_modifier_string(chord_buf, active_mods, sizeof(chord_buf));
        append_keycode_name(chord_buf, basic_keycode, sizeof(chord_buf));

        combo_executed = true;

        push_to_history(chord_buf);
        update_active_modifiers_display(chord_buf);

        chord_has_content = true;
    }

    return true;
}

/**
 * @brief Initialize dashboard screens, layout elements, and container hierarchies
 */
void init_custom_dashboard(void) {
    wait_ms(LCD_WAIT_TIME);

    lcd = qp_st7789_make_spi_device(LCD_WIDTH, LCD_HEIGHT, LCD_CS_PIN, LCD_DC_PIN, LCD_RST_PIN, LCD_SPI_DIVISOR, SPI_MODE);
    qp_init(lcd, LCD_ROTATION);
    qp_set_viewport_offsets(lcd, LCD_OFFSET_X, LCD_OFFSET_Y);
    qp_lvgl_attach(lcd);
    qp_power(lcd, 1);

    init_styles();

    for (int i = 0; i < HISTORY_DEPTH; i++) {
        strcpy(history_buffers[i], "-");
    }

    ui_screen = lv_obj_create(NULL);
    ui_styles_t *styles = get_current_ui_styles();

    // ==========================================
    // 1. FULL-SCREEN ACTIVE / MODIFIER OVERLAY
    // ==========================================
    cont_active_fullscreen = ui_create_container(ui_screen);
    lv_obj_set_size(cont_active_fullscreen, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_all(cont_active_fullscreen, 8, LV_PART_MAIN);
    lv_obj_add_style(cont_active_fullscreen, &(styles->history_item_container), LV_PART_MAIN);

    ui_create_secondary_text(cont_active_fullscreen, "ACTIVE MODIFIERS", true, 1);

    mod_flex_container = lv_obj_create(cont_active_fullscreen);
    lv_obj_remove_style_all(mod_flex_container);
    lv_obj_set_width(mod_flex_container, LV_PCT(100));
    lv_obj_set_height(mod_flex_container, LV_SIZE_CONTENT);
    lv_obj_set_layout(mod_flex_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(mod_flex_container, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(mod_flex_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(mod_flex_container, 4, LV_PART_MAIN);
    lv_obj_set_style_pad_row(mod_flex_container, 4, LV_PART_MAIN);

    lv_obj_add_flag(cont_active_fullscreen, LV_OBJ_FLAG_HIDDEN);

    // ==========================================
    // 2. FULL-SCREEN HISTORY VIEW (NO MODS)
    // ==========================================
    cont_history_fullscreen = ui_create_container(ui_screen);
    lv_obj_set_size(cont_history_fullscreen, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_all(cont_history_fullscreen, 8, LV_PART_MAIN);

    ui_create_secondary_text(cont_history_fullscreen, "RECENT HISTORY", true, 1);
    ui_create_line_separator(cont_history_fullscreen, 1, 2);

    lv_obj_t *history_list_cont = lv_obj_create(cont_history_fullscreen);
    lv_obj_add_style(history_list_cont, &(styles->flex_container), LV_PART_MAIN);
    lv_obj_set_layout(history_list_cont, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(history_list_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(history_list_cont, 4, LV_PART_MAIN);
    lv_obj_set_width(history_list_cont, LV_PCT(100));
    lv_obj_set_height(history_list_cont, LV_SIZE_CONTENT);

    for (int i = 0; i < HISTORY_DEPTH; i++) {
        lv_obj_t *item_cnt = lv_obj_create(history_list_cont);
        lv_obj_add_style(item_cnt, &(styles->history_item_container), LV_PART_MAIN);
        lv_obj_set_size(item_cnt, LV_PCT(100), LV_SIZE_CONTENT);

        label_history[i] = lv_label_create(item_cnt);
        lv_label_set_text(label_history[i], "-");

        // Inherit text color from active_combo_label theme style
        lv_obj_add_style(label_history[i], &(styles->active_combo_label), LV_PART_MAIN);
        // Override font to 14pt for the smaller history items
        lv_obj_set_style_text_font(label_history[i], &lv_font_montserrat_14, LV_PART_MAIN);
    }
}

void load_custom_dashboard(void) {
    lv_scr_load(ui_screen);
}

/**
 * @brief Housekeeping loop called on display redraw ticks
 */
void housekeeping_custom_dashboard(void) {
    if (!is_keyboard_left()) return;

    if (update_needed) {
        bool has_active_modifiers = (live_buffer[0] != '\0' && strcmp(live_buffer, "-") != 0);

        if (has_active_modifiers) {
            lv_obj_clear_flag(cont_active_fullscreen, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(cont_history_fullscreen, LV_OBJ_FLAG_HIDDEN);

            render_modifier_badges(live_buffer);
        } else {
            lv_obj_clear_flag(cont_history_fullscreen, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(cont_active_fullscreen, LV_OBJ_FLAG_HIDDEN);

            for (int i = 0; i < HISTORY_DEPTH; i++) {
                lv_label_set_text(label_history[i], history_buffers[i]);
            }
        }

        update_needed = false;
    }
}

lcd_module_t lcd_module_dashboard = {
    .init_module                                      = &init_custom_dashboard,
    .load_custom_theme_elements                       = NULL,
    .load_module                                      = &load_custom_dashboard,
    .update_custom_elements_styles_from_current_theme = NULL,
    .process_record                                   = &process_record_custom_dashboard,
    .housekeeping_task                                = &housekeeping_custom_dashboard,
};
