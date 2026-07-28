/*
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 *
 * Theme support for Pacman dongle
 */

#pragma once

#include <stdint.h>

#define THEME_PACMAN 0
#define THEME_CLASSIC 1

typedef struct {
    uint8_t current_theme;
    uint16_t bg_color;
    uint16_t fg_color;
    uint16_t accent_color;
} theme_state_t;

void theme_init(theme_state_t *st, uint8_t theme);
void theme_set(theme_state_t *st, uint8_t theme);
uint8_t theme_get(theme_state_t *st);
const char *theme_name(theme_state_t *st);
