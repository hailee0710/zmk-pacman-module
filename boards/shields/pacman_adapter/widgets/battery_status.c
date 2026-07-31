/*
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 *
 * Battery status implementation
 *
 * This dongle has no keys and no battery of its own that a user cares
 * about — what belongs on screen is each keyboard half's battery, which
 * only reaches the dongle via zmk_peripheral_battery_state_changed
 * (source 0/1 = left/right), not zmk_battery_state_of_charge() (that
 * queries the dongle's own SoC and used to get reported as both halves).
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "battery_status.h"

LOG_MODULE_REGISTER(battery_status, CONFIG_DISPLAY_LOG_LEVEL);

void battery_status_init(battery_status_t *st) {
    st->level_pct[0] = 100;
    st->level_pct[1] = 100;
    st->charging[0] = false;
    st->charging[1] = false;
}

void battery_set_level(battery_status_t *st, uint8_t index, uint8_t level) {
    if (index > 1) return;
    st->level_pct[index] = level;
}

uint8_t battery_get_level(battery_status_t *st, uint8_t index) {
    if (index > 1) return 0;
    return st->level_pct[index];
}

bool battery_is_charging(battery_status_t *st, uint8_t index) {
    if (index > 1) return false;
    return st->charging[index];
}
