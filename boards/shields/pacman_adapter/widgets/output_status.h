/*
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 *
 * Output/connection status widget
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    bool ble_connected;
    bool usb_connected;
    uint8_t active_profile;
} output_status_t;

void output_status_init(output_status_t *st);
void output_state_update(output_status_t *st);
bool output_is_ble_connected(output_status_t *st);
bool output_is_usb_connected(output_status_t *st);
