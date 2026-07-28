/*
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 *
 * Configuration widget
 */

#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    bool dirty;
} configuration_t;

void configuration_init(configuration_t *st);
void configuration_set_dirty(configuration_t *st, bool dirty);
bool configuration_is_dirty(configuration_t *st);
