/*
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 *
 * Theme implementation
 */

#include "theme.h"
#include "helpers/display.h"

void theme_init(theme_state_t *st, uint8_t theme) {
    st->current_theme = theme;
    st->bg_color = COLOR_BLACK;
    st->fg_color = COLOR_WHITE;
    st->accent_color = COLOR_PACMAN_YELLOW;
}

void theme_set(theme_state_t *st, uint8_t theme) {
    st->current_theme = theme;
}

uint8_t theme_get(theme_state_t *st) {
    return st->current_theme;
}

const char *theme_name(theme_state_t *st) {
    return (st->current_theme == THEME_PACMAN) ? "PACMAN" : "CLASSIC";
}
