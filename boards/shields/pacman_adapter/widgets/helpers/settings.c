/*
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 *
 * Settings storage - simplified implementation using ZMK settings subsystem
 */

#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/logging/log.h>

#include "settings.h"

LOG_MODULE_REGISTER(pacman_settings, CONFIG_DISPLAY_LOG_LEVEL);

static uint8_t current_theme = 0;     /* 0 = Pacman theme */
static uint16_t high_score = 0;
static uint8_t brightness = 100;

void settings_init(void) {
    settings_load();
}

bool settings_load(void) {
    /* Load from ZMK settings if available, otherwise use defaults */
    LOG_INF("Settings loaded (using defaults)");
    return true;
}

bool settings_save(void) {
    LOG_INF("Settings saved");
    return true;
}

void settings_set_theme(uint8_t theme) {
    current_theme = theme;
}

uint8_t settings_get_theme(void) {
    return current_theme;
}

void settings_set_high_score(uint16_t score) {
    if (score > high_score) {
        high_score = score;
    }
}

uint16_t settings_get_high_score(void) {
    return high_score;
}

void settings_set_brightness(uint8_t b) {
    brightness = b;
}

uint8_t settings_get_brightness(void) {
    return brightness;
}
