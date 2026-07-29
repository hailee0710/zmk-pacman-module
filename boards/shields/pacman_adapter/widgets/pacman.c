/*
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 *
 * Pacman Status Widget — landscape 320×172
 *
 * Each dot = one keypress. Ghost if WPM ≥ 80.
 * All drawing goes through framebuffer; single flush per frame.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <stdio.h>

#include "pacman.h"
#include "helpers/display.h"

LOG_MODULE_REGISTER(pacman_status, CONFIG_DISPLAY_LOG_LEVEL);

/* ---- Helpers ---- */

static uint8_t wpm_to_speed(uint8_t wpm) {
    if (wpm > 120) wpm = 120;
    if (wpm == 0)  return DOT_SPEED_MIN;
    return DOT_SPEED_MIN + (uint8_t)((uint16_t)wpm * (DOT_SPEED_MAX - DOT_SPEED_MIN) / 120);
}

static bool is_ghost_zone(uint8_t wpm) { return wpm >= WPM_GHOST_THRESHOLD; }

static uint16_t batt_color(uint8_t pct) {
    if (pct > 60) return COLOR_GREEN;
    if (pct > 25) return COLOR_YELLOW;
    return COLOR_RED;
}

/* Draw a small battery icon at (x,y) */
static void draw_battery(uint16_t x, uint16_t y, uint8_t pct) {
    uint16_t bw = 22, bh = 12, nip_w = 3, nip_h = 6;
    display_draw_rect(x, y, bw + 1, bh + 1, COLOR_WHITE);
    display_fill_rect(x + bw + 1, y + (bh - nip_h) / 2, nip_w, nip_h, COLOR_WHITE);
    if (pct > 0) {
        uint16_t fw = (uint16_t)(bw - 2) * (pct > 100 ? 100 : pct) / 100;
        if (fw < 1) fw = 1;
        display_fill_rect(x + 1, y + 1, fw, bh - 2, batt_color(pct));
    }
}

/* Find a free dot slot, or recycle the rightmost one */
static int8_t find_slot(pacman_status_t *st) {
    for (int i = 0; i < DOT_MAX_COUNT; i++)
        if (!st->dots[i].active) return i;
    int16_t best = -1; int8_t best_i = -1;
    for (int i = 0; i < DOT_MAX_COUNT; i++)
        if (st->dots[i].active && st->dots[i].x > best) { best = st->dots[i].x; best_i = i; }
    return best_i;
}

static void spawn(pacman_status_t *st) {
    int8_t idx = find_slot(st);
    if (idx < 0) return;
    pacman_dot_t *d = &st->dots[idx];
    d->active   = true;
    d->is_ghost = is_ghost_zone(st->current_wpm);
    d->x        = DOT_SPAWN_X;
    d->speed    = wpm_to_speed(st->current_wpm);
}

static void effect(pacman_status_t *st) {
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
}

void pacman_status_set_wpm(pacman_status_t *st, uint8_t wpm) { st->current_wpm = wpm; }

void pacman_status_set_host_connection(pacman_status_t *st, bool c, uint8_t t) {
    st->host_connected = c; st->host_transport = t; st->dirty = true;
}

void pacman_status_set_batteries(pacman_status_t *st, uint8_t l, uint8_t r) {
    st->left_battery = l; st->right_battery = r; st->dirty = true;
}

void pacman_status_key_pressed(pacman_status_t *st) {
    if (!st->initialized) return;
    spawn(st);
}

/* ---- Tick ---- */

void pacman_status_tick(pacman_status_t *st) {
    if (!st->initialized) return;
    st->anim_tick++;

    static uint8_t prev_mouth = 0xFF;
    static uint8_t last_wpm = 0xFF;

    /* Mouth */
    st->mouth_frame += st->mouth_delta;
    if (st->mouth_frame >= 3) { st->mouth_frame = 3; st->mouth_delta = -1; }
    else if (st->mouth_frame <= 0) { st->mouth_frame = 0; st->mouth_delta = 1; }

    bool any_dot_active = false;

    /* Move dots */
    for (int i = 0; i < DOT_MAX_COUNT; i++) {
        if (!st->dots[i].active) continue;
        any_dot_active = true;
        pacman_dot_t *d = &st->dots[i];
        d->x -= d->speed;
        if (d->x <= DOT_EAT_X) {
            if (d->is_ghost) effect(st);
            else { st->dots_eaten++; st->score += 10; }
            d->active = false;
        }
    }

    /* Effect timer */
    bool effect_was_active = st->effect_active;
    if (st->effect_active) {
        st->effect_timer--;
        st->effect_phase = (st->effect_timer / 10) % 3;
        if (st->effect_timer == 0) st->effect_active = false;
    }

    if (st->current_wpm > st->peak_wpm) st->peak_wpm = st->current_wpm;

    /* Only mark dirty if something actually changed */
    if (any_dot_active ||
        st->mouth_frame != prev_mouth ||
        st->effect_active || effect_was_active ||
        st->current_wpm != last_wpm) {
        st->dirty = true;
    }

    prev_mouth = st->mouth_frame;
    last_wpm = st->current_wpm;
}

/* ---- Rendering ---- */

static void render_top_bar(pacman_status_t *st) {
    char buf[16];
    display_fill_rect(0, 0, SCREEN_W, TOP_BAR_H, COLOR_BLACK);

    /* Left battery */
    draw_battery(4, 6, st->left_battery);
    snprintf(buf, sizeof(buf), "%u%%", st->left_battery);
    display_write_text(30, 4, buf, batt_color(st->left_battery), COLOR_BLACK, 1);

    draw_battery(68, 6, st->right_battery);
    snprintf(buf, sizeof(buf), "%u%%", st->right_battery);
    display_write_text(94, 4, buf, batt_color(st->right_battery), COLOR_BLACK, 1);

    /* Title */
    display_write_text(120, 4, "PACMAN", COLOR_PACMAN_YELLOW, COLOR_BLACK, 1);
    display_write_text(172, 4, "DONGLE", COLOR_WHITE, COLOR_BLACK, 1);

    /* Host connection */
    if (st->host_connected) {
        const char *label; uint16_t color;
        if (st->host_transport == 1) { label = "USB"; color = COLOR_BLUE; }
        else                         { label = "BLE"; color = COLOR_GREEN; }
        display_draw_filled_circle(260, 12, 4, color);
        display_write_text(270, 4, label, color, COLOR_BLACK, 1);
    } else {
        display_draw_filled_circle(260, 12, 4, COLOR_RED);
        display_write_text(270, 4, "---", COLOR_RED, COLOR_BLACK, 1);
    }

    display_draw_line(0, TOP_BAR_H - 1, SCREEN_W - 1, TOP_BAR_H - 1, COLOR_DARK_GRAY);
}

static void render_zone(pacman_status_t *st) {
    display_fill_rect(0, MAIN_ZONE_Y, SCREEN_W, MAIN_ZONE_H, COLOR_BLACK);

    /* Guide line */
    display_draw_line(DOT_EAT_X + 10, MAIN_ZONE_CY, DOT_SPAWN_X - 10, MAIN_ZONE_CY, COLOR_DARK_GRAY);

    /* Dots + ghosts */
    for (int i = 0; i < DOT_MAX_COUNT; i++) {
        if (!st->dots[i].active) continue;
        pacman_dot_t *d = &st->dots[i];
        if (d->is_ghost)
            display_draw_ghost(d->x - GHOST_W / 2, DOT_Y - GHOST_H / 2, GHOST_W, GHOST_H,
                               COLOR_GHOST_RED, true);
        else
            display_draw_dot(d->x, DOT_Y, DOT_RADIUS, COLOR_DOT_WHITE);
    }

    /* Effect halo */
    if (st->effect_active) {
        uint16_t flash;
        switch (st->effect_phase) {
            case 0: flash = COLOR_GHOST_RED;    break;
            case 1: flash = COLOR_PACMAN_YELLOW; break;
            default:flash = COLOR_GHOST_CYAN;   break;
        }
        display_draw_circle(PACMAN_CX, PACMAN_CY, PACMAN_RADIUS + 12 + (st->effect_timer % 8), flash);
        if (st->effect_timer > EFFECT_DURATION / 2)
            display_write_text(PACMAN_CX + 48, PACMAN_CY - 10, "POWER!", COLOR_GHOST_RED, COLOR_BLACK, 1);
    }

    /* Pacman */
    display_draw_pacman(PACMAN_CX, PACMAN_CY, PACMAN_RADIUS, DIR_RIGHT, st->mouth_frame,
                        COLOR_PACMAN_YELLOW);

    display_draw_line(0, BOTTOM_BAR_Y - 1, SCREEN_W - 1, BOTTOM_BAR_Y - 1, COLOR_DARK_GRAY);
}

static void render_bottom_bar(pacman_status_t *st) {
    char buf[32];
    display_fill_rect(0, BOTTOM_BAR_Y, SCREEN_W, BOTTOM_BAR_H, COLOR_BLACK);

    /* WPM bar */
    uint16_t bx = 4, by = BOTTOM_BAR_Y + 5, bw = 160, bh = 10;
    display_draw_rect(bx, by, bw + 1, bh + 1, COLOR_DARK_GRAY);

    uint16_t wpm = (st->current_wpm > 120) ? 120 : st->current_wpm;
    uint16_t fw = (uint16_t)bw * wpm / 120;
    uint16_t fc = (wpm >= WPM_GHOST_THRESHOLD) ? COLOR_GHOST_RED
                : (wpm > 40)                   ? COLOR_PACMAN_YELLOW
                :                                COLOR_GREEN;
    if (fw > 0) display_fill_rect(bx + 1, by + 1, fw, bh - 1, fc);

    uint16_t mk = bx + (uint16_t)bw * WPM_GHOST_THRESHOLD / 120;
    display_draw_line(mk, by - 2, mk, by + bh + 2, COLOR_GHOST_RED);
    snprintf(buf, sizeof(buf), "%u", WPM_GHOST_THRESHOLD);
    display_write_text(mk - 10, BOTTOM_BAR_Y + 16, buf, COLOR_GHOST_RED, COLOR_BLACK, 1);

    /* Stats */
    snprintf(buf, sizeof(buf), "G:%u", st->ghosts_eaten);
    display_write_text(172, BOTTOM_BAR_Y + 1, buf, COLOR_GHOST_RED, COLOR_BLACK, 1);
    snprintf(buf, sizeof(buf), "D:%u", st->dots_eaten);
    display_write_text(210, BOTTOM_BAR_Y + 1, buf, COLOR_DOT_WHITE, COLOR_BLACK, 1);
    snprintf(buf, sizeof(buf), "SCORE:%u", st->score);
    display_write_text(172, BOTTOM_BAR_Y + 12, buf, COLOR_PACMAN_YELLOW, COLOR_BLACK, 1);
    snprintf(buf, sizeof(buf), "WPM:%u", st->current_wpm);
    display_write_text(258, BOTTOM_BAR_Y + 1, buf, COLOR_WHITE, COLOR_BLACK, 1);
    snprintf(buf, sizeof(buf), "PEAK:%u", st->peak_wpm);
    display_write_text(258, BOTTOM_BAR_Y + 12, buf, COLOR_GRAY, COLOR_BLACK, 1);
}

void pacman_status_render(pacman_status_t *st) {
    if (!st->dirty || !st->initialized) return;
    render_top_bar(st);
    render_zone(st);
    render_bottom_bar(st);
    display_flush();    /* single flush per frame — no LVGL allocations during drawing */
    st->dirty = false;
}
