/*
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 *
 * Battery status implementation
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zmk/battery.h>

#include "battery_status.h"

LOG_MODULE_REGISTER(battery_status, CONFIG_DISPLAY_LOG_LEVEL);

void battery_status_init(battery_status_t *st) {
    st->level_pct[0] = 100;
    st->level_pct[1] = 100;
    st->charging[0] = false;
    st->charging[1] = false;
}

void battery_state_update(battery_status_t *st) {
    /* Get battery level from ZMK */
    uint8_t level = zmk_battery_state_of_charge();

    /* Simplified: use same level for both peripherals */
    st->level_pct[0] = level;
    st->level_pct[1] = level;
}

uint8_t battery_get_level(battery_status_t *st, uint8_t index) {
    if (index > 1) return 0;
    return st->level_pct[index];
}

bool battery_is_charging(battery_status_t *st, uint8_t index) {
    if (index > 1) return false;
    return st->charging[index];
}
