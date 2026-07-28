/*
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 *
 * ST7789P3 Display Driver interface
 * 320x172 landscape orientation
 */

#pragma once

#include <zephyr/device.h>

/* Display dimensions - landscape */
#define ST7789_WIDTH  320
#define ST7789_HEIGHT 172

/* Initialize the ST7789P3 display */
int display_st7789p3_init(const struct device *dev);
