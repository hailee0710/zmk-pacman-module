/*
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 *
 * Output status implementation
 */

#include "output_status.h"

void output_status_init(output_status_t *st) {
    st->ble_connected = false;
    st->usb_connected = false;
    st->active_profile = 0;
}

void output_state_update(output_status_t *st) {
    /* Simplified - actual state comes from ZMK events */
}

bool output_is_ble_connected(output_status_t *st) {
    return st->ble_connected;
}

bool output_is_usb_connected(output_status_t *st) {
    return st->usb_connected;
}
