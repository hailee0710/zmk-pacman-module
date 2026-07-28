/*
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 *
 * Logo widget — landscape 320×172
 */

#include <zephyr/device.h>
#include "logo.h"
#include "helpers/display.h"

static bool visible = false;

void logo_init(void) { visible = false; }

void logo_draw(const struct device *dev) {
    display_fill(dev, COLOR_BLACK);
    display_write_text(dev, 70, 50, "PACMAN", COLOR_PACMAN_YELLOW, COLOR_BLACK, 2);
    display_write_text(dev, 80, 90, "DONGLE", COLOR_WHITE, COLOR_BLACK, 1);
    display_draw_filled_circle(dev, 160, 140, 24, COLOR_PACMAN_YELLOW);
    visible = true;
}

void logo_hide(void) { visible = false; }
