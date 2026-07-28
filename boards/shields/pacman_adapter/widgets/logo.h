/*
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 *
 * Logo widget - displays boot logo
 */

#pragma once
#include <zephyr/device.h>

void logo_init(void);
void logo_draw(const struct device *dev);
void logo_hide(void);
