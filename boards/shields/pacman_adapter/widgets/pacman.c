/*
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 *
 * Pacman Status Widget — landscape 320×172
 *
 * - Each dot in the main zone = one keypress you made.
 * - Dots spawn at right edge, flow left toward Pacman.
 * - If WPM ≥ 80 at press time, the dot is a ghost instead.
 * - Battery displayed as graphical icon with green→yellow→red.
 * - Ghost/dots counters in bottom bar; │80 is ghost threshold marker.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <stdio.h>

#include "pacman.h"
#include "helpers/display.h"

LOG_MODULE_REGISTER(pacman_status, CONFIG_DISPLAY_LOG_LEVEL);

/* ---- Internal helpers ---- */

static uint8_t wpm_to_dot_speed(uint8_t wpm) {
    if (wpm > 120) wpm = 120;
    if (wpm == 0)  return DOT_SPEED_MIN;
    return DOT_SPEED_MIN + (uint8_t)((uint16_t)wpm * (DOT_SPEED_MAX - DOT_SPEED_MIN) / 120);
}

static bool wpm_is_ghost_territory(uint8_t wpm) {
    return wpm >= WPM_GHOST_THRESHOLD;
}

/* Battery: green >60%, yellow 25-60%, red <25% */
static uint16_t batt_fill_color(uint8_t pct) {
    if (pct > 60) return COLOR_GREEN;
    if (pct > 25) return COLOR_YELLOW;
    return COLOR_RED;
}

/* Draw a battery icon at (x, y) with given percentage */
static void draw_battery_icon(const struct device *dev,
                               uint16_t x, uint16_t y,
                               uint8_t pct) {
    uint16_t body_w = 22;
    uint16_t body_h = 12;
    uint16_t nip_w  = 3;
    uint16_t nip_h  = 6;

    /* Battery body outline */
    display_draw_rect(dev, x, y, x + body_w, y + body_h, COLOR_WHITE);
    /* Nipple on the right */
    display_fill_rect(dev, x + body_w + 1, y + (body_h - nip_h) / 2,
                      x + body_w + nip_w, y + (body_h + nip_h) / 2,
                      COLOR_WHITE);

    /* Fill level */
    if (pct > 0) {
        uint16_t fill_w = (uint16_t)(body_w - 2) * (pct > 100 ? 100 : pct) / 100;
        if (fill_w < 1) fill_w = 1;
        display_fill_rect(dev, x + 1, y + 1, x + fill_w, y + body_h - 1,
                          batt_fill_color(pct));
    }
}

/* Find a free dot slot */
static int8_t find_free_dot(pacman_status_t *st) {
    for (int i = 0; i < DOT_MAX_COUNT; i++)
        if (!st->dots[i].active) return i;
    return -1;
}

/* Spawn a dot (or ghost) — called on every keypress */
static void spawn_dot(pacman_status_t *st) {
    int8_t idx = find_free_dot(st);
    if (idx < 0) {
        /* All slots full — reclaim the rightmost one about to be eaten */
        int16_t best = -1;
        int8_t  best_i = -1;
        for (int i = 0; i < DOT_MAX_COUNT; i++) {
            if (st->dots[i].active && st->dots[i].x > best) {
                best   = st->dots[i].x;
                best_i = i;
            }
        }
        idx = best_i;
        if (idx < 0) return;
    }

    pacman_dot_t *d = &st->dots[idx];
    d->active   = true;
    d->is_ghost = wpm_is_ghost_territory(st->current_wpm);
    d->x        = DOT_SPAWN_X;
    d->speed    = wpm_to_dot_speed(st->current_wpm);
}

/* Trigger ghost-eating special effect */
static void trigger_ghost_effect(pacman_status_t *st) {
    st->effect_active = true;
    st->effect_timer  = EFFECT_DURATION;
    st->effect_phase  = 0;
    st->ghosts_eaten++;
    st->score += 200;
}

/* ---- Public API ---- */

void pacman_status_init(pacman_status_t *st, const struct device *dev) {
    memset(st, 0, sizeof(*st));
    st->dev          = dev;
    st->mouth_frame  = 2;
    st->mouth_delta  = 1;
    st->initialized  = true;
    LOG_INF("Pacman status init (landscape, keypress-driven dots)");
}

void pacman_status_set_wpm(pacman_status_t *st, uint8_t wpm) {
    st->current_wpm = wpm;
}

void pacman_status_set_host_connection(pacman_status_t *st, bool connected, uint8_t transport) {
    st->host_connected = connected;
    st->host_transport = transport;
    st->dirty = true;
}

void pacman_status_set_batteries(pacman_status_t *st, uint8_t lp, uint8_t rp) {
    st->left_battery  = lp;
    st->right_battery = rp;
    st->dirty = true;
}

/* Called on each keypress — spawns a dot (or ghost if WPM ≥ threshold) */
void pacman_status_key_pressed(pacman_status_t *st) {
    if (!st->initialized) return;
    spawn_dot(st);
}

/* ---- Tick (~30 Hz) ---- */

void pacman_status_tick(pacman_status_t *st) {
    if (!st->initialized) return;
    st->anim_tick++;

    /* Mouth animation */
    st->mouth_frame += st->mouth_delta;
    if      (st->mouth_frame >= 3) { st->mouth_frame = 3; st->mouth_delta = -1; }
    else if (st->mouth_frame <= 0) { st->mouth_frame = 0; st->mouth_delta =  1; }

    /* Move dots leftward */
    for (int i = 0; i < DOT_MAX_COUNT; i++) {
        if (!st->dots[i].active) continue;
        pacman_dot_t *d = &st->dots[i];
        d->x -= d->speed;

        if (d->x <= DOT_EAT_X) {
            if (d->is_ghost) {
                trigger_ghost_effect(st);
            } else {
                st->dots_eaten++;
                st->score += 10;
            }
            d->active = false;
        }
    }

    /* Ghost-eating effect countdown */
    if (st->effect_active) {
        st->effect_timer--;
        st->effect_phase = (st->effect_timer / 10) % 3;
        if (st->effect_timer == 0) st->effect_active = false;
    }

    /* Peak tracking */
    if (st->current_wpm > st->peak_wpm) st->peak_wpm = st->current_wpm;

    st->dirty = true;
}

/* ---- Rendering ---- */

static void render_top_bar(pacman_status_t *st) {
    const struct device *dev = st->dev;
    char buf[16];

    /* Background */
    display_fill_rect(dev, 0, 0, SCREEN_W - 1, TOP_BAR_H - 1, COLOR_BLACK);

    /* ── Left: two peripheral battery icons ── */
    draw_battery_icon(dev, 4, 6, st->left_battery);
    snprintf(buf, sizeof(buf), "%u%%", st->left_battery);
    display_write_text(dev, 30, 4, buf, batt_fill_color(st->left_battery),
                       COLOR_BLACK, 1);

    draw_battery_icon(dev, 68, 6, st->right_battery);
    snprintf(buf, sizeof(buf), "%u%%", st->right_battery);
    display_write_text(dev, 94, 4, buf, batt_fill_color(st->right_battery),
                       COLOR_BLACK, 1);

    /* ── Center title ── */
    display_write_text(dev, 120, 4, "PACMAN", COLOR_PACMAN_YELLOW, COLOR_BLACK, 1);
    display_write_text(dev, 172, 4, "DONGLE", COLOR_WHITE, COLOR_BLACK, 1);

    /* ── Right: dongle to host connection ── */
    if (st->host_connected) {
        const char *label;
        uint16_t color;
        if (st->host_transport == 1) {
            label = "USB";
            color = COLOR_BLUE;
        } else {
            label = "BLE";
            color = COLOR_GREEN;
        }
        display_draw_filled_circle(dev, 260, 12, 4, color);
        display_write_text(dev, 270, 4, label, color, COLOR_BLACK, 1);
    } else {
        display_draw_filled_circle(dev, 260, 12, 4, COLOR_RED);
        display_write_text(dev, 270, 4, "---", COLOR_RED, COLOR_BLACK, 1);
    }

    /* Separator */
    display_draw_line(dev, 0, TOP_BAR_H - 1, SCREEN_W - 1, TOP_BAR_H - 1,
                      COLOR_DARK_GRAY);
}

static void render_main_zone(pacman_status_t *st) {
    const struct device *dev = st->dev;

    /* Clear */
    display_fill_rect(dev, 0, MAIN_ZONE_Y, SCREEN_W - 1,
                      MAIN_ZONE_Y + MAIN_ZONE_H - 1, COLOR_BLACK);

    /* Subtle center guide line */
    display_draw_line(dev, DOT_EAT_X + 10, MAIN_ZONE_CY,
                      DOT_SPAWN_X - 10,  MAIN_ZONE_CY,
                      COLOR_DARK_GRAY);

    /* ── Dots and ghosts ── */
    for (int i = 0; i < DOT_MAX_COUNT; i++) {
        if (!st->dots[i].active) continue;
        pacman_dot_t *d = &st->dots[i];

        if (d->is_ghost) {
            display_draw_ghost(dev,
                d->x - GHOST_W / 2, DOT_Y - GHOST_H / 2,
                GHOST_W, GHOST_H,
                COLOR_GHOST_RED, true);
        } else {
            display_draw_dot(dev, d->x, DOT_Y, DOT_RADIUS, COLOR_DOT_WHITE);
        }
    }

    /* ── Special effect halo ── */
    if (st->effect_active) {
        uint16_t flash;
        switch (st->effect_phase) {
            case 0: flash = COLOR_GHOST_RED;    break;
            case 1: flash = COLOR_PACMAN_YELLOW; break;
            default:flash = COLOR_GHOST_CYAN;   break;
        }
        uint16_t r = PACMAN_RADIUS + 12 + (st->effect_timer % 8);
        display_draw_circle(dev, PACMAN_CX, PACMAN_CY, r, flash);

        if (st->effect_timer > EFFECT_DURATION / 2) {
            display_write_text(dev, PACMAN_CX + 48, PACMAN_CY - 10,
                               "POWER!", COLOR_GHOST_RED, COLOR_BLACK, 1);
        }
    }

    /* ── Big Pacman ── */
    display_draw_pacman(dev, PACMAN_CX, PACMAN_CY, PACMAN_RADIUS,
                        DIR_RIGHT, st->mouth_frame, COLOR_PACMAN_YELLOW);

    /* Separator */
    display_draw_line(dev, 0, BOTTOM_BAR_Y - 1, SCREEN_W - 1,
                      BOTTOM_BAR_Y - 1, COLOR_DARK_GRAY);
}

static void render_bottom_bar(pacman_status_t *st) {
    const struct device *dev = st->dev;
    char buf[32];

    display_fill_rect(dev, 0, BOTTOM_BAR_Y, SCREEN_W - 1, SCREEN_H - 1, COLOR_BLACK);

    /* ── WPM bar ── */
    uint16_t bar_x = 4, bar_y = BOTTOM_BAR_Y + 5, bar_w = 160, bar_h = 10;
    display_draw_rect(dev, bar_x, bar_y, bar_x + bar_w, bar_y + bar_h, COLOR_DARK_GRAY);

    uint16_t wpm = (st->current_wpm > 120) ? 120 : st->current_wpm;
    uint16_t fill_w = (uint16_t)bar_w * wpm / 120;
    uint16_t fill_c  = (wpm >= WPM_GHOST_THRESHOLD) ? COLOR_GHOST_RED
                     : (wpm > 40)                   ? COLOR_PACMAN_YELLOW
                     :                                COLOR_GREEN;
    if (fill_w > 0)
        display_fill_rect(dev, bar_x + 1, bar_y + 1,
                          bar_x + fill_w, bar_y + bar_h - 1, fill_c);

    /* Ghost threshold marker and label */
    uint16_t mk = bar_x + (uint16_t)bar_w * WPM_GHOST_THRESHOLD / 120;
    display_draw_line(dev, mk, bar_y - 2, mk, bar_y + bar_h + 2, COLOR_GHOST_RED);
    snprintf(buf, sizeof(buf), "%u", WPM_GHOST_THRESHOLD);
    display_write_text(dev, mk - 10, BOTTOM_BAR_Y + 16, buf,
                       COLOR_GHOST_RED, COLOR_BLACK, 1);

    /* ── Stats ── */
    snprintf(buf, sizeof(buf), "G:%u", st->ghosts_eaten);
    display_write_text(dev, 172, BOTTOM_BAR_Y + 1, buf, COLOR_GHOST_RED, COLOR_BLACK, 1);

    snprintf(buf, sizeof(buf), "D:%u", st->dots_eaten);
    display_write_text(dev, 210, BOTTOM_BAR_Y + 1, buf, COLOR_DOT_WHITE, COLOR_BLACK, 1);

    snprintf(buf, sizeof(buf), "SCORE:%u", st->score);
    display_write_text(dev, 172, BOTTOM_BAR_Y + 12, buf,
                       COLOR_PACMAN_YELLOW, COLOR_BLACK, 1);

    /* WPM display */
    snprintf(buf, sizeof(buf), "WPM:%u", st->current_wpm);
    display_write_text(dev, 258, BOTTOM_BAR_Y + 1, buf, COLOR_WHITE, COLOR_BLACK, 1);

    snprintf(buf, sizeof(buf), "PEAK:%u", st->peak_wpm);
    display_write_text(dev, 258, BOTTOM_BAR_Y + 12, buf, COLOR_GRAY, COLOR_BLACK, 1);
}

void pacman_status_render(pacman_status_t *st) {
    if (!st->dirty || !st->initialized) return;

    render_top_bar(st);
    render_main_zone(st);
    render_bottom_bar(st);

    st->dirty = false;
}
