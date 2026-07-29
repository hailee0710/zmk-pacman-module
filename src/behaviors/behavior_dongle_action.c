/*
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zmk/behavior.h>

#include <zmk_dongle_events/dongle_action_event.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static int behavior_dongle_action_init(const struct device *dev) {
    return 0;
}

static int behavior_dongle_action_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                                         struct zmk_behavior_binding_event event) {
    LOG_DBG("Dongle action pressed: action=%d", binding->param1);

    struct zmk_dongle_action_event *action_event =
        new_zmk_dongle_action_event();
    action_event->action = binding->param1;
    ZMK_EVENT_RAISE(action_event);

    return ZMK_BEHAVIOR_OPAQUE;
}

static int behavior_dongle_action_keymap_binding_released(struct zmk_behavior_binding *binding,
                                                          struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_dongle_action_driver_api = {
    .binding_pressed = behavior_dongle_action_keymap_binding_pressed,
    .binding_released = behavior_dongle_action_keymap_binding_released,
};

#define DT_DRV_COMPAT zmk_behavior_dongle_action

#define DONGLE_ACTION_INST(n)                                                                      \
    BEHAVIOR_DT_INST_DEFINE(n, behavior_dongle_action_init, NULL, NULL, NULL, POST_KERNEL,         \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                                   \
                            &behavior_dongle_action_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DONGLE_ACTION_INST)
