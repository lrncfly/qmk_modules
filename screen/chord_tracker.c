#include QMK_KEYBOARD_H
#include "chord_tracker.h"
#include "keycode_string.h"
#include <stdio.h>
#include <string.h>

#ifndef LCD_CHORD_HISTORY_COUNT
#    define LCD_CHORD_HISTORY_COUNT 3
#endif

#ifndef LCD_CHORD_HISTORY_TIMEOUT
#    define LCD_CHORD_HISTORY_TIMEOUT 20000
#endif

#ifndef LCD_CHORD_DISPLAY_TIMEOUT
#    define LCD_CHORD_DISPLAY_TIMEOUT 5000
#endif

#define CHORD_COMPONENT_COUNT 4
#define CHORD_LAYER_NAME_SIZE 32
#define CHORD_HISTORY_ENTRY_SIZE CHORD_TRACKER_TEXT_SIZE

typedef struct {
    uint16_t event_time;
    uint8_t row;
    uint8_t col;
    uint16_t keycode;
    bool valid;
} key_event_t;

static const uint8_t modifier_masks[CHORD_COMPONENT_COUNT] = {
    MOD_MASK_CTRL,
    MOD_MASK_SHIFT,
    MOD_MASK_ALT,
    MOD_MASK_GUI,
};

static const char *const modifier_names[CHORD_COMPONENT_COUNT] = {
    "CTRL",
    "SHIFT",
    "ALT",
    "GUI",
};

static uint8_t modifier_order[CHORD_COMPONENT_COUNT];
static uint8_t modifier_count;
static uint8_t previous_mods;

static char overlay_text[CHORD_TRACKER_TEXT_SIZE];
static bool overlay_visible;
static bool overlay_completed;
static bool overlay_suppressed;
static bool overlay_has_layer_context;
static bool overlay_dirty;

static bool sequence_active;
static uint8_t sequence_mods;
static char sequence_layer[CHORD_LAYER_NAME_SIZE];
static char sequence_history[CHORD_TRACKER_TEXT_SIZE];
static char sequence_overlay[CHORD_TRACKER_TEXT_SIZE];
static uint16_t sequence_stroke_event_time;
static uint32_t sequence_last_activity;

static key_event_t last_terminal_event;

#if LCD_CHORD_HISTORY_COUNT > 0
static char chord_history[LCD_CHORD_HISTORY_COUNT][CHORD_HISTORY_ENTRY_SIZE];
static uint32_t chord_history_times[LCD_CHORD_HISTORY_COUNT];
static char history_text[LCD_CHORD_HISTORY_COUNT * CHORD_HISTORY_ENTRY_SIZE];
static uint8_t chord_history_count;
static bool history_visible;
static bool history_dirty;
#else
static char history_text[1];
#endif

static bool append_text(char *buffer, size_t buffer_size, const char *text) {
    size_t length = strlen(buffer);
    if (length >= buffer_size) {
        return false;
    }

    int written = snprintf(buffer + length, buffer_size - length, "%s", text);
    return written >= 0 && (size_t)written < buffer_size - length;
}

static bool append_component(char *buffer, size_t buffer_size, const char *component) {
    if (buffer[0] != '\0' && !append_text(buffer, buffer_size, " + ")) {
        return false;
    }
    return append_text(buffer, buffer_size, component);
}

static void update_modifier_order(uint8_t mods) {
    for (uint8_t i = 0; i < modifier_count;) {
        if (!(mods & modifier_masks[modifier_order[i]])) {
            memmove(&modifier_order[i], &modifier_order[i + 1], modifier_count - i - 1);
            modifier_count--;
        } else {
            i++;
        }
    }

    for (uint8_t i = 0; i < CHORD_COMPONENT_COUNT; i++) {
        if ((mods & modifier_masks[i]) && !(previous_mods & modifier_masks[i])) {
            modifier_order[modifier_count++] = i;
        }
    }
    previous_mods = mods;
}

static bool context_matches(uint8_t mods, const char *layer_name) {
    const char *layer = layer_name ? layer_name : "";
    return sequence_mods == mods && strcmp(sequence_layer, layer) == 0;
}

static bool has_context(uint8_t mods, const char *layer_name) {
    return mods != 0 || (layer_name && layer_name[0] != '\0');
}

static void set_overlay(const char *text, bool visible, bool completed, bool has_layer_context) {
    has_layer_context = visible && has_layer_context;
    bool changed = overlay_visible != visible || overlay_has_layer_context != has_layer_context ||
                   (visible && strcmp(overlay_text, text) != 0);
    if (!changed) {
        overlay_completed = completed;
        return;
    }

    if (visible) {
        snprintf(overlay_text, sizeof(overlay_text), "%s", text);
    } else {
        overlay_text[0] = '\0';
    }
    overlay_visible = visible;
    overlay_completed = completed;
    overlay_has_layer_context = has_layer_context;
    overlay_dirty = true;
}

static bool format_context(uint8_t mods, const char *layer_name, char *history, size_t history_size,
                           char *overlay, size_t overlay_size) {
    history[0] = '\0';
    overlay[0] = '\0';

    if (layer_name && layer_name[0] != '\0') {
        if (!append_component(history, history_size, layer_name) ||
            !append_text(overlay, overlay_size, layer_name)) {
            return false;
        }
    }

    for (uint8_t i = 0; i < modifier_count; i++) {
        const char *name = modifier_names[modifier_order[i]];
        if ((mods & modifier_masks[modifier_order[i]]) == 0) {
            continue;
        }
        if (!append_component(history, history_size, name)) {
            return false;
        }
        if (overlay[0] != '\0' && !append_text(overlay, overlay_size, "\n")) {
            return false;
        }
        if (!append_text(overlay, overlay_size, name)) {
            return false;
        }
    }
    return true;
}

static char us_layout_character(uint16_t keycode, uint8_t mods, bool caps_lock) {
    if (IS_QK_MODS(keycode)) {
        mods |= QK_MODS_GET_MODS(keycode);
        keycode = QK_MODS_GET_BASIC_KEYCODE(keycode);
    }

    bool shifted = (mods & MOD_MASK_SHIFT) != 0;
    if (keycode >= KC_A && keycode <= KC_Z) {
        char letter = (char)('a' + keycode - KC_A);
        bool uppercase = shifted ^ caps_lock;
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

static void format_terminal_key(uint16_t keycode, uint8_t mods, bool caps_lock, char *text, size_t text_size) {
    if (text_size == 0) {
        return;
    }

    char character = us_layout_character(keycode, mods, caps_lock);
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

#if LCD_CHORD_HISTORY_COUNT > 0
static void rebuild_history_text(void) {
    history_text[0] = '\0';
    size_t length = 0;
    for (uint8_t i = 0; i < chord_history_count; i++) {
        int written = snprintf(history_text + length, sizeof(history_text) - length, "%s%s",
                               i == 0 ? "" : "\n", chord_history[i]);
        if (written < 0 || (size_t)written >= sizeof(history_text) - length) {
            break;
        }
        length += (size_t)written;
    }
    history_visible = chord_history_count > 0;
    history_dirty = true;
}

static void add_history_entry(const char *text, uint32_t timestamp) {
    uint8_t entries_to_move = chord_history_count;
    if (entries_to_move >= LCD_CHORD_HISTORY_COUNT) {
        entries_to_move = LCD_CHORD_HISTORY_COUNT - 1;
    } else {
        chord_history_count++;
    }

    memmove(&chord_history[1], &chord_history[0], entries_to_move * sizeof(chord_history[0]));
    memmove(&chord_history_times[1], &chord_history_times[0], entries_to_move * sizeof(chord_history_times[0]));
    snprintf(chord_history[0], sizeof(chord_history[0]), "%s", text);
    chord_history_times[0] = timestamp;
    rebuild_history_text();
}

static void update_history_entry(const char *text, uint32_t timestamp) {
    if (chord_history_count == 0) {
        add_history_entry(text, timestamp);
        return;
    }
    snprintf(chord_history[0], sizeof(chord_history[0]), "%s", text);
    chord_history_times[0] = timestamp;
    rebuild_history_text();
}

static void expire_history(void) {
#    if LCD_CHORD_HISTORY_TIMEOUT > 0
    bool changed = false;
    for (uint8_t i = 0; i < chord_history_count;) {
        if (timer_elapsed32(chord_history_times[i]) >= LCD_CHORD_HISTORY_TIMEOUT) {
            memmove(&chord_history[i], &chord_history[i + 1],
                    (chord_history_count - i - 1) * sizeof(chord_history[0]));
            memmove(&chord_history_times[i], &chord_history_times[i + 1],
                    (chord_history_count - i - 1) * sizeof(chord_history_times[0]));
            chord_history_count--;
            changed = true;
        } else {
            i++;
        }
    }
    if (changed) {
        rebuild_history_text();
    }
#    endif
}
#endif

static bool build_stroke(uint8_t mods, const char *layer_name, const char *terminal_key,
                         char *history, size_t history_size, char *overlay, size_t overlay_size) {
    if (!format_context(mods, layer_name, history, history_size, overlay, overlay_size)) {
        return false;
    }

    if (!append_component(history, history_size, terminal_key)) {
        return false;
    }
    if (overlay[0] != '\0' && !append_text(overlay, overlay_size, "\n")) {
        return false;
    }
    return append_text(overlay, overlay_size, terminal_key);
}

static void begin_sequence(uint8_t mods, const char *layer_name, const char *stroke_history,
                           const char *stroke_overlay, uint16_t event_time, uint32_t timestamp, bool can_continue) {
    snprintf(sequence_history, sizeof(sequence_history), "%s", stroke_history);
    snprintf(sequence_overlay, sizeof(sequence_overlay), "%s", stroke_overlay);
    sequence_mods = mods;
    snprintf(sequence_layer, sizeof(sequence_layer), "%s", layer_name ? layer_name : "");
    sequence_stroke_event_time = event_time;
    sequence_last_activity = timestamp;
    sequence_active = can_continue;
    set_overlay(sequence_overlay, true, true, layer_name && layer_name[0] != '\0');
#if LCD_CHORD_HISTORY_COUNT > 0
    add_history_entry(sequence_history, timestamp);
#endif
}

static bool is_duplicate_terminal_event(const keyrecord_t *record, uint16_t keycode) {
    return last_terminal_event.valid && last_terminal_event.event_time == record->event.time &&
           last_terminal_event.row == record->event.key.row && last_terminal_event.col == record->event.key.col &&
           last_terminal_event.keycode == keycode;
}

static void remember_terminal_event(const keyrecord_t *record, uint16_t keycode) {
    last_terminal_event.event_time = record->event.time;
    last_terminal_event.row = record->event.key.row;
    last_terminal_event.col = record->event.key.col;
    last_terminal_event.keycode = keycode;
    last_terminal_event.valid = true;
}

static void record_terminal_key(uint16_t keycode, keyrecord_t *record, uint8_t mods, bool caps_lock,
                               const char *layer_name, bool standalone_dual_role_tap) {
    bool context_active = has_context(mods, layer_name);
    if (!context_active && !standalone_dual_role_tap) {
        sequence_active = false;
        set_overlay("", false, false, false);
        return;
    }

    if (is_duplicate_terminal_event(record, keycode)) {
        return;
    }
    remember_terminal_event(record, keycode);

    char terminal_key[16];
    format_terminal_key(keycode, mods, caps_lock, terminal_key, sizeof(terminal_key));

    char stroke_history[CHORD_TRACKER_TEXT_SIZE];
    char stroke_overlay[CHORD_TRACKER_TEXT_SIZE];
    if (!build_stroke(mods, layer_name, terminal_key, stroke_history, sizeof(stroke_history),
                      stroke_overlay, sizeof(stroke_overlay))) {
        return;
    }

    uint32_t timestamp = timer_read32();
    bool can_continue = context_active && sequence_active && context_matches(mods, layer_name);
#if LCD_CHORD_DISPLAY_TIMEOUT > 0
    can_continue = can_continue && timer_elapsed32(sequence_last_activity) < LCD_CHORD_DISPLAY_TIMEOUT;
#endif

    if (!can_continue) {
        begin_sequence(mods, layer_name, stroke_history, stroke_overlay, record->event.time, timestamp, context_active);
        return;
    }

    if (sequence_stroke_event_time == record->event.time) {
        char next_history[CHORD_TRACKER_TEXT_SIZE];
        char next_overlay[CHORD_TRACKER_TEXT_SIZE];
        snprintf(next_history, sizeof(next_history), "%s", sequence_history);
        snprintf(next_overlay, sizeof(next_overlay), "%s", sequence_overlay);
        if (!append_text(next_history, sizeof(next_history), " + ") ||
            !append_text(next_history, sizeof(next_history), terminal_key) ||
            !append_text(next_overlay, sizeof(next_overlay), " + ") ||
            !append_text(next_overlay, sizeof(next_overlay), terminal_key)) {
            begin_sequence(mods, layer_name, stroke_history, stroke_overlay, record->event.time, timestamp, context_active);
            return;
        }
        snprintf(sequence_history, sizeof(sequence_history), "%s", next_history);
        snprintf(sequence_overlay, sizeof(sequence_overlay), "%s", next_overlay);
    } else {
        char next_history[CHORD_TRACKER_TEXT_SIZE];
        char next_overlay[CHORD_TRACKER_TEXT_SIZE];
        snprintf(next_history, sizeof(next_history), "%s", sequence_history);
        snprintf(next_overlay, sizeof(next_overlay), "%s", sequence_overlay);
        if (!append_text(next_history, sizeof(next_history), " ") ||
            !append_text(next_history, sizeof(next_history), stroke_history) ||
            !append_text(next_overlay, sizeof(next_overlay), " / ") ||
            !append_text(next_overlay, sizeof(next_overlay), stroke_overlay)) {
            begin_sequence(mods, layer_name, stroke_history, stroke_overlay, record->event.time, timestamp, context_active);
            return;
        }
        snprintf(sequence_history, sizeof(sequence_history), "%s", next_history);
        snprintf(sequence_overlay, sizeof(sequence_overlay), "%s", next_overlay);
    }

    sequence_stroke_event_time = record->event.time;
    sequence_last_activity = timestamp;
    set_overlay(sequence_overlay, true, true, layer_name && layer_name[0] != '\0');
#if LCD_CHORD_HISTORY_COUNT > 0
    update_history_entry(sequence_history, timestamp);
#endif
}

void chord_tracker_init(void) {
    modifier_count = 0;
    previous_mods = 0;
    overlay_text[0] = '\0';
    overlay_visible = false;
    overlay_completed = false;
    overlay_suppressed = false;
    overlay_has_layer_context = false;
    overlay_dirty = true;
    sequence_active = false;
    sequence_layer[0] = '\0';
    sequence_history[0] = '\0';
    sequence_overlay[0] = '\0';
    last_terminal_event.valid = false;
#if LCD_CHORD_HISTORY_COUNT > 0
    chord_history_count = 0;
    memset(chord_history_times, 0, sizeof(chord_history_times));
    history_text[0] = '\0';
    history_visible = false;
    history_dirty = true;
#else
    history_text[0] = '\0';
#endif
}

void chord_tracker_process_keycode(uint16_t keycode, keyrecord_t *record, uint8_t mods, bool caps_lock,
                                   const char *layer_name) {
    update_modifier_order(mods);
    if (sequence_active && !context_matches(mods, layer_name)) {
        sequence_active = false;
    }

    bool standalone_dual_role_tap = false;
    if (IS_QK_MOD_TAP(keycode)) {
        if (record->event.pressed || !record->tap.count) {
            return;
        }
        keycode = QK_MOD_TAP_GET_TAP_KEYCODE(keycode);
        standalone_dual_role_tap = true;
    } else if (IS_QK_LAYER_TAP(keycode)) {
        if (record->event.pressed || !record->tap.count) {
            return;
        }
        keycode = QK_LAYER_TAP_GET_TAP_KEYCODE(keycode);
        standalone_dual_role_tap = true;
    } else if (!record->event.pressed) {
        return;
    }

    overlay_suppressed = false;
    if (keycode >= KC_LCTL && keycode <= KC_RGUI) {
        sequence_active = false;
        overlay_completed = false;
        return;
    }

    record_terminal_key(keycode, record, mods, caps_lock, layer_name, standalone_dual_role_tap);
}

void chord_tracker_housekeeping(uint8_t mods, const char *layer_name) {
    update_modifier_order(mods);
    if (sequence_active && !context_matches(mods, layer_name)) {
        sequence_active = false;
    }

    if (!overlay_completed && !overlay_suppressed) {
        if (modifier_count > 0) {
            char history[CHORD_TRACKER_TEXT_SIZE];
            char overlay[CHORD_TRACKER_TEXT_SIZE];
            if (format_context(mods, layer_name, history, sizeof(history), overlay, sizeof(overlay))) {
                set_overlay(overlay, true, false, layer_name && layer_name[0] != '\0');
            }
        } else {
            set_overlay("", false, false, false);
        }
    }

#if LCD_CHORD_DISPLAY_TIMEOUT > 0
    if (overlay_completed && timer_elapsed32(sequence_last_activity) >= LCD_CHORD_DISPLAY_TIMEOUT) {
        sequence_active = false;
        overlay_suppressed = true;
        set_overlay("", false, false, false);
    }
#endif

#if LCD_CHORD_HISTORY_COUNT > 0
    expire_history();
#endif
}

bool chord_tracker_take_overlay_update(const char **text, bool *visible, bool *has_layer_context) {
    if (!overlay_dirty) {
        return false;
    }
    overlay_dirty = false;
    *text = overlay_text;
    *visible = overlay_visible;
    *has_layer_context = overlay_has_layer_context;
    return true;
}

bool chord_tracker_take_history_update(const char **text, bool *visible) {
#if LCD_CHORD_HISTORY_COUNT > 0
    if (!history_dirty) {
        return false;
    }
    history_dirty = false;
    *text = history_text;
    *visible = history_visible;
    return true;
#else
    (void)text;
    (void)visible;
    return false;
#endif
}
