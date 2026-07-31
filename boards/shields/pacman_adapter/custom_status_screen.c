/*
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 *
 * Custom Status Screen — main orchestrator
 *
 * Implements zmk_display_status_screen(), the hook ZMK core calls (on
 * zmk_display_work_q(), after lv_init() has run) to obtain the root LVGL
 * object to load. All widget state updates — both the periodic Pacman
 * animation tick and every ZMK event — are marshalled onto that same
 * queue via ZMK_DISPLAY_WIDGET_LISTENER / lv_timer_create(), so nothing
 * here needs its own locking: producer and consumer are the same thread.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>

#include <lvgl.h>

#include <zmk/display.h>
#include <zmk/ble.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/endpoints.h>
#include <zmk/keymap.h>

#include "custom_status_screen.h"
#include "widgets/helpers/display.h"
#include "widgets/pacman.h"
#include "widgets/theme.h"
#include "widgets/battery_status.h"
#include "widgets/layer_status.h"
#include "widgets/output_status.h"
#include "widgets/wpm.h"

LOG_MODULE_REGISTER(custom_status, CONFIG_DISPLAY_LOG_LEVEL);

static pacman_status_t pacman_st;
static theme_state_t    theme_st;
static battery_status_t battery_st;
static layer_status_t   layer_st;
static output_status_t  output_st;
static wpm_state_t      wpm_st;

static const struct device *display_dev;

/* ---- Periodic animation tick (~30fps) ----
 * lv_timer callbacks run from inside lv_task_handler(), which ZMK submits
 * to zmk_display_work_q() on its own tick timer — so this always runs on
 * the display queue, serialized with every widget listener below.
 */
static void render_tick_cb(lv_timer_t *timer) {
    ARG_UNUSED(timer);
    wpm_tick(&wpm_st);
    pacman_status_set_wpm(&pacman_st, wpm_get_current(&wpm_st));
    pacman_status_tick(&pacman_st);
    if (pacman_st.dirty) {
        pacman_status_render(&pacman_st);
    }
}

/* ---- ZMK event -> widget state ----
 * Each ZMK_DISPLAY_WIDGET_LISTENER pair does the thread-unsafe part
 * (reading zmk core state) on whichever thread raised the event, behind
 * a mutex, then invokes the _cb below on the display queue to actually
 * touch pacman_st/output_st. See zmk/display.h.
 */

/* Both the BLE-profile and USB-conn events land here: whichever fired,
 * re-derive the whole picture from zmk core via output_status.c rather
 * than trying to patch just the one transport that changed. */
static void refresh_output_status(void) {
    output_state_update(&output_st);
    bool usb = output_is_usb_connected(&output_st);
    bool ble = output_is_ble_connected(&output_st);
    if (usb) {
        pacman_status_set_host_connection(&pacman_st, true, 1);
    } else if (ble) {
        pacman_status_set_host_connection(&pacman_st, true, 2);
    } else {
        pacman_status_set_host_connection(&pacman_st, false, 0);
    }
}

static bool ble_get_state(const zmk_event_t *eh) {
    ARG_UNUSED(eh);
    return zmk_ble_active_profile_is_connected();
}

static void ble_update_cb(bool connected) {
    ARG_UNUSED(connected);
    refresh_output_status();
}

ZMK_DISPLAY_WIDGET_LISTENER(cs_ble, bool, ble_update_cb, ble_get_state)
ZMK_SUBSCRIPTION(cs_ble, zmk_ble_active_profile_changed);

static bool usb_get_state(const zmk_event_t *eh) {
    const struct zmk_usb_conn_state_changed *ev = as_zmk_usb_conn_state_changed(eh);
    return ev && ev->conn_state == ZMK_USB_CONN_HID;
}

static void usb_update_cb(bool connected) {
    ARG_UNUSED(connected);
    refresh_output_status();
}

ZMK_DISPLAY_WIDGET_LISTENER(cs_usb, bool, usb_update_cb, usb_get_state)
ZMK_SUBSCRIPTION(cs_usb, zmk_usb_conn_state_changed);

/* zmk_peripheral_battery_state_changed carries one half's level at a time
 * (ev->source is 0 or 1); this dongle has no battery a user cares about,
 * so the dongle's own zmk_battery_state_changed is intentionally not
 * subscribed to here. */
struct battery_update {
    uint8_t source;
    uint8_t level;
};

static struct battery_update battery_get_state(const zmk_event_t *eh) {
    const struct zmk_peripheral_battery_state_changed *ev =
        as_zmk_peripheral_battery_state_changed(eh);
    if (!ev) {
        return (struct battery_update){0};
    }
    return (struct battery_update){.source = ev->source, .level = ev->state_of_charge};
}

static void battery_update_cb(struct battery_update upd) {
    battery_set_level(&battery_st, upd.source, upd.level);
    pacman_status_set_batteries(&pacman_st, battery_get_level(&battery_st, 0),
                                 battery_get_level(&battery_st, 1));
}

ZMK_DISPLAY_WIDGET_LISTENER(cs_batt, struct battery_update, battery_update_cb, battery_get_state)
ZMK_SUBSCRIPTION(cs_batt, zmk_peripheral_battery_state_changed);

static uint8_t layer_get_state(const zmk_event_t *eh) {
    ARG_UNUSED(eh);
    layer_state_update(&layer_st);
    return layer_get_active(&layer_st);
}

static void layer_update_cb(uint8_t layer) {
    ARG_UNUSED(layer);
    pacman_status_set_layer(&pacman_st, layer_get_name(&layer_st));
}

ZMK_DISPLAY_WIDGET_LISTENER(cs_layer, uint8_t, layer_update_cb, layer_get_state)
ZMK_SUBSCRIPTION(cs_layer, zmk_layer_state_changed);

static bool keycode_get_state(const zmk_event_t *eh) {
    const struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);
    return ev && ev->state;
}

static void keycode_update_cb(bool pressed) {
    if (!pressed) {
        return;
    }
    wpm_key_pressed(&wpm_st);
    pacman_status_key_pressed(&pacman_st);
}

ZMK_DISPLAY_WIDGET_LISTENER(cs_key, bool, keycode_update_cb, keycode_get_state)
ZMK_SUBSCRIPTION(cs_key, zmk_keycode_state_changed);

/* ---- Public API ---- */

lv_obj_t *zmk_display_status_screen(void) {
    display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
    if (!device_is_ready(display_dev)) {
        LOG_ERR("Display not ready");
        return NULL;
    }

    if (display_init(display_dev) != 0) {
        LOG_ERR("Framebuffer init failed");
        return NULL;
    }

    pacman_status_init(&pacman_st, display_dev);
    theme_init(&theme_st, THEME_PACMAN);
    battery_status_init(&battery_st);
    layer_status_init(&layer_st);
    output_status_init(&output_st);
    wpm_init(&wpm_st);

    pacman_status_set_batteries(&pacman_st, 100, 100);

    /* Seed each widget listener's state now that zmk core is queryable —
     * this immediately calls refresh_output_status() etc., so host
     * connection state doesn't need a separate placeholder default. */
    cs_ble_init();
    cs_usb_init();
    cs_batt_init();
    cs_layer_init();
    cs_key_init();

    lv_timer_create(render_tick_cb, 33, NULL);

    LOG_INF("Custom status screen ready");

    /* The framebuffer is flushed onto lv_layer_top() (see display_init()),
     * which is drawn above every screen, so the screen object itself just
     * needs to exist for lv_scr_load() to have something to load. */
    return lv_obj_create(NULL);
}

const struct device *custom_status_screen_get_display(void) { return display_dev; }
