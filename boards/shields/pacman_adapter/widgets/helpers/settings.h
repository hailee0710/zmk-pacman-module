/*
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 *
 * Settings storage helper
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

void settings_init(void);
bool settings_load(void);
bool settings_save(void);

/* Theme settings */
void settings_set_theme(uint8_t theme);
uint8_t settings_get_theme(void);

/* Game settings */
void settings_set_high_score(uint16_t score);
uint16_t settings_get_high_score(void);

/* Display settings */
void settings_set_brightness(uint8_t brightness);
uint8_t settings_get_brightness(void);
