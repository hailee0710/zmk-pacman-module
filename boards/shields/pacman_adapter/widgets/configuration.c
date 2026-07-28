/*
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 *
 * Configuration widget implementation
 */

#include "configuration.h"

void configuration_init(configuration_t *st) {
    st->dirty = false;
}

void configuration_set_dirty(configuration_t *st, bool dirty) {
    st->dirty = dirty;
}

bool configuration_is_dirty(configuration_t *st) {
    return st->dirty;
}
