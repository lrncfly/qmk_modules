#include QMK_KEYBOARD_H
#include "screen.h"
#include "screen_data.h"
#include "chord_tracker.h"
#include "lvgl.h"
#include "qp.h"
#include "color.h"
#include "ui_elements.h"
#include <string.h>

// Master Screen Slate
static lv_obj_t *ui_screen;
static lv_obj_t *main_cont;
static lv_obj_t *chord_cont;
static lv_obj_t *chord_layer_label;
static lv_obj_t *chord_label;

// 1. Persistent Header Containers & Widgets
static lv_obj_t *label_status_tag;

static char displayed_chord[CHORD_TRACKER_TEXT_SIZE];

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
static void render_chord_tracker_updates(void);

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

static uint8_t get_active_mods(void) {
    return get_mods() | get_weak_mods() | get_oneshot_mods();
}

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
    displayed_chord[0] = '\0';
    chord_tracker_init();
    render_chord_tracker_updates();
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

static void show_chord(const char *text) {
    if (strcmp(displayed_chord, text) != 0) {
        lv_label_set_text(chord_label, text);
        snprintf(displayed_chord, sizeof(displayed_chord), "%s", text);
    }
    lv_obj_add_flag(main_cont, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(chord_cont, LV_OBJ_FLAG_HIDDEN);
}

static void show_dashboard(void) {
    lv_obj_clear_flag(main_cont, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(chord_layer_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(chord_cont, LV_OBJ_FLAG_HIDDEN);
    displayed_chord[0] = '\0';
}

static void render_chord_tracker_updates(void) {
    const char *text;
    bool visible;
    bool has_layer_context;
    if (chord_tracker_take_overlay_update(&text, &visible, &has_layer_context)) {
        if (visible) {
            show_chord(text);
            if (has_layer_context) {
                lv_obj_add_flag(chord_layer_label, LV_OBJ_FLAG_HIDDEN);
            } else {
                lv_obj_clear_flag(chord_layer_label, LV_OBJ_FLAG_HIDDEN);
            }
        } else {
            show_dashboard();
        }
    }
#if LCD_CHORD_HISTORY_COUNT > 0
    if (chord_tracker_take_history_update(&text, &visible)) {
        lv_obj_t *history_widgets[] = {label_chord_history_title, label_chord_history};
        if (visible) {
            lv_label_set_text(label_chord_history, text);
            lv_obj_clear_flag(history_widgets[0], LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(history_widgets[1], LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_label_set_text(label_chord_history, "");
            lv_obj_add_flag(history_widgets[0], LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(history_widgets[1], LV_OBJ_FLAG_HIDDEN);
        }
    }
#endif
}

void screen_process_keycode(uint16_t keycode, keyrecord_t *record) {
    if (!is_keyboard_left()) return;

    chord_tracker_process_keycode(keycode, record, get_active_mods(),
                                  host_keyboard_led_state().caps_lock, lrncfly_screen_get_chord_layer_name());
    render_chord_tracker_updates();
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
    chord_tracker_housekeeping(get_active_mods(), dashboard_data.chord_layer_name);
    render_chord_tracker_updates();
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

    // 3. Update the actual data readouts inside the unhidden container
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
