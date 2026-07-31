/*
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 *
 * Output status implementation
 */

#include <zmk/ble.h>
#include <zmk/endpoints.h>

#include "output_status.h"

void output_status_init(output_status_t *st) {
    st->ble_connected = false;
    st->usb_connected = false;
    st->active_profile = 0;
}

void output_state_update(output_status_t *st) {
    struct zmk_endpoint_instance selected = zmk_endpoints_selected();

    st->usb_connected = (selected.transport == ZMK_TRANSPORT_USB);
    st->ble_connected = (selected.transport == ZMK_TRANSPORT_BLE) &&
                         zmk_ble_active_profile_is_connected();
    st->active_profile = zmk_ble_active_profile_index();
}

bool output_is_ble_connected(output_status_t *st) {
    return st->ble_connected;
}

bool output_is_usb_connected(output_status_t *st) {
    return st->usb_connected;
}
