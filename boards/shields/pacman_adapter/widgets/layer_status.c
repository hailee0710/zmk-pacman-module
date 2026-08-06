/*
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 *
 * Layer status implementation
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <zephyr/kernel.h>

#include <zmk/keymap.h>

#include "layer_status.h"

void layer_status_init(layer_status_t *st) {
    st->active_layer = UINT8_MAX;
    st->layer_name[0] = '\0';
    st->changed = false;
}

void layer_state_update(layer_status_t *st, uint8_t layer) {
    if (layer != st->active_layer) {
        st->active_layer = layer;
        st->changed = true;
        const char *name = zmk_keymap_layer_name(layer);
        if (name && name[0]) {
            strncpy(st->layer_name, name, sizeof(st->layer_name) - 1);
            st->layer_name[sizeof(st->layer_name) - 1] = '\0';
        } else {
            snprintf(st->layer_name, sizeof(st->layer_name), "L%u", layer);
        }
    }
}

uint8_t layer_get_active(layer_status_t *st) {
    return st->active_layer;
}

const char *layer_get_name(layer_status_t *st) {
    return st->layer_name;
}
