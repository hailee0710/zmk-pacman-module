/*
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 *
 * Splash screen — landscape 320×172
 */

#include <zephyr/device.h>
#include "splash.h"
#include "helpers/display.h"

static bool visible;

void splash_init(void) { visible = false; }

void splash_show(const struct device *dev) {
    display_fill(COLOR_BLACK);
    display_write_text(85, 20, "ZMK",    COLOR_WHITE, COLOR_BLACK, 2);
    display_write_text(50, 55, "PACMAN", COLOR_PACMAN_YELLOW, COLOR_BLACK, 2);
    display_write_text(50, 90, "DONGLE", COLOR_WHITE, COLOR_BLACK, 1);
    display_draw_pacman(160, 145, 30, DIR_RIGHT, 2, COLOR_PACMAN_YELLOW);
    display_draw_dot(210, 145, 3, COLOR_DOT_WHITE);
    display_draw_dot(235, 145, 3, COLOR_DOT_WHITE);
    display_draw_dot(260, 145, 3, COLOR_DOT_WHITE);
    display_draw_dot(285, 145, 3, COLOR_DOT_WHITE);
    display_write_text(130, 165, "v1.0", COLOR_GRAY, COLOR_BLACK, 1);
    display_flush();
    visible = true;
}

void splash_hide(void) { visible = false; }
bool splash_is_visible(void) { return visible; }
