#ifndef SCREEN_H
#define SCREEN_H
#include QMK_KEYBOARD_H

void init_custom_dashboard(void);
void load_custom_dashboard(void);
void housekeeping_custom_dashboard(void);
void screen_note_activity(void);
void screen_process_keycode(uint16_t keycode, keyrecord_t *record);
#endif
