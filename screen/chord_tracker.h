#pragma once

#include QMK_KEYBOARD_H

#define CHORD_TRACKER_TEXT_SIZE 256

void chord_tracker_init(void);
void chord_tracker_process_keycode(uint16_t keycode, keyrecord_t *record, uint8_t mods, bool caps_lock,
                                   const char *layer_name);
void chord_tracker_housekeeping(uint8_t mods, const char *layer_name);
bool chord_tracker_take_overlay_update(const char **text, bool *visible, bool *has_layer_context);
bool chord_tracker_take_history_update(const char **text, bool *visible);
