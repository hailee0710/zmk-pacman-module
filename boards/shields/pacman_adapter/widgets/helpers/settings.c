/*
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 *
 * Settings storage - ZMK settings subsystem integration
 */

#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/logging/log.h>

#include "settings.h"

LOG_MODULE_REGISTER(pacman_settings, CONFIG_DISPLAY_LOG_LEVEL);

/* Settings subtree paths */
#define SETTINGS_PATH_THEME     "pacman/theme"
#define SETTINGS_PATH_HIGHSCORE "pacman/highscore"
#define SETTINGS_PATH_BRIGHTNESS "pacman/brightness"

static uint8_t current_theme = 0;
static uint16_t high_score = 0;
static uint8_t brightness = 100;

/* ZMK settings handler — called on load and when a setting changes at runtime */
static int settings_handler(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg) {
    const char *next;
    
    if (settings_name_steq(name, "theme", &next) && !next) {
        if (len != sizeof(current_theme)) return -EINVAL;
        int rc = read_cb(cb_arg, &current_theme, sizeof(current_theme));
        return (rc >= 0) ? 0 : rc;
    }
    if (settings_name_steq(name, "highscore", &next) && !next) {
        if (len != sizeof(high_score)) return -EINVAL;
        int rc = read_cb(cb_arg, &high_score, sizeof(high_score));
        return (rc >= 0) ? 0 : rc;
    }
    if (settings_name_steq(name, "brightness", &next) && !next) {
        if (len != sizeof(brightness)) return -EINVAL;
        int rc = read_cb(cb_arg, &brightness, sizeof(brightness));
        return (rc >= 0) ? 0 : rc;
    }
    return -ENOENT;
}

SETTINGS_STATIC_HANDLER_DEFINE(pacman_settings, SETTINGS_PATH_THEME, NULL,
                               settings_handler, NULL, NULL);

void pacman_settings_init(void) {
    pacman_settings_load();
    LOG_INF("Pacman settings initialized (theme=%u, highscore=%u, brightness=%u)",
            current_theme, high_score, brightness);
}

bool pacman_settings_load(void) {
    int rc = settings_load_subtree(SETTINGS_PATH_THEME);
    if (rc) LOG_WRN("Failed to load settings subtree: %d", rc);
    return (rc == 0);
}

bool pacman_settings_save(void) {
    int rc;
    rc = settings_save_one(SETTINGS_PATH_THEME, &current_theme, sizeof(current_theme));
    if (rc) return false;
    rc = settings_save_one(SETTINGS_PATH_HIGHSCORE, &high_score, sizeof(high_score));
    if (rc) return false;
    rc = settings_save_one(SETTINGS_PATH_BRIGHTNESS, &brightness, sizeof(brightness));
    return (rc == 0);
}

void settings_set_theme(uint8_t theme) {
    if (current_theme != theme) {
        current_theme = theme;
        pacman_settings_save();
    }
}

uint8_t settings_get_theme(void) { return current_theme; }

void settings_set_high_score(uint16_t score) {
    if (score > high_score) {
        high_score = score;
        pacman_settings_save();
    }
}

uint16_t settings_get_high_score(void) { return high_score; }

void settings_set_brightness(uint8_t b) {
    if (brightness != b) {
        brightness = b;
        pacman_settings_save();
    }
}

uint8_t settings_get_brightness(void) { return brightness; }
