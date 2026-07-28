/*
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 *
 * Modifier key status widget
 */

#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    bool shift;
    bool ctrl;
    bool alt;
    bool gui;
} modifier_t;

void modifier_init(modifier_t *st);
void modifier_set(modifier_t *st, uint8_t mods);
bool modifier_any_active(modifier_t *st);
