/*
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 *
 * Action button widget implementation
 */

#include <string.h>
#include <zephyr/device.h>
#include "action_button.h"
#include "helpers/display.h"

void action_button_init(action_button_t *st) {
    st->action = 0;
    st->label = "";
    st->pressed = false;
}

void action_button_set(action_button_t *st, uint8_t action, const char *label) {
    st->action = action;
    st->label = label;
}

void action_button_press(action_button_t *st) {
    st->pressed = true;
}

void action_button_release(action_button_t *st) {
    st->pressed = false;
}

void action_button_draw(const struct device *dev, action_button_t *st,
                        uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    uint16_t color = st->pressed ? COLOR_PACMAN_YELLOW : COLOR_DARK_GRAY;
    display_fill_rect(dev, x, y, x + w - 1, y + h - 1, color);
    display_write_text(dev, x + 4, y + 2, st->label, COLOR_WHITE, color, 1);
}
