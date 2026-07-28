/*
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 *
 * Layer status widget
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint8_t active_layer;
    char layer_name[16];
    bool changed;
} layer_status_t;

void layer_status_init(layer_status_t *st);
void layer_state_update(layer_status_t *st);
uint8_t layer_get_active(layer_status_t *st);
const char *layer_get_name(layer_status_t *st);
