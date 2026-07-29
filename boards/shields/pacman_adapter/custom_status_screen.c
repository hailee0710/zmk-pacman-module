/*
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 *
 * Custom Status Screen — main orchestrator
 *
 * Reacts to ZMK events (keypress, BLE, USB, battery, layer).
 * Runs Pacman animation at ~30fps via a workqueue (not ISR).
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>

#include <zmk/display.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/endpoints.h>
#include <zmk/keymap.h>
#include <zmk_dongle_events/dongle_action_event.h>

#include "custom_status_screen.h"
#include "widgets/pacman.h"
#include "widgets/theme.h"
#include "widgets/battery_status.h"
#include "widgets/layer_status.h"
#include "widgets/output_status.h"
#include "widgets/wpm.h"

LOG_MODULE_REGISTER(custom_status, CONFIG_DISPLAY_LOG_LEVEL);

K_THREAD_STACK_DEFINE(render_stack, 4096);
static struct k_work_q render_workq;

static pacman_status_t pacman_st;
static theme_state_t    theme_st;
static battery_status_t battery_st;
static layer_status_t   layer_st;
static output_status_t  output_st;
static wpm_state_t      wpm_st;

static const struct device *display_dev;
static bool initialized;

/* ---- Deferred render work (runs in thread context, safe for LVGL) ---- */

static void render_work_handler(struct k_work *work) {
    if (!initialized) return;
    wpm_tick(&wpm_st);
    pacman_status_set_wpm(&pacman_st, wpm_get_current(&wpm_st));
    pacman_status_tick(&pacman_st);
    /* Only render if something changed */
    if (pacman_st.dirty) {
        pacman_status_render(&pacman_st);
    }
}

static K_WORK_DEFINE(render_work, render_work_handler);

/* Timer callback (ISR) — just kicks the workqueue */
static void tick_timer_handler(struct k_timer *unused) {
    k_work_submit_to_queue(&render_workq, &render_work);
}

K_TIMER_DEFINE(tick_timer, tick_timer_handler, NULL);

/* ---- ZMK event handlers ---- */

static int ble_handler(const zmk_event_t *eh) {
    const struct zmk_ble_active_profile_changed *ev = as_zmk_ble_active_profile_changed(eh);
    if (ev) {
        /* profile_index of 0xFF means disconnected; anything else is connected */
        bool connected = (ev->profile_index != 0xFF);
        pacman_status_set_host_connection(&pacman_st, connected, 2);
    }
    return ZMK_EV_EVENT_BUBBLE;
}

static int usb_handler(const zmk_event_t *eh) {
    const struct zmk_usb_conn_state_changed *ev = as_zmk_usb_conn_state_changed(eh);
    if (ev) pacman_status_set_host_connection(&pacman_st, ev->conn_state, 1);
    return ZMK_EV_EVENT_BUBBLE;
}

static int battery_handler(const zmk_event_t *eh) {
    battery_state_update(&battery_st);
    pacman_status_set_batteries(&pacman_st,
        battery_get_level(&battery_st, 0), battery_get_level(&battery_st, 1));
    return ZMK_EV_EVENT_BUBBLE;
}

static int layer_handler(const zmk_event_t *eh) {
    layer_state_update(&layer_st);
    return ZMK_EV_EVENT_BUBBLE;
}

static int keycode_handler(const zmk_event_t *eh) {
    const struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);
    if (ev && ev->state) {
        wpm_key_pressed(&wpm_st);
        pacman_status_key_pressed(&pacman_st);
    }
    return ZMK_EV_EVENT_BUBBLE;
}

static int dongle_action_handler(const zmk_event_t *eh) {
    const struct zmk_dongle_action_event *ev = as_zmk_dongle_action_event(eh);
    if (!ev) return ZMK_EV_EVENT_BUBBLE;

    LOG_DBG("Dongle action received: %d", ev->action);

    switch (ev->action) {
    case DONGLE_ACTION_PACMAN_UP:
        /* Reserved for future game navigation */
        break;
    case DONGLE_ACTION_PACMAN_DOWN:
        break;
    case DONGLE_ACTION_PACMAN_LEFT:
        break;
    case DONGLE_ACTION_PACMAN_RIGHT:
        break;
    case DONGLE_ACTION_PACMAN_START:
        /* Start/restart game or animation */
        break;
    case DONGLE_ACTION_PACMAN_PAUSE:
        /* Toggle pause */
        break;
    case DONGLE_ACTION_PACMAN_QUIT:
        /* Quit to status screen */
        break;
    default:
        break;
    }

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(cs_ble, ble_handler);
ZMK_SUBSCRIPTION(cs_ble, zmk_ble_active_profile_changed);
ZMK_LISTENER(cs_usb, usb_handler);
ZMK_SUBSCRIPTION(cs_usb, zmk_usb_conn_state_changed);
ZMK_LISTENER(cs_batt, battery_handler);
ZMK_SUBSCRIPTION(cs_batt, zmk_battery_state_changed);
ZMK_LISTENER(cs_layer, layer_handler);
ZMK_SUBSCRIPTION(cs_layer, zmk_layer_state_changed);
ZMK_LISTENER(cs_key, keycode_handler);
ZMK_SUBSCRIPTION(cs_key, zmk_keycode_state_changed);
ZMK_LISTENER(cs_dongle_action, dongle_action_handler);
ZMK_SUBSCRIPTION(cs_dongle_action, zmk_dongle_action_event);

/* ---- Public API ---- */

int custom_status_screen_init(void) {
    LOG_INF("Initializing custom status screen");

    display_dev = DEVICE_DT_GET(DT_CHOSEN(zmk_display));
    if (!device_is_ready(display_dev)) { LOG_ERR("Display not ready"); return -ENODEV; }

    display_init(display_dev);

    pacman_status_init(&pacman_st, display_dev);
    theme_init(&theme_st, THEME_PACMAN);
    battery_status_init(&battery_st);
    layer_status_init(&layer_st);
    output_status_init(&output_st);
    wpm_init(&wpm_st);

    pacman_status_set_host_connection(&pacman_st, false, 0);
    pacman_status_set_batteries(&pacman_st, 100, 100);

    /* Start workqueue and periodic timer (~30fps) */
    k_work_queue_init(&render_workq);
    k_work_queue_start(&render_workq, render_stack,
                       K_THREAD_STACK_SIZEOF(render_stack),
                       K_PRIO_PREEMPT(5), NULL);
    k_timer_start(&tick_timer, K_MSEC(200), K_MSEC(33));

    initialized = true;
    LOG_INF("Custom status screen ready");
    return 0;
}

void custom_status_screen_redraw(void) {
    if (initialized) { pacman_st.dirty = true; pacman_status_render(&pacman_st); }
}

const struct device *custom_status_screen_get_display(void) { return display_dev; }

SYS_INIT(custom_status_screen_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
