#include QMK_KEYBOARD_H
#include "screen.h"
#include "screen_data.h"
#include "lvgl.h"
#include "qp.h"
#include "color.h"
#include "ui_elements.h"
#include "keycode_string.h"
#include <string.h>

// Master Screen Slate
static lv_obj_t *ui_screen;
static lv_obj_t *main_cont;
static lv_obj_t *chord_cont;
static lv_obj_t *chord_layer_label;
static lv_obj_t *chord_label;

// 1. Persistent Header Containers & Widgets
static lv_obj_t *label_status_tag;

typedef struct {
    uint8_t mod_mask;
    const char *name;
} modifier_entry_t;

static const modifier_entry_t modifiers[] = {
    {MOD_MASK_CTRL, "CTRL"},
    {MOD_MASK_SHIFT, "SHIFT"},
    {MOD_MASK_ALT, "ALT"},
    {MOD_MASK_GUI, "GUI"},
};

static uint8_t modifier_order[sizeof(modifiers) / sizeof(modifiers[0])];
static uint8_t modifier_count;
static uint8_t previous_mods;
static bool chord_complete;
static bool chord_display_timed_out;
static uint32_t chord_display_completed_at;
static uint16_t last_chord_event_time;
static uint16_t last_chord_keycode;
static uint8_t last_chord_event_row;
static uint8_t last_chord_event_col;
static bool last_chord_event_valid;
static char displayed_chord[64];

// 2. Contextual Dynamic View Containers
static lv_obj_t *cont_default_view;
static lv_obj_t *cont_pointer_view;
static lv_obj_t *cont_media_view;

// 3. Dynamic Widget Value Trackers
#ifdef WPM_ENABLE
static lv_obj_t *label_wpm_value;
static lv_obj_t *bar_wpm;
#endif
#if LCD_CHORD_HISTORY_COUNT > 0
static lv_obj_t *label_chord_history_title;
static lv_obj_t *label_chord_history;
#endif

static lv_obj_t *bar_dpi;
static lv_obj_t *label_dpi_val;
static lv_obj_t *bar_snipe;
static lv_obj_t *label_snipe_val;

static lv_obj_t *bar_lcd;
static lv_obj_t *label_lcd_val;
static lv_obj_t *bar_rgb;
static lv_obj_t *label_rgb_val;

typedef struct {
    uint16_t value;
    uint16_t percentage;
    bool enabled;
    bool valid;
} numeric_display_cache_t;

#ifdef WPM_ENABLE
static numeric_display_cache_t wpm_display_cache;
#endif
static numeric_display_cache_t dpi_display_cache;
static numeric_display_cache_t snipe_display_cache;
static numeric_display_cache_t lcd_display_cache;
static numeric_display_cache_t rgb_display_cache;

painter_device_t lcd; // Global pointer for the driver

static uint32_t last_screen_activity;
static uint8_t screen_backlight_level;
static bool screen_timed_out;
#ifdef RGB_MATRIX_ENABLE
static RGB last_displayed_layer_rgb;
static bool displayed_layer_rgb_valid;
#endif

#if LCD_CHORD_HISTORY_COUNT > 0
#    define CHORD_HISTORY_ENTRY_SIZE 64
static char chord_history[LCD_CHORD_HISTORY_COUNT][CHORD_HISTORY_ENTRY_SIZE];
static uint32_t chord_history_times[LCD_CHORD_HISTORY_COUNT];
static uint8_t chord_history_count;
#endif

static void update_layer_display(const screen_dashboard_data_t *data) {
    lv_label_set_text(label_status_tag, data->status_text);
    lv_label_set_text(chord_layer_label, data->layer_name);
#ifdef RGB_MATRIX_ENABLE
    last_displayed_layer_rgb = data->layer_rgb;
    displayed_layer_rgb_valid = true;
    lv_color_t background = lv_color_make(data->layer_rgb.r, data->layer_rgb.g, data->layer_rgb.b);
    lv_color_t foreground = lv_color_white();
    if ((uint32_t)data->layer_rgb.r * 299 + (uint32_t)data->layer_rgb.g * 587 +
            (uint32_t)data->layer_rgb.b * 114 >
        128000) {
        foreground = lv_color_black();
    }

    lv_obj_t *status_button = lv_obj_get_parent(label_status_tag);
    lv_obj_set_style_bg_color(status_button, background, LV_PART_MAIN);
    lv_obj_set_style_bg_grad_color(status_button, background, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(status_button, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_text_color(label_status_tag, foreground, LV_PART_MAIN);

    lv_obj_set_style_bg_color(chord_layer_label, background, LV_PART_MAIN);
    lv_obj_set_style_bg_grad_color(chord_layer_label, background, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(chord_layer_label, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_text_color(chord_layer_label, foreground, LV_PART_MAIN);
#endif
}

static void update_numeric_display(lv_obj_t *label, lv_obj_t *bar, uint16_t value, uint16_t percentage, bool enabled,
                                   numeric_display_cache_t *cache) {
    if (!cache->valid || cache->enabled != enabled || (enabled && cache->value != value)) {
        char text[12];
        if (enabled) {
            snprintf(text, sizeof(text), "%u", value);
        } else {
            snprintf(text, sizeof(text), "Off");
        }
        lv_label_set_text(label, text);
        cache->value = value;
    }
    if (!cache->valid || cache->percentage != percentage) {
        lv_bar_set_value(bar, percentage, LV_ANIM_OFF);
        cache->percentage = percentage;
    }
    cache->enabled = enabled;
    cache->valid = true;
}

static uint16_t value_percentage(uint16_t value, uint16_t minimum, uint16_t maximum) {
    if (value <= minimum) {
        return 0;
    }
    if (value >= maximum) {
        return 100;
    }
    return ((uint32_t)(value - minimum) * 100U) / (maximum - minimum);
}

// --- Initialization Phase ---
void init_custom_dashboard(void) {
    // 1. Low-level hardware initialization sequence
    wait_ms(LCD_WAIT_TIME);

    lcd = qp_st7789_make_spi_device(LCD_WIDTH, LCD_HEIGHT, LCD_CS_PIN, LCD_DC_PIN, LCD_RST_PIN, LCD_SPI_DIVISOR, SPI_MODE);
    qp_init(lcd, LCD_ROTATION);
    qp_set_viewport_offsets(lcd, LCD_OFFSET_X, LCD_OFFSET_Y);

    // This dynamically hooks LVGL's internal memory manager to the display buffer
    qp_lvgl_attach(lcd);

    // Power display screen on
    qp_power(lcd, 1);
    qp_rect(lcd, 0, 0, LCD_WIDTH + LCD_MARGIN, LCD_HEIGHT + LCD_MARGIN, HSV_BLACK, true);
    qp_flush(lcd);

    // 2. Load the general formatting themes
    load_themes();
    init_styles();

    // 3. NOW it is 100% safe to build your layout objects!
    ui_screen = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(ui_screen, LV_SCROLLBAR_MODE_OFF);

    // Create the master base column wrapper
    main_cont = ui_create_container(ui_screen);

    // ==========================================
    // PERSISTENT ZONE (Always visible at top)
    // ==========================================
    // Header Zone: Status Indicator (Primary/Secondary)
    label_status_tag = ui_create_layer_label(main_cont);
    lv_label_set_text(label_status_tag, "");

    ui_create_line_separator(main_cont, 1, 3);

    // ==========================================
    // VIEW A: DEFAULT VIEW (WPM Meter)
    // ==========================================
    cont_default_view = ui_create_container(main_cont);
#ifdef WPM_ENABLE
    ui_create_secondary_text(cont_default_view, "WPM", true, 1);
    label_wpm_value = ui_create_number_label(cont_default_view, 2);
    lv_label_set_text(label_wpm_value, "0");
    bar_wpm = ui_create_progress_bar(cont_default_view, 4);
    lv_bar_set_value(bar_wpm, 0, LV_ANIM_OFF);
#endif

#if LCD_CHORD_HISTORY_COUNT > 0
    label_chord_history_title = ui_create_secondary_text(cont_default_view, "RECENT CHORDS", true, 1);
    label_chord_history = ui_create_secondary_text(cont_default_view, "", true, 1);
    lv_obj_set_width(label_chord_history, LCD_WIDTH - 24);
    lv_label_set_long_mode(label_chord_history, LV_LABEL_LONG_WRAP);
    lv_obj_add_flag(label_chord_history_title, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(label_chord_history, LV_OBJ_FLAG_HIDDEN);
#endif

    // ==========================================
    // VIEW B: POINTER VIEW (DPI / Snipe metrics)
    // ==========================================
    cont_pointer_view = ui_create_container(main_cont);

    ui_create_secondary_text(cont_pointer_view, "DPI", true, 1);
    label_dpi_val = ui_create_number_label(cont_pointer_view, 1);
    bar_dpi       = ui_create_progress_bar(cont_pointer_view, 4);

    ui_create_secondary_text(cont_pointer_view, "SNIPE", true, 1);
    label_snipe_val = ui_create_number_label(cont_pointer_view, 1);
    bar_snipe       = ui_create_progress_bar(cont_pointer_view, 4);

    // ==========================================
    // VIEW C: MEDIA VIEW (RGB / LCD metrics)
    // ==========================================
    cont_media_view = ui_create_container(main_cont);

    ui_create_secondary_text(cont_media_view, "LCD", true, 1);
    label_lcd_val = ui_create_number_label(cont_media_view, 1);
    bar_lcd       = ui_create_progress_bar(cont_media_view, 4);

    ui_create_secondary_text(cont_media_view, "RGB", true, 1);
    label_rgb_val = ui_create_number_label(cont_media_view, 1);
    bar_rgb       = ui_create_progress_bar(cont_media_view, 4);

    chord_cont = lv_obj_create(ui_screen);
    lv_obj_clear_flag(chord_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(chord_cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_size(chord_cont, LCD_WIDTH, LCD_HEIGHT);
    lv_obj_center(chord_cont);
    lv_obj_add_style(chord_cont, &get_current_ui_styles()->flex_container, 0);
    lv_obj_add_flag(chord_cont, LV_OBJ_FLAG_HIDDEN);

    chord_layer_label = lv_label_create(chord_cont);
    lv_obj_add_style(chord_layer_label, &get_current_ui_styles()->secondary_labels, 0);
    lv_obj_set_style_text_color(chord_layer_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(chord_layer_label, LV_ALIGN_TOP_MID, 0, 12);

    chord_label = lv_label_create(chord_cont);
    lv_obj_add_style(chord_label, &get_current_ui_styles()->secondary_labels, 0);
    lv_obj_set_style_text_font(chord_label, &lv_font_montserrat_28, LV_PART_MAIN);
    lv_obj_set_width(chord_label, LCD_WIDTH - 24);
    lv_obj_set_height(chord_label, LCD_HEIGHT - 56);
    lv_label_set_long_mode(chord_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(chord_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(chord_label, lv_color_white(), 0);
    lv_obj_set_style_text_line_space(chord_label, 8, 0);
    lv_obj_align(chord_label, LV_ALIGN_TOP_MID, 0, 48);
    screen_dashboard_data_t dashboard_data;
    lrncfly_screen_get_dashboard_data(&dashboard_data);
    update_layer_display(&dashboard_data);

    // Default visibility settings at startup
    lv_obj_clear_flag(cont_default_view, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(cont_pointer_view, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(cont_media_view, LV_OBJ_FLAG_HIDDEN);

    screen_backlight_level = get_backlight_level();
    last_screen_activity = timer_read32();
    screen_timed_out = false;
#ifdef WPM_ENABLE
    wpm_display_cache.valid = false;
#endif
    dpi_display_cache.valid = false;
    snipe_display_cache.valid = false;
    lcd_display_cache.valid = false;
    rgb_display_cache.valid = false;
    modifier_count = 0;
    previous_mods = 0;
    chord_complete = false;
    chord_display_timed_out = false;
    last_chord_event_valid = false;
    displayed_chord[0] = '\0';
#if LCD_CHORD_HISTORY_COUNT > 0
    chord_history_count = 0;
    memset(chord_history_times, 0, sizeof(chord_history_times));
#endif
}

void load_custom_dashboard(void) {
    lv_scr_load(ui_screen);
}

void screen_note_activity(void) {
    if (!is_keyboard_left()) return;

    last_screen_activity = timer_read32();
    if (screen_timed_out) {
        qp_power(lcd, 1);
        backlight_set(screen_backlight_level);
        screen_timed_out = false;
    } else {
        uint8_t current_backlight_level = get_backlight_level();
        if (current_backlight_level > 0) {
            screen_backlight_level = current_backlight_level;
        }
    }
}

static uint8_t update_modifier_sequence(void) {
    uint8_t current_mods = get_mods() | get_oneshot_mods();
    const uint8_t modifier_total = sizeof(modifiers) / sizeof(modifiers[0]);

    for (uint8_t i = 0; i < modifier_count;) {
        if (!(current_mods & modifiers[modifier_order[i]].mod_mask)) {
            memmove(&modifier_order[i], &modifier_order[i + 1], modifier_count - i - 1);
            modifier_count--;
        } else {
            i++;
        }
    }

    for (uint8_t i = 0; i < modifier_total; i++) {
        uint8_t mod_mask = modifiers[i].mod_mask;
        if ((current_mods & mod_mask) && !(previous_mods & mod_mask)) {
            modifier_order[modifier_count++] = i;
        }
    }
    previous_mods = current_mods;
    return current_mods;
}

static void show_chord(const char *terminal_key) {
    char text[sizeof(displayed_chord)];
    size_t text_length = 0;

    for (uint8_t i = 0; i < modifier_count; i++) {
        text_length += snprintf(&text[text_length], sizeof(text) - text_length, "%s\n", modifiers[modifier_order[i]].name);
    }

    if (terminal_key) {
        snprintf(&text[text_length], sizeof(text) - text_length, "%s", terminal_key);
    } else if (text_length > 0) {
        text[--text_length] = '\0';
    } else {
        text[0] = '\0';
    }

    if (strcmp(displayed_chord, text) != 0) {
        lv_label_set_text(chord_label, text);
        snprintf(displayed_chord, sizeof(displayed_chord), "%s", text);
    }

    lv_obj_add_flag(main_cont, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(chord_cont, LV_OBJ_FLAG_HIDDEN);
}

#if LCD_CHORD_HISTORY_COUNT > 0
static void update_chord_history(bool force_render) {
    bool history_changed = false;
#if LCD_CHORD_HISTORY_TIMEOUT > 0
    for (uint8_t i = 0; i < chord_history_count;) {
        if (timer_elapsed32(chord_history_times[i]) >= LCD_CHORD_HISTORY_TIMEOUT) {
            memmove(&chord_history[i], &chord_history[i + 1],
                    (chord_history_count - i - 1) * sizeof(chord_history[0]));
            memmove(&chord_history_times[i], &chord_history_times[i + 1],
                    (chord_history_count - i - 1) * sizeof(chord_history_times[0]));
            chord_history_count--;
            history_changed = true;
        } else {
            i++;
        }
    }
#endif

    if (!force_render && !history_changed) {
        return;
    }

    lv_obj_t *history_widgets[] = {label_chord_history_title, label_chord_history};
    if (chord_history_count == 0) {
        lv_label_set_text(label_chord_history, "");
        lv_obj_add_flag(history_widgets[0], LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(history_widgets[1], LV_OBJ_FLAG_HIDDEN);
        return;
    }

    lv_obj_clear_flag(history_widgets[0], LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(history_widgets[1], LV_OBJ_FLAG_HIDDEN);
    char history_text[LCD_CHORD_HISTORY_COUNT * CHORD_HISTORY_ENTRY_SIZE];
    size_t text_length = 0;
    for (uint8_t i = 0; i < chord_history_count; i++) {
        int written = snprintf(&history_text[text_length], sizeof(history_text) - text_length,
                               "%s%s", i == 0 ? "" : "\n", chord_history[i]);
        if (written < 0 || (size_t)written >= sizeof(history_text) - text_length) {
            break;
        }
        text_length += (size_t)written;
    }
    lv_label_set_text(label_chord_history, history_text);
}

static void record_chord_history(const char *terminal_key) {
    char entry[CHORD_HISTORY_ENTRY_SIZE];
    size_t length = 0;

    for (uint8_t i = 0; i < modifier_count && length < sizeof(entry) - 1; i++) {
        int written = snprintf(&entry[length], sizeof(entry) - length, "%s%s",
                               i == 0 ? "" : " + ", modifiers[modifier_order[i]].name);
        if (written < 0 || (size_t)written >= sizeof(entry) - length) {
            entry[sizeof(entry) - 1] = '\0';
            break;
        }
        length += (size_t)written;
    }

    if (terminal_key && length < sizeof(entry) - 1) {
        snprintf(&entry[length], sizeof(entry) - length, "%s%s", length == 0 ? "" : " + ", terminal_key);
    }

    size_t history_count = chord_history_count;
    if (history_count >= LCD_CHORD_HISTORY_COUNT) {
        history_count = LCD_CHORD_HISTORY_COUNT - 1;
    } else {
        chord_history_count++;
    }
    memmove(&chord_history[1], &chord_history[0], history_count * sizeof(chord_history[0]));
    memmove(&chord_history_times[1], &chord_history_times[0], history_count * sizeof(chord_history_times[0]));
    snprintf(chord_history[0], sizeof(chord_history[0]), "%s", entry);
    chord_history_times[0] = timer_read32();
    update_chord_history(true);
}
#endif

static void show_dashboard(void) {
    lv_obj_clear_flag(main_cont, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(chord_cont, LV_OBJ_FLAG_HIDDEN);
    displayed_chord[0] = '\0';
}

static char us_layout_character(uint16_t keycode, uint8_t mods) {
    if (IS_QK_MODS(keycode)) {
        mods |= QK_MODS_GET_MODS(keycode);
        keycode = QK_MODS_GET_BASIC_KEYCODE(keycode);
    }

    bool shifted = (mods & MOD_MASK_SHIFT) != 0;
    if (keycode >= KC_A && keycode <= KC_Z) {
        char letter = (char)('a' + keycode - KC_A);
        bool uppercase = shifted ^ host_keyboard_led_state().caps_lock;
        return uppercase ? (char)(letter - 'a' + 'A') : letter;
    }

    if (keycode >= KC_1 && keycode <= KC_9) {
        static const char unshifted[] = "123456789";
        static const char shifted_chars[] = "!@#$%^&*(";
        uint8_t index = keycode - KC_1;
        return shifted ? shifted_chars[index] : unshifted[index];
    }

    switch (keycode) {
        case KC_0: return shifted ? ')' : '0';
        case KC_MINS: return shifted ? '_' : '-';
        case KC_EQL: return shifted ? '+' : '=';
        case KC_LBRC: return shifted ? '{' : '[';
        case KC_RBRC: return shifted ? '}' : ']';
        case KC_BSLS: return shifted ? '|' : '\\';
        case KC_SCLN: return shifted ? ':' : ';';
        case KC_QUOT: return shifted ? '"' : '\'';
        case KC_GRV: return shifted ? '~' : '`';
        case KC_COMM: return shifted ? '<' : ',';
        case KC_DOT: return shifted ? '>' : '.';
        case KC_SLSH: return shifted ? '?' : '/';
        default: return '\0';
    }
}

static void format_terminal_key(uint16_t keycode, char *text, size_t text_size) {
    if (text_size == 0) return;

    char character = us_layout_character(keycode, get_mods() | get_oneshot_mods());
    if (character != '\0') {
        if (text_size < 2) {
            text[0] = '\0';
            return;
        }
        text[0] = character;
        text[1] = '\0';
        return;
    }

    const char *key_name = get_keycode_string(keycode);
    size_t output_index = 0;

    while (*key_name && output_index + 1 < text_size) {
        if (strncmp(key_name, "KC_", 3) == 0) {
            key_name += 3;
            continue;
        }
        text[output_index++] = *key_name == '_' ? ' ' : *key_name;
        key_name++;
    }
    text[output_index] = '\0';
}

void screen_process_keycode(uint16_t keycode, keyrecord_t *record) {
    if (!is_keyboard_left()) return;

    if (IS_QK_MOD_TAP(keycode)) {
        if (record->event.pressed || !record->tap.count) return;
        keycode = QK_MOD_TAP_GET_TAP_KEYCODE(keycode);
    } else if (IS_QK_LAYER_TAP(keycode)) {
        if (record->event.pressed || !record->tap.count) return;
        keycode = QK_LAYER_TAP_GET_TAP_KEYCODE(keycode);
    } else if (!record->event.pressed) {
        return;
    }

    if (keycode >= KC_LCTL && keycode <= KC_RGUI) {
        chord_display_timed_out = false;
        if (chord_complete) {
            chord_complete = false;
        }
        update_modifier_sequence();
        return;
    }

    uint8_t current_mods = get_mods() | get_oneshot_mods();
    if (current_mods != 0 && last_chord_event_valid &&
        last_chord_event_time == record->event.time &&
        last_chord_event_row == record->event.key.row &&
        last_chord_event_col == record->event.key.col &&
        last_chord_keycode == keycode) {
        return;
    }

    chord_display_timed_out = false;
    if (chord_complete) {
        chord_complete = false;
    }
    current_mods = update_modifier_sequence();

    if (current_mods != 0) {
        last_chord_event_time = record->event.time;
        last_chord_event_row = record->event.key.row;
        last_chord_event_col = record->event.key.col;
        last_chord_keycode = keycode;
        last_chord_event_valid = true;

        char terminal_key[16];
        format_terminal_key(keycode, terminal_key, sizeof(terminal_key));
#if LCD_CHORD_HISTORY_COUNT > 0
        record_chord_history(terminal_key);
#endif
        show_chord(terminal_key);
        chord_complete = true;
        chord_display_timed_out = false;
        chord_display_completed_at = timer_read32();
    } else if (modifier_count == 0) {
        show_dashboard();
    }
}

// --- Dynamic Rendering & Visibility Loop ---
void housekeeping_custom_dashboard(void) {
    if (!is_keyboard_left()) return;

#if LCD_SCREEN_TIMEOUT > 0
    if (!screen_timed_out && timer_elapsed32(last_screen_activity) >= LCD_SCREEN_TIMEOUT) {
        uint8_t current_backlight_level = get_backlight_level();
        if (current_backlight_level > 0) {
            screen_backlight_level = current_backlight_level;
        }
        backlight_set(0);
        qp_power(lcd, 0);
        screen_timed_out = true;
        return;
    }
#endif

    // 1. Resolve active keyboard state through the keymap data adapter
    screen_dashboard_data_t dashboard_data;
    lrncfly_screen_get_dashboard_data(&dashboard_data);
    uint8_t highest_layer = dashboard_data.layer;

    // 2. Run Context Visibility Toggling & Layer Name Updates
    static uint8_t last_rendered_layer = 255;
#ifdef RGB_MATRIX_ENABLE
    RGB current_layer_rgb = dashboard_data.layer_rgb;
    bool layer_color_changed = !displayed_layer_rgb_valid ||
                               current_layer_rgb.r != last_displayed_layer_rgb.r ||
                               current_layer_rgb.g != last_displayed_layer_rgb.g ||
                               current_layer_rgb.b != last_displayed_layer_rgb.b;
#endif
    bool layer_changed = highest_layer != last_rendered_layer;
    if (layer_changed) {
        // --- Container Visibility Toggling ---
        // Enforce total layout blackout
        lv_obj_add_flag(cont_default_view, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(cont_pointer_view, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(cont_media_view, LV_OBJ_FLAG_HIDDEN);

        // Selectively awake the target context container
        switch (dashboard_data.view) {
            case SCREEN_DASHBOARD_POINTER:
                lv_obj_clear_flag(cont_pointer_view, LV_OBJ_FLAG_HIDDEN);
                break;
            case SCREEN_DASHBOARD_MEDIA:
                lv_obj_clear_flag(cont_media_view, LV_OBJ_FLAG_HIDDEN);
                break;
            default:
                lv_obj_clear_flag(cont_default_view, LV_OBJ_FLAG_HIDDEN);
                break;
        }
        last_rendered_layer = highest_layer;
    }

#ifdef RGB_MATRIX_ENABLE
    if (layer_changed || layer_color_changed) {
        update_layer_display(&dashboard_data);
    }
#else
    if (layer_changed) {
        update_layer_display(&dashboard_data);
    }
#endif

    // 3. Show active modifiers in press order, then hold the completed chord
    update_modifier_sequence();
#if LCD_CHORD_HISTORY_COUNT > 0
    update_chord_history(false);
#endif
#if LCD_CHORD_DISPLAY_TIMEOUT > 0
    if (chord_complete && timer_elapsed32(chord_display_completed_at) >= LCD_CHORD_DISPLAY_TIMEOUT) {
        chord_complete = false;
        chord_display_timed_out = true;
    }
#endif
    if (chord_display_timed_out) {
        show_dashboard();
    } else if (!chord_complete) {
        if (modifier_count > 0) {
            show_chord(NULL);
        } else {
            show_dashboard();
        }
    }

    // 4. Update the actual data readouts inside the unhidden container
    if (!lv_obj_has_flag(cont_default_view, LV_OBJ_FLAG_HIDDEN)) {
#ifdef WPM_ENABLE
        uint8_t  current_wpm    = dashboard_data.wpm;
        uint16_t wpm_percentage = ((uint16_t)current_wpm * 100) / 120;
        update_numeric_display(label_wpm_value, bar_wpm, current_wpm, wpm_percentage > 100 ? 100 : wpm_percentage, true,
                               &wpm_display_cache);
#endif
    } else if (!lv_obj_has_flag(cont_pointer_view, LV_OBJ_FLAG_HIDDEN)) {
        update_numeric_display(label_dpi_val, bar_dpi, dashboard_data.default_dpi,
                               value_percentage(dashboard_data.default_dpi, dashboard_data.minimum_default_dpi, dashboard_data.maximum_default_dpi), true,
                               &dpi_display_cache);

        update_numeric_display(label_snipe_val, bar_snipe, dashboard_data.sniping_dpi,
                               value_percentage(dashboard_data.sniping_dpi, dashboard_data.minimum_sniping_dpi, dashboard_data.maximum_sniping_dpi), true,
                               &snipe_display_cache);
    } else if (!lv_obj_has_flag(cont_media_view, LV_OBJ_FLAG_HIDDEN)) {
        // Using standard QMK core API — works perfectly on the left side
        uint8_t native_lcd_val = dashboard_data.lcd_brightness;

#ifndef BACKLIGHT_LEVELS
#    define BACKLIGHT_LEVELS 32
#endif

        uint16_t lcd_percentage = ((uint16_t)native_lcd_val * 100U) / BACKLIGHT_LEVELS;
        update_numeric_display(label_lcd_val, bar_lcd, native_lcd_val, lcd_percentage, true, &lcd_display_cache);

#ifdef RGB_MATRIX_ENABLE
        bool     rgb_enabled = dashboard_data.rgb_enabled;
        uint16_t rgb_value   = dashboard_data.rgb_brightness;
        uint16_t rgb_percentage =
            rgb_enabled ? value_percentage(rgb_value, 0, RGB_MATRIX_MAXIMUM_BRIGHTNESS) : 0;
        update_numeric_display(label_rgb_val, bar_rgb, rgb_value, rgb_percentage, rgb_enabled, &rgb_display_cache);
#else
        update_numeric_display(label_rgb_val, bar_rgb, 0, 0, false, &rgb_display_cache);
#endif
    }
}

void set_current_module(uint8_t module_index) {
    // Stub: Currently does nothing
}

// Global Export Structure
lcd_module_t lcd_module_dashboard = {
    .init_module                                      = &init_custom_dashboard,
    .load_custom_theme_elements                       = NULL,
    .load_module                                      = &load_custom_dashboard,
    .update_custom_elements_styles_from_current_theme = NULL,
    .process_record                                   = NULL,
    .housekeeping_task                                = &housekeeping_custom_dashboard,
};
