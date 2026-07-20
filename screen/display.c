#include QMK_KEYBOARD_H
#include "screen.h"
#include "dilemma_sync.h"
#include "layers.h"
#include "lvgl.h"
#include "qp.h"
#include "ui_elements.h"

// Master Screen Slate
static lv_obj_t *ui_screen;

// 1. Persistent Header Containers & Widgets
static lv_obj_t *label_status_tag;
static lv_obj_t *badge_shift;
static lv_obj_t *badge_alt;
static lv_obj_t *badge_ctrl;
static lv_obj_t *badge_gui;

// 2. Contextual Dynamic View Containers
static lv_obj_t *cont_default_view;
static lv_obj_t *cont_pointer_view;
static lv_obj_t *cont_media_view;

// 3. Dynamic Widget Value Trackers
static lv_obj_t *label_wpm_value;
static lv_obj_t *bar_wpm;

static lv_obj_t *bar_dpi;
static lv_obj_t *label_dpi_val;
static lv_obj_t *bar_snipe;
static lv_obj_t *label_snipe_val;

static lv_obj_t *bar_lcd;
static lv_obj_t *label_lcd_val;
static lv_obj_t *bar_rgb;
static lv_obj_t *label_rgb_val;

// Helper to safely fetch dilemma state (borrowed conceptually from base screen logic)
extern dilemma_status_t get_dilemma_status(void);

painter_device_t lcd; // Global pointer for the driver

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

    // 2. Load the general formatting themes
    load_themes();
    init_styles();

    // 3. NOW it is 100% safe to build your layout objects!
    ui_screen = lv_obj_create(NULL);

    // Create the master base column wrapper
    lv_obj_t *main_cont = ui_create_container(ui_screen);

    // ==========================================
    // PERSISTENT ZONE (Always visible at top)
    // ==========================================
    // Header Zone: Status Indicator (Primary/Secondary)
    label_status_tag = ui_create_layer_label(main_cont);
    lv_label_set_text(label_status_tag, is_keyboard_master() ? "MASTER" : "SLAVE");

    // Horizontal modifier block
    badge_shift = ui_create_mod_button(main_cont, "SHFT", true, MOD_MASK_SHIFT);
    badge_alt   = ui_create_mod_button(main_cont, "ALT", false, MOD_MASK_ALT);
    badge_ctrl  = ui_create_mod_button(main_cont, "CTRL", false, MOD_MASK_CTRL);
    badge_gui   = ui_create_mod_button(main_cont, "GUI", false, MOD_MASK_GUI);

    ui_create_line_separator(main_cont, 1, 3);

    // ==========================================
    // VIEW A: DEFAULT VIEW (WPM Meter)
    // ==========================================
    cont_default_view = ui_create_container(main_cont);
    ui_create_secondary_text(cont_default_view, "WPM", true, 1);
    label_wpm_value = ui_create_number_label(cont_default_view, 2);
    lv_label_set_text(label_wpm_value, "0");
    bar_wpm = ui_create_progress_bar(cont_default_view, 4);
    lv_bar_set_value(bar_wpm, 0, LV_ANIM_OFF);

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

    // Default visibility settings at startup
    lv_obj_clear_flag(cont_default_view, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(cont_pointer_view, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(cont_media_view, LV_OBJ_FLAG_HIDDEN);
}

void load_custom_dashboard(void) {
    lv_scr_load(ui_screen);
}

static void update_transient_mod(lv_obj_t *obj, uint8_t mod_mask, uint8_t current_mods) {
    if (!obj) return;
    if (current_mods & mod_mask) {
        lv_event_send(obj, LV_EVENT_PRESSED, NULL);
    } else {
        lv_event_send(obj, LV_EVENT_RELEASED, NULL);
    }
}

// --- Dynamic Rendering & Visibility Loop ---
void housekeeping_custom_dashboard(void) {
    if (!is_keyboard_left()) return;

    // 1. Resolve active hardware status variables from QMK core layer
    dilemma_status_t status        = get_dilemma_status();
    uint8_t          highest_layer = get_highest_layer(layer_state);

    // 2. Run Context Visibility Toggling & Layer Name Updates
    static uint8_t last_rendered_layer = 255;
    if (highest_layer != last_rendered_layer) {
        // --- Dynamic Layer Name Override ---
        switch (highest_layer) {
            case LAYER_BASE:
                lv_label_set_text(label_status_tag, is_keyboard_master() ? "MASTER" : "SLAVE");
                break;
            case LAYER_FUNCTION:
                lv_label_set_text(label_status_tag, "FUNCTION");
                break;
            case LAYER_NAVIGATION:
                lv_label_set_text(label_status_tag, "NAVIGATE");
                break;
            case LAYER_MEDIA:
                lv_label_set_text(label_status_tag, "MEDIA");
                break;
            case LAYER_POINTER:
                lv_label_set_text(label_status_tag, "POINTER");
                break;
            case LAYER_NUMERAL:
                lv_label_set_text(label_status_tag, "NUMBERS");
                break;
            case LAYER_SYMBOLS:
                lv_label_set_text(label_status_tag, "SYMBOLS");
                break;
            case LAYER_LCD:
                lv_label_set_text(label_status_tag, "LCD CNFG");
                break;
            default:
                lv_label_set_text(label_status_tag, "UNKNOWN");
                break;
        }

        // --- Container Visibility Toggling ---
        // Enforce total layout blackout
        lv_obj_add_flag(cont_default_view, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(cont_pointer_view, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(cont_media_view, LV_OBJ_FLAG_HIDDEN);

        // Selectively awake the target context container
        switch (highest_layer) {
            case LAYER_NAVIGATION:
            case LAYER_POINTER:
                lv_obj_clear_flag(cont_pointer_view, LV_OBJ_FLAG_HIDDEN);
                break;
            case LAYER_MEDIA:
            case LAYER_LCD:
                lv_obj_clear_flag(cont_media_view, LV_OBJ_FLAG_HIDDEN);
                break;
            default:
                lv_obj_clear_flag(cont_default_view, LV_OBJ_FLAG_HIDDEN);
                break;
        }
        last_rendered_layer = highest_layer;
    }

    // 3. Keep Modifiers fully tracking continuously across all views
    uint8_t live_mods = get_mods() | get_oneshot_mods();
    update_transient_mod(badge_shift, MOD_MASK_SHIFT, live_mods);
    update_transient_mod(badge_alt, MOD_MASK_ALT, live_mods);
    update_transient_mod(badge_ctrl, MOD_MASK_CTRL, live_mods);
    update_transient_mod(badge_gui, MOD_MASK_GUI, live_mods);

    // 4. Update the actual data readouts inside the unhidden container
    if (!lv_obj_has_flag(cont_default_view, LV_OBJ_FLAG_HIDDEN)) {
#ifdef WPM_ENABLE
        uint8_t current_wpm = get_current_wpm();
        char    wpm_str[12];
        snprintf(wpm_str, sizeof(wpm_str), "%u", current_wpm);
        lv_label_set_text(label_wpm_value, wpm_str);
        uint16_t wpm_percentage = ((uint16_t)current_wpm * 100) / 120;
        lv_bar_set_value(bar_wpm, wpm_percentage > 100 ? 100 : wpm_percentage, LV_ANIM_OFF);
#endif
    } else if (!lv_obj_has_flag(cont_pointer_view, LV_OBJ_FLAG_HIDDEN)) {
        char val_str[12];

        // Trackpad Main DPI
        snprintf(val_str, sizeof(val_str), "%u", status.dpi);
        lv_label_set_text(label_dpi_val, val_str);
        float dpi_rel = (float)((status.dpi + 200 - 400)) * 100 / (200 * 16);
        lv_bar_set_value(bar_dpi, (uint16_t)dpi_rel, LV_ANIM_OFF);

        // Sniper Mode DPI
        snprintf(val_str, sizeof(val_str), "%u", status.s_dpi);
        lv_label_set_text(label_snipe_val, val_str);
        float snipe_rel = (float)((status.s_dpi + 100 - 200)) * 100 / (100 * 4);
        lv_bar_set_value(bar_snipe, (uint16_t)snipe_rel, LV_ANIM_OFF);
    } else if (!lv_obj_has_flag(cont_media_view, LV_OBJ_FLAG_HIDDEN)) {
        char val_str[12];

        // Using standard QMK core API — works perfectly on the left side
        uint8_t native_lcd_val = get_backlight_level();

        snprintf(val_str, sizeof(val_str), "%u", native_lcd_val);
        lv_label_set_text(label_lcd_val, val_str);

#ifndef BACKLIGHT_LEVELS
#    define BACKLIGHT_LEVELS 32
#endif

        float lcd_rel = (float)(native_lcd_val) * 100 / BACKLIGHT_LEVELS;
        lv_bar_set_value(bar_lcd, (uint16_t)lcd_rel, LV_ANIM_OFF);
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
