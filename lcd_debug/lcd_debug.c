#include QMK_KEYBOARD_H
#include "color.h"
#include "lcd_debug.h"
#include "qp.h"

static painter_device_t lcd;

void init_lcd_debug(void) {
    wait_ms(LCD_WAIT_TIME);

    lcd = qp_st7789_make_spi_device(LCD_WIDTH, LCD_HEIGHT, LCD_CS_PIN, LCD_DC_PIN, LCD_RST_PIN, LCD_SPI_DIVISOR, SPI_MODE);
    qp_init(lcd, LCD_ROTATION);
    qp_set_viewport_offsets(lcd, LCD_OFFSET_X, LCD_OFFSET_Y);
    qp_power(lcd, 1);

    qp_rect(lcd, 0, 0, 300, 300, HSV_GREEN, true);
    qp_flush(lcd);
}
