/*
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 *
 * Pacman Status Widget — landscape 320×172
 *
 * Layout:
 *   ┌──────────────────────────────────────────────────────────────┐ y=0
 *   │ ████ 85%  ████ 72% │ PACMAN DONGLE │ 🟢 BLE (or 🔵 USB)     │  top bar (24px)
 *   │ left batt   right batt           │ host link indicator       │
 *   ├──────────────────────────────────────────────────────────────┤ y=24
 *   │                                                              │
 *   │        😮  •   •   •   •   •   •   •   •   •   •            │  main zone (124px)
 *   │        🟡 ← each dot = one keystroke, speed ∝ WPM            │  single row
 *   │                                                              │
 *   ├──────────────────────────────────────────────────────────────┤ y=148
 *   │ [████████████░░░░░░]│80  G:3  D:142 SCORE:1240 WPM:045 PEAK:089│  bottom bar (24px)
 *   └──────────────────────────────────────────────────────────────┘ y=171
 *
 * Each dot/ghost in the main zone represents a keypress you made.
 * Dots spawn on keypress at the right edge, flow left to Pacman.
 * If WPM ≥ 80, the dot becomes a ghost instead.
 * WPM bar shows current speed; │80 marks where ghosts start.
 * Ghost-eating triggers a POWER! special effect.
 */

#pragma once

#include <zephyr/device.h>
#include <stdbool.h>
#include <stdint.h>

/* ---- Layout constants (320×172 landscape) ---- */
#define SCREEN_W  320
#define SCREEN_H  172

#define TOP_BAR_Y     0
#define TOP_BAR_H     24

#define MAIN_ZONE_Y   24
#define MAIN_ZONE_H   124
#define MAIN_ZONE_CY  (MAIN_ZONE_Y + MAIN_ZONE_H / 2)  /* 86 */

#define BOTTOM_BAR_Y  148
#define BOTTOM_BAR_H  24

/* Pacman */
#define PACMAN_CX      75
#define PACMAN_CY      MAIN_ZONE_CY
#define PACMAN_RADIUS  40
#define PACMAN_MOUTH_X (PACMAN_CX - PACMAN_RADIUS)  /* 35 */

/* Dots — single horizontal row, each dot = one keypress */
#define DOT_RADIUS         4
#define DOT_Y              MAIN_ZONE_CY     /* 86 */
#define DOT_SPAWN_X        305
#define DOT_EAT_X          PACMAN_MOUTH_X   /* 35 */
#define DOT_MAX_COUNT      20

/* Ghost */
#define GHOST_W         28
#define GHOST_H         32

/* Animation mapping */
#define DOT_SPEED_MIN           1
#define DOT_SPEED_MAX           5
#define WPM_GHOST_THRESHOLD     80

/* Special effect duration (ticks) */
#define EFFECT_DURATION  30

/* Battery icon dimensions */
#define BATTERY_ICON_X  4
#define BATTERY_ICON_W  24
#define BATTERY_ICON_H  12

/* ---- Dot particle = one keypress ---- */
typedef struct {
    int16_t  x;
    bool     active;
    bool     is_ghost;       /* true if WPM ≥ threshold at press time */
    uint8_t  speed;          /* px/tick, based on WPM at press time */
} pacman_dot_t;

/* ---- Pacman status state ---- */
typedef struct {
    /* Pacman animation */
    uint8_t  mouth_frame;
    int8_t   mouth_delta;
    uint16_t anim_tick;

    /* Dots flowing (keypress-driven) */
    pacman_dot_t dots[DOT_MAX_COUNT];
    uint16_t dot_spawn_interval;   /* unused now, kept for compat */

    /* Ghost-eating effect */
    bool     effect_active;
    uint16_t effect_timer;
    uint8_t  effect_phase;

    /* Stats */
    uint16_t dots_eaten;
    uint16_t ghosts_eaten;
    uint16_t score;

    /* WPM */
    uint8_t  current_wpm;
    uint8_t  peak_wpm;

    /* Dongle → host connection */
    bool     host_connected;      /* true when dongle is linked to computer */
    uint8_t  host_transport;      /* 0 = none, 1 = USB, 2 = BLE */

    /* Peripheral batteries */
    uint8_t  left_battery;
    uint8_t  right_battery;

    /* Display */
    const struct device *dev;
    bool dirty;
    bool initialized;
} pacman_status_t;

/* ---- Public API ---- */
void pacman_status_init(pacman_status_t *st, const struct device *dev);
void pacman_status_tick(pacman_status_t *st);
void pacman_status_render(pacman_status_t *st);

/* Called on each keypress to spawn a dot (or ghost if WPM ≥ threshold) */
void pacman_status_key_pressed(pacman_status_t *st);

void pacman_status_set_wpm(pacman_status_t *st, uint8_t wpm);
void pacman_status_set_host_connection(pacman_status_t *st, bool connected, uint8_t transport);
void pacman_status_set_batteries(pacman_status_t *st, uint8_t lp, uint8_t rp);
