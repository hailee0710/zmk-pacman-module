/*
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zmk/events/event.h>

struct zmk_dongle_action_event {
    struct zmk_event header;
    uint8_t action;
};

ZMK_EVENT_DECLARE(zmk_dongle_action_event);
