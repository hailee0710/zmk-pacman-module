/*
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 *
 * Battery status widget
 */

#pragma once

#include <stdint.h>

typedef struct {
    uint8_t level_pct[2];  /* [0]=left, [1]=right */
    bool charging[2];
} battery_status_t;

void battery_status_init(battery_status_t *st);
void battery_state_update(battery_status_t *st);
uint8_t battery_get_level(battery_status_t *st, uint8_t index);
bool battery_is_charging(battery_status_t *st, uint8_t index);
