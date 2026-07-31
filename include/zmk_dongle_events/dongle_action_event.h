/*
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zmk/event_manager.h>

/* Dongle action IDs — passed via binding->param1 in devicetree */
enum {
    DONGLE_ACTION_PACMAN_UP    = 0,
    DONGLE_ACTION_PACMAN_DOWN  = 1,
    DONGLE_ACTION_PACMAN_LEFT  = 2,
    DONGLE_ACTION_PACMAN_RIGHT = 3,
    DONGLE_ACTION_PACMAN_START = 4,
    DONGLE_ACTION_PACMAN_PAUSE = 5,
    DONGLE_ACTION_PACMAN_QUIT  = 6,
};

struct zmk_dongle_action_event {
    uint8_t action;
};

ZMK_EVENT_DECLARE(zmk_dongle_action_event);
