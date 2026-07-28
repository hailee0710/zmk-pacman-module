/*
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 *
 * Modifier widget implementation
 */

#include "modifier.h"

void modifier_init(modifier_t *st) {
    st->shift = false;
    st->ctrl = false;
    st->alt = false;
    st->gui = false;
}

void modifier_set(modifier_t *st, uint8_t mods) {
    st->shift = (mods & 0x01);
    st->ctrl  = (mods & 0x02);
    st->alt   = (mods & 0x04);
    st->gui   = (mods & 0x08);
}

bool modifier_any_active(modifier_t *st) {
    return st->shift || st->ctrl || st->alt || st->gui;
}
