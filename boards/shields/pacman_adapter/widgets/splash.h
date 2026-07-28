/*
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 *
 * Splash screen widget
 */

#pragma once
#include <zephyr/device.h>

void splash_init(void);
void splash_show(const struct device *dev);
void splash_hide(void);
bool splash_is_visible(void);
