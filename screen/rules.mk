# Enable QMK's Native Display & Graphics Stack
QUANTUM_PAINTER_ENABLE = yes
QUANTUM_PAINTER_DRIVERS += st7789_spi
QUANTUM_PAINTER_DRIVERS += surface
QUANTUM_PAINTER_LVGL_INTEGRATION = yes
ST7789 = true


BACKLIGHT_ENABLE = yes
BACKLIGHT_DRIVER = pwm

# Add this module's directory to search paths so headers resolve everywhere
VPATH += $(CURRENT_DIR)

# Module source files
SRC += display.c
SRC += ui_elements.c
SRC += theme.c  # if ui_elements depends on theme
