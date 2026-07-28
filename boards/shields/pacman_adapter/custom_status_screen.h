/*
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 *
 * Custom Status Screen - Main orchestrator for the Pacman dongle display
 */

#pragma once

#include <zephyr/device.h>

/* Initialize the custom status screen subsystem */
int custom_status_screen_init(void);

/* Called periodically to update animations and refresh display */
void custom_status_screen_tick(void);

/* Force a full redraw */
void custom_status_screen_redraw(void);

/* Get the display device */
const struct device *custom_status_screen_get_display(void);
