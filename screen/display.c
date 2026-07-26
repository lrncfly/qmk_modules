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
static lv_obj_t *label_live_combo;
static char      live_buffer[32] = "-";

// 2. History Lines
static lv_obj_t *label_history[HISTORY_DEPTH];
static char      history_buffers[HISTORY_DEPTH][32];

// Core State Tracking Flags
static bool update_needed     = true;
static bool chord_has_content = false;
static bool combo_executed    = false; // Prevents trailing bare-mod releases from clogging history

painter_device_t lcd;

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

    // Clear BOTH Left and Right variants on release so no ghost shift remains
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

    // Handle Mod-Tap / Layer-Tap variations
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

// --- Fully Synchronized Key Capture Hook ---
// --- Press-Driven Key Capture Hook ---
bool process_record_custom_dashboard(uint16_t keycode, keyrecord_t *record) {
    bool is_tap_hold = (keycode >= QK_MOD_TAP && keycode <= QK_MOD_TAP_MAX) || (keycode >= QK_LAYER_TAP && keycode <= QK_LAYER_TAP_MAX);

    uint16_t basic_keycode = keycode;
    if (is_tap_hold) {
        basic_keycode = keycode & 0xFF;
    } else if (keycode >= QK_MODS && keycode <= QK_MODS_MAX) {
        basic_keycode = keycode & 0xFF;
    }

    bool is_bare_mod = (basic_keycode == KC_LCTL || basic_keycode == KC_RCTL || basic_keycode == KC_LSFT || basic_keycode == KC_RSFT || basic_keycode == KC_LALT || basic_keycode == KC_RALT || basic_keycode == KC_LGUI || basic_keycode == KC_RGUI);

    uint8_t key_mod_mask = keycode_to_mod_mask(keycode);

    // Filter strictly for physical/real modifiers — ignoring weak and oneshot mods
    uint8_t real_mods = get_mods();

    // -------------------------------------------------------------
    // EVENT 1: KEY RELEASED
    // -------------------------------------------------------------
    if (!record->event.pressed) {
        // Calculate remaining real modifiers after stripping the releasing key
        uint8_t remaining_mods = real_mods & ~key_mod_mask;

        // Fast tapping of home-row mod keys (typing 'f' without holding)
        if (is_tap_hold && record->tap.count > 0 && remaining_mods == 0) {
            strcpy(live_buffer, "-");
            chord_has_content = false;
            update_needed     = true;
            return true;
        }

        if (chord_has_content) {
            // Re-evaluate active display based solely on remaining held modifiers
            if (remaining_mods > 0) {
                char mod_buf[32] = "";
                build_modifier_string(mod_buf, remaining_mods, sizeof(mod_buf));
                if (strlen(mod_buf) > 0 && mod_buf[strlen(mod_buf) - 1] == '+') {
                    mod_buf[strlen(mod_buf) - 1] = '\0';
                }
                strncpy(live_buffer, mod_buf, sizeof(live_buffer));
            } else {
                // Bare modifier release: if held alone without executing a chord, log it to history
                if (is_bare_mod && !combo_executed) {
                    push_to_history(live_buffer);
                }

                // Reset active line to idle
                strcpy(live_buffer, "-");
                chord_has_content = false;
                combo_executed    = false;
            }

            update_needed = true;
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
            strncpy(live_buffer, mod_buf, sizeof(live_buffer));
            chord_has_content = true;

            // Reset combo flag if starting a fresh modifier hold
            if (active_mods == key_mod_mask) {
                combo_executed = false;
            }
            update_needed = true;
        }
        return true;
    }

    // 2B. Non-modifier key pressed while modifiers are active (INSTANT LOGGING)
    if (active_mods > 0) {
        char chord_buf[32] = "";
        build_modifier_string(chord_buf, active_mods, sizeof(chord_buf));
        append_keycode_name(chord_buf, basic_keycode, sizeof(chord_buf));

        // Mark as combo so the bare modifier won't duplicate to history on release
        combo_executed = true;

        // Instantly push to history and update live buffer on key press
        push_to_history(chord_buf);
        strncpy(live_buffer, chord_buf, sizeof(live_buffer));

        chord_has_content = true;
        update_needed     = true;
    }

    return true;
}

// --- Initialization ---
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

    // Main layout container
    lv_obj_t *main_cont = ui_create_container(ui_screen);
    lv_obj_set_style_pad_row(main_cont, 2, LV_PART_MAIN);

    ui_create_secondary_text(main_cont, "ACTIVE & HISTORY", true, 1);

    // Live slot (Top)
    label_live_combo = ui_create_number_label(main_cont, 2);
    lv_label_set_text(label_live_combo, live_buffer);

    ui_create_line_separator(main_cont, 1, 2);

    // 2. Create a parent container dedicated specifically to history items
    lv_obj_t    *history_list_cont = lv_obj_create(main_cont);
    ui_styles_t *styles            = get_current_ui_styles();
    lv_obj_add_style(history_list_cont, &(styles->flex_container), LV_PART_MAIN);
    lv_obj_set_layout(history_list_cont, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(history_list_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(history_list_cont, 4, LV_PART_MAIN); // Gap between item cards
    lv_obj_set_width(history_list_cont, LV_PCT(100));
    lv_obj_set_height(history_list_cont, LV_SIZE_CONTENT);

    // 3. Populate history slots using item containers
    for (int i = 0; i < HISTORY_DEPTH; i++) {
        // Individual item card wrapper
        lv_obj_t *item_cnt = lv_obj_create(history_list_cont);
        lv_obj_add_style(item_cnt, &(styles->history_item_container), LV_PART_MAIN);
        lv_obj_set_width(item_cnt, LV_PCT(100));
        lv_obj_set_height(item_cnt, LV_SIZE_CONTENT);

        // Label inside its own container
        label_history[i] = ui_create_secondary_text(item_cnt, history_buffers[i], false, 1);
        lv_obj_set_style_pad_top(label_history[i], 1, LV_PART_MAIN);
        lv_obj_set_style_pad_bottom(label_history[i], 1, LV_PART_MAIN);
    }
}

void load_custom_dashboard(void) {
    lv_scr_load(ui_screen);
}

// --- Render Loop ---
void housekeeping_custom_dashboard(void) {
    if (!is_keyboard_left()) return;

    if (update_needed) {
        lv_label_set_text(label_live_combo, live_buffer);

        for (int i = 0; i < HISTORY_DEPTH; i++) {
            lv_label_set_text(label_history[i], history_buffers[i]);
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
