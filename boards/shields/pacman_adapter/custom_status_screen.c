/*
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 *
 * Custom Status Screen - Main orchestrator
 *
 * Ties together all widgets:
 *   - Pacman status animation (keypress-driven dots = one dot per keystroke)
 *   - Battery status display
 *   - BLE connection indicators
 *   - Layer status
 *   - WPM tracking
 *
 * Uses ZMK event system to react to keyboard state changes.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <zmk/display.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/endpoints.h>
#include <zmk/keymap.h>
#include <zmk/ble.h>
#include <zmk/usb.h>

#include "custom_status_screen.h"
#include "widgets/pacman.h"
#include "widgets/theme.h"
#include "widgets/battery_status.h"
#include "widgets/layer_status.h"
#include "widgets/output_status.h"
#include "widgets/wpm.h"

LOG_MODULE_REGISTER(custom_status, CONFIG_DISPLAY_LOG_LEVEL);

/* Singleton state */
static pacman_status_t pacman_st;
static theme_state_t theme_st;
static battery_status_t battery_st;
static layer_status_t layer_st;
static output_status_t output_st;
static wpm_state_t wpm_st;

static const struct device *display_dev = NULL;
static bool initialized = false;

/* Timer for periodic updates (~30fps = 33ms intervals) */
static struct k_timer tick_timer;

/* ---- Timer callback ---- */
static void tick_timer_handler(struct k_timer *timer) {
    if (!initialized) return;

    /* Update WPM calculation */
    wpm_tick(&wpm_st);

    /* Update Pacman animation */
    pacman_status_set_wpm(&pacman_st, wpm_get_current(&wpm_st));
    pacman_status_tick(&pacman_st);

    /* Render */
    pacman_status_render(&pacman_st);
}

/* ---- ZMK Event handlers ---- */

static int ble_profile_changed_handler(const zmk_event_t *eh) {
    /* Dongle connected to host via BLE */
    pacman_status_set_host_connection(&pacman_st, true, 2);
    return ZMK_EV_EVENT_BUBBLE;
}

static int usb_conn_state_changed_handler(const zmk_event_t *eh) {
    const struct zmk_usb_conn_state_changed *ev =
        as_zmk_usb_conn_state_changed(eh);
    if (ev) {
        pacman_status_set_host_connection(&pacman_st, ev->conn_state == ZMK_USB_CONN_HID, 1);
    }
    return ZMK_EV_EVENT_BUBBLE;
}

static int battery_changed_handler(const zmk_event_t *eh) {
    battery_state_update(&battery_st);

    uint8_t left_pct = battery_get_level(&battery_st, 0);
    uint8_t right_pct = battery_get_level(&battery_st, 1);

    pacman_status_set_batteries(&pacman_st, left_pct, right_pct);
    return ZMK_EV_EVENT_BUBBLE;
}

static int layer_changed_handler(const zmk_event_t *eh) {
    layer_state_update(&layer_st);
    return ZMK_EV_EVENT_BUBBLE;
}

static int keycode_changed_handler(const zmk_event_t *eh) {
    const struct zmk_keycode_state_changed *ev =
        as_zmk_keycode_state_changed(eh);
    if (ev && ev->state) {
        /* Key pressed — feed WPM tracker */
        wpm_key_pressed(&wpm_st);
        /* Spawn a dot (or ghost if WPM ≥ threshold) in the Pacman display */
        pacman_status_key_pressed(&pacman_st);
    }
    return ZMK_EV_EVENT_BUBBLE;
}

/* Register event listeners */
ZMK_LISTENER(custom_status_ble, ble_profile_changed_handler);
ZMK_SUBSCRIPTION(custom_status_ble, zmk_ble_active_profile_changed);

ZMK_LISTENER(custom_status_usb, usb_conn_state_changed_handler);
ZMK_SUBSCRIPTION(custom_status_usb, zmk_usb_conn_state_changed);

ZMK_LISTENER(custom_status_battery, battery_changed_handler);
ZMK_SUBSCRIPTION(custom_status_battery, zmk_battery_state_changed);

ZMK_LISTENER(custom_status_layer, layer_changed_handler);
ZMK_SUBSCRIPTION(custom_status_layer, zmk_layer_state_changed);

ZMK_LISTENER(custom_status_keycode, keycode_changed_handler);
ZMK_SUBSCRIPTION(custom_status_keycode, zmk_keycode_state_changed);

/* ---- Public API ---- */

int custom_status_screen_init(void) {
    LOG_INF("Initializing custom status screen");

    /* Get the display device via devicetree chosen node */
    display_dev = DEVICE_DT_GET(DT_CHOSEN(zmk_display));
    if (!device_is_ready(display_dev)) {
        LOG_ERR("Display device not ready");
        return -ENODEV;
    }

    /* Initialize all widgets */
    pacman_status_init(&pacman_st, display_dev);
    theme_init(&theme_st, THEME_PACMAN);
    battery_status_init(&battery_st);
    layer_status_init(&layer_st);
    output_status_init(&output_st);
    wpm_init(&wpm_st);

    /* Set initial values */
    pacman_status_set_host_connection(&pacman_st, false, 0);
    pacman_status_set_batteries(&pacman_st, 100, 100);

    /* Initial render */
    pacman_status_render(&pacman_st);

    /* Start periodic timer (~30fps) */
    k_timer_init(&tick_timer, tick_timer_handler, NULL);
    k_timer_start(&tick_timer, K_MSEC(200), K_MSEC(33));

    initialized = true;
    LOG_INF("Custom status screen initialized");

    return 0;
}

void custom_status_screen_tick(void) {
    if (!initialized) return;
    tick_timer_handler(&tick_timer);
}

void custom_status_screen_redraw(void) {
    if (!initialized) return;
    pacman_st.dirty = true;
    pacman_status_render(&pacman_st);
}

const struct device *custom_status_screen_get_display(void) {
    return display_dev;
}

/* Auto-initialize at application level */
static int custom_status_screen_sys_init(void) {
    return custom_status_screen_init();
}

SYS_INIT(custom_status_screen_sys_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
