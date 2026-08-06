/*
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 *
 * Pacman Status Widget — landscape 320×172
 *
 * Each dot = one keypress. Ghost if WPM ≥ 80.
 * Single-row horizontal dot flow toward big, stationary Pacman.
 */

#pragma once

#include <zephyr/device.h>
#include <stdbool.h>
#include <stdint.h>

#define SCREEN_W  320
#define SCREEN_H  172

#define TOP_BAR_Y     0
#define TOP_BAR_H     36

#define MAIN_ZONE_Y   36
#define MAIN_ZONE_H   96
#define MAIN_ZONE_CY  (MAIN_ZONE_Y + MAIN_ZONE_H / 2)  /* 84 */

#define BOTTOM_BAR_Y  132
#define BOTTOM_BAR_H  40

/* Big Pacman */
#define PACMAN_CX      100
#define PACMAN_CY      MAIN_ZONE_CY
#define PACMAN_RADIUS  38
#define PACMAN_MOUTH_X (PACMAN_CX - PACMAN_RADIUS)  /* 62 */

#define DOT_RADIUS      4
#define DOT_Y           MAIN_ZONE_CY
#define DOT_SPAWN_X     310
#define DOT_EAT_X       PACMAN_MOUTH_X
#define DOT_MAX_COUNT   20
#define DOT_SPACING     18  /* minimum center-to-center pixel gap */

#define GHOST_W  28
#define GHOST_H  32

#define DOT_SPEED_MIN       1
#define DOT_SPEED_MAX       8
#define WPM_GHOST_THRESHOLD 80

/* Dirty zone bits — track which screen bands need redraw.
 * Kept in sync with display.h zone layout. */
#define DIRTY_TOP    0x01
#define DIRTY_MAIN   0x02
#define DIRTY_BOTTOM 0x04

typedef struct {
    int16_t  x;
    bool     active;
    bool     is_ghost;
    uint8_t  speed;
} pacman_dot_t;

typedef struct {
    uint8_t  mouth_frame;
    int8_t   mouth_delta;
    uint16_t anim_tick;

    pacman_dot_t dots[DOT_MAX_COUNT];

    uint8_t  current_wpm;
    uint8_t  peak_wpm;

    bool     host_connected;
    uint8_t  host_transport;   /* 0=none, 1=USB, 2=BLE */

    uint8_t  left_battery;
    uint8_t  right_battery;

    char     layer_name[8];

    const struct device *dev;
    uint8_t dirty_zones;   /* bitmask of DIRTY_TOP | DIRTY_MAIN | DIRTY_BOTTOM */
    bool initialized;
} pacman_status_t;

void pacman_status_init(pacman_status_t *st, const struct device *dev);
void pacman_status_tick(pacman_status_t *st);
void pacman_status_render(pacman_status_t *st);
void pacman_status_key_pressed(pacman_status_t *st);

void pacman_status_set_wpm(pacman_status_t *st, uint8_t wpm);
void pacman_status_set_host_connection(pacman_status_t *st, bool connected, uint8_t transport);
void pacman_status_set_batteries(pacman_status_t *st, uint8_t lp, uint8_t rp);
void pacman_status_set_layer(pacman_status_t *st, const char *name);
