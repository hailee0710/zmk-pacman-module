/*
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 *
 * Custom Status Screen interface
 */

#pragma once

#include <zephyr/device.h>

int custom_status_screen_init(void);
void custom_status_screen_redraw(void);
const struct device *custom_status_screen_get_display(void);
