#pragma once

#include QMK_KEYBOARD_H

typedef enum {
    SCREEN_DASHBOARD_DEFAULT,
    SCREEN_DASHBOARD_POINTER,
    SCREEN_DASHBOARD_MEDIA,
} screen_dashboard_view_t;

typedef struct {
    uint8_t layer;
    screen_dashboard_view_t view;
    const char *layer_name;
    const char *chord_layer_name;
    const char *status_text;
    RGB layer_rgb;
    uint8_t wpm;
    uint16_t default_dpi;
    uint16_t minimum_default_dpi;
    uint16_t maximum_default_dpi;
    uint16_t sniping_dpi;
    uint16_t minimum_sniping_dpi;
    uint16_t maximum_sniping_dpi;
    uint8_t lcd_brightness;
    bool rgb_enabled;
    uint8_t rgb_brightness;
} screen_dashboard_data_t;

void lrncfly_screen_get_dashboard_data(screen_dashboard_data_t *data);
const char *lrncfly_screen_get_chord_layer_name(void);
