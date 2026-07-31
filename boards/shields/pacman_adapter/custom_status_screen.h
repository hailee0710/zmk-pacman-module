/*
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 *
 * Custom Status Screen interface
 */

#pragma once

#include <zephyr/device.h>

/* zmk_display_status_screen() (declared by ZMK core, defined in
 * custom_status_screen.c) is the actual entry point ZMK calls once LVGL
 * and the display device are ready. */

const struct device *custom_status_screen_get_display(void);
