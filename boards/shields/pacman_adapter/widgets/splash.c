/*
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 *
 * Splash screen — landscape 320×172
 */

#include <zephyr/device.h>
#include "splash.h"
#include "helpers/display.h"

static bool visible = false;

void splash_init(void) { visible = false; }

void splash_show(const struct device *dev) {
    display_fill(dev, COLOR_BLACK);

    /* Title centered */
    display_write_text(dev, 85, 20, "ZMK",    COLOR_WHITE, COLOR_BLACK, 2);
    display_write_text(dev, 50, 55, "PACMAN", COLOR_PACMAN_YELLOW, COLOR_BLACK, 2);
    display_write_text(dev, 50, 90, "DONGLE", COLOR_WHITE, COLOR_BLACK, 1);

    /* Big Pacman centered */
    display_draw_pacman(dev, 160, 145, 30, DIR_RIGHT, 2, COLOR_PACMAN_YELLOW);

    /* Dots to the right */
    display_draw_dot(dev, 210, 145, 3, COLOR_DOT_WHITE);
    display_draw_dot(dev, 235, 145, 3, COLOR_DOT_WHITE);
    display_draw_dot(dev, 260, 145, 3, COLOR_DOT_WHITE);
    display_draw_dot(dev, 285, 145, 3, COLOR_DOT_WHITE);

    display_write_text(dev, 130, 165, "v1.0", COLOR_GRAY, COLOR_BLACK, 1);
    visible = true;
}

void splash_hide(void) { visible = false; }
bool splash_is_visible(void) { return visible; }
