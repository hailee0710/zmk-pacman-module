/*
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 *
 * Action button widget
 */

#pragma once
#include <zephyr/device.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint8_t action;
    const char *label;
    bool pressed;
} action_button_t;

void action_button_init(action_button_t *st);
void action_button_set(action_button_t *st, uint8_t action, const char *label);
void action_button_press(action_button_t *st);
void action_button_release(action_button_t *st);
void action_button_draw(const struct device *dev, action_button_t *st,
                        uint16_t x, uint16_t y, uint16_t w, uint16_t h);
