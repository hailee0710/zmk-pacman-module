/*
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 *
 * Pacman Status Widget — landscape 320×172
 *
 * Each dot = one keypress. Ghost if WPM ≥ 80.
 * Single-row horizontal dot flow toward big, stationary Pacman.
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
    if (wpm > 80) wpm = 80;
    if (wpm == 0)  return DOT_SPEED_MIN;
    return DOT_SPEED_MIN + (uint8_t)((uint16_t)wpm * (DOT_SPEED_MAX - DOT_SPEED_MIN) / 80);
}

static bool is_ghost_zone(uint8_t wpm) { return wpm >= WPM_GHOST_THRESHOLD; }

static uint16_t batt_color(uint8_t pct) {
    if (pct > 60) return COLOR_GREEN;
    if (pct > 25) return COLOR_BLUE;
    return COLOR_RED;
}

/* Vertical battery icon at (x, y).
 * Body w=14 h=20, nip w=6 h=3 on top.
 * Fill from bottom up based on percentage.
 * pct==0: empty outline with diagonal strike (not connected). */
static void draw_battery_vertical(uint16_t x, uint16_t y, uint8_t pct) {
    uint16_t bw = 14, bh = 20, nip_w = 6, nip_h = 3;
    uint16_t nip_x = x + (bw - nip_w) / 2;

    if (pct > 0) {
        uint16_t outline = COLOR_WHITE;
        /* Nip */
        display_fill_rect(nip_x, y, nip_w, nip_h + 1, outline);
        /* Body outline */
        display_draw_rect(x, y + nip_h, bw + 1, bh + 1, outline);
        /* Fill from bottom — interior is (bw-1)×(bh-1) inside 1px border */
        uint8_t cpct = pct > 100 ? 100 : pct;
        uint16_t iw = bw - 1, ih = bh - 1;
        uint16_t fh = (uint16_t)ih * cpct / 100;
        if (fh < 1 && cpct > 0) fh = 1;
        uint16_t fill_color = batt_color(pct);
        display_fill_rect(x + 1, y + nip_h + 1 + ih - fh, iw, fh, fill_color);
    } else {
        uint16_t outline = COLOR_ORANGE;
        /* Nip */
        display_draw_rect(nip_x, y, nip_w, nip_h + 1, outline);
        /* Body outline */
        display_draw_rect(x, y + nip_h, bw + 1, bh + 1, outline);
        /* Diagonal strike */
        display_draw_line(x + 1, y + nip_h + 1, x + bw, y + nip_h + bh, COLOR_ORANGE);
    }
}

/* Find a free dot slot, or recycle the leftmost (oldest, closest to being
 * eaten) one. Recycling the rightmost/newest dot instead — as this used
 * to — would teleport the dot that just spawned back to the spawn point,
 * visibly stalling the flow at high WPM once all slots are busy. */
static int8_t find_slot(pacman_status_t *st) {
    for (int i = 0; i < DOT_MAX_COUNT; i++)
        if (!st->dots[i].active) return i;
    int16_t best = INT16_MAX; int8_t best_i = -1;
    for (int i = 0; i < DOT_MAX_COUNT; i++)
        if (st->dots[i].active && st->dots[i].x < best) { best = st->dots[i].x; best_i = i; }
    return best_i;
}

static void spawn(pacman_status_t *st) {
    int8_t idx = find_slot(st);
    if (idx < 0) return;
    pacman_dot_t *d = &st->dots[idx];

    /* Find rightmost active dot to maintain consistent spacing.
     * Cap the search window so dots never cascade unboundedly
     * off-screen — without the cap, sustained typing at certain
     * speeds (notably WPM 28-30 where speed=2 and spawn interval
     * is tight enough that each dot lands ~2px right of the last)
     * pushes the spawn point hundreds of pixels past the right
     * edge, making new dots invisible for seconds. */
    int16_t rightmost = DOT_SPAWN_X - DOT_SPACING;
    int16_t cap       = DOT_SPAWN_X + DOT_SPACING * 2;  /* 382 — 62px off-screen */
    for (int i = 0; i < DOT_MAX_COUNT; i++) {
        if (st->dots[i].active && st->dots[i].x > rightmost) {
            rightmost = st->dots[i].x;
        }
    }
    if (rightmost > cap) rightmost = cap;

    d->active   = true;
    d->is_ghost = is_ghost_zone(st->current_wpm);
    d->x        = rightmost + DOT_SPACING;
    if (d->x < DOT_SPAWN_X) d->x = DOT_SPAWN_X;
    d->speed    = 0; /* unused — global speed drives all dots equally */
}

/* ---- Public API ---- */

void pacman_status_init(pacman_status_t *st, const struct device *dev) {
    memset(st, 0, sizeof(*st));
    st->dev          = dev;
    st->mouth_frame  = 2;
    st->mouth_delta  = 1;
    strcpy(st->layer_name, "L0");
    st->initialized  = true;
}

void pacman_status_set_wpm(pacman_status_t *st, uint8_t wpm) { st->current_wpm = wpm; }

void pacman_status_set_host_connection(pacman_status_t *st, bool c, uint8_t t) {
    st->host_connected = c; st->host_transport = t; st->dirty_zones |= DIRTY_TOP;
}

void pacman_status_set_batteries(pacman_status_t *st, uint8_t l, uint8_t r) {
    st->left_battery = l; st->right_battery = r; st->dirty_zones |= DIRTY_BOTTOM;
}

void pacman_status_set_layer(pacman_status_t *st, const char *name) {
    strncpy(st->layer_name, name, sizeof(st->layer_name) - 1);
    st->layer_name[sizeof(st->layer_name) - 1] = '\0';
    st->dirty_zones |= DIRTY_TOP;
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

    /* Mouth animation */
    st->mouth_frame += st->mouth_delta;
    if (st->mouth_frame >= 3) { st->mouth_frame = 3; st->mouth_delta = -1; }
    else if (st->mouth_frame <= 0) { st->mouth_frame = 0; st->mouth_delta = 1; }

    bool any_dot_active = false;

    /* Move dots — all at same global speed driven by current WPM */
    uint8_t global_speed = wpm_to_speed(st->current_wpm);
    for (int i = 0; i < DOT_MAX_COUNT; i++) {
        if (!st->dots[i].active) continue;
        any_dot_active = true;
        pacman_dot_t *d = &st->dots[i];
        d->x -= global_speed;
        if (d->x <= DOT_EAT_X) {
            d->active = false;
        }
    }

    if (st->current_wpm > st->peak_wpm) st->peak_wpm = st->current_wpm;

    /* Per-zone dirty tracking: dots + mouth animate in the main zone,
     * WPM readout and bar live in the bottom zone. Top zone is only
     * touched by explicit setters (host, battery, layer). */
    if (any_dot_active || st->mouth_frame != prev_mouth) {
        st->dirty_zones |= DIRTY_MAIN;
    }
    if (st->current_wpm != last_wpm) {
        st->dirty_zones |= DIRTY_BOTTOM;
    }

    prev_mouth = st->mouth_frame;
    last_wpm = st->current_wpm;
}

/* ---- Rendering ---- */

static void render_top_bar(pacman_status_t *st) {
    display_fill_rect(0, 0, SCREEN_W, TOP_BAR_H, COLOR_BLACK);

    /* Layer name — top-left, scale 2, inset for curved corner.
     * Bar is 36px, text 14px tall at scale 2. Center: (36-14)/2 = 11. */
    display_write_text(12, 11, st->layer_name, COLOR_CYAN, COLOR_BLACK, 2, 1);

    /* Title — centered, scale 2, spaced letters.
     * 6 chars × 10px + 5 gaps × 4px = 80px. Center: (320-80)/2 = 120. */
    display_write_text(120, 11, "PACMAN", COLOR_PACMAN_YELLOW, COLOR_BLACK, 2, 4);

    /* Host connection — right side, scale 2, orange */
    if (st->host_connected) {
        const char *label;
        if (st->host_transport == 1) label = "USB";
        else                         label = "BLE";
        display_draw_filled_circle(252, 18, 5, COLOR_ORANGE);
        display_write_text(264, 11, label, COLOR_ORANGE, COLOR_BLACK, 2, 1);
    } else {
        display_draw_filled_circle(252, 18, 5, COLOR_RED);
        display_write_text(264, 11, "---", COLOR_RED, COLOR_BLACK, 2, 1);
    }

    display_draw_line(0, TOP_BAR_H - 1, SCREEN_W - 1, TOP_BAR_H - 1, COLOR_BLUE);
}

static void render_zone(pacman_status_t *st) {
    display_fill_rect(0, MAIN_ZONE_Y, SCREEN_W, MAIN_ZONE_H, COLOR_BLACK);

    /* Dots + ghosts flowing right-to-left */
    for (int i = 0; i < DOT_MAX_COUNT; i++) {
        if (!st->dots[i].active) continue;
        pacman_dot_t *d = &st->dots[i];
        if (d->is_ghost)
            display_draw_ghost(d->x - GHOST_W / 2, DOT_Y - GHOST_H / 2, GHOST_W, GHOST_H,
                               COLOR_GHOST_ORANGE, true);
        else
            display_draw_dot(d->x, DOT_Y, DOT_RADIUS, COLOR_DOT_WHITE);
    }

    /* Big Pacman */
    display_draw_pacman(PACMAN_CX, PACMAN_CY, PACMAN_RADIUS, DIR_RIGHT, st->mouth_frame,
                        COLOR_PACMAN_YELLOW);

    display_draw_line(0, BOTTOM_BAR_Y - 1, SCREEN_W - 1, BOTTOM_BAR_Y - 1, COLOR_BLUE);
}

static void render_bottom_bar(pacman_status_t *st) {
    char buf[8];
    display_fill_rect(0, BOTTOM_BAR_Y, SCREEN_W, BOTTOM_BAR_H, COLOR_BLACK);

    /* 4 equal areas × 80px, full-width edge to edge */
    uint16_t by = BOTTOM_BAR_Y;

    /* Thin light-gray dividers between areas */
    for (uint8_t i = 1; i < 4; i++) {
        uint16_t dx = i * 80;
        display_draw_line(dx, by + 4, dx, by + BOTTOM_BAR_H - 4, COLOR_BLUE);
    }

    /* Area 1 (0-79): left battery.
     * Icon 14px + gap 5px + text up to 40px (4 chars "100%" at scale 2) = 59px.
     * Center in 80px area: start at 10. */
    uint16_t batt_x1 = 10;
    uint16_t batt_y = by + 8;
    draw_battery_vertical(batt_x1, batt_y, st->left_battery);
    if (st->left_battery > 0) {
        snprintf(buf, sizeof(buf), "%u%%", st->left_battery);
        display_write_text(batt_x1 + 19, batt_y + 7, buf,
                           batt_color(st->left_battery), COLOR_BLACK, 2, 0);
    } else {
        display_write_text(batt_x1 + 19, batt_y + 7, "--",
                           COLOR_ORANGE, COLOR_BLACK, 2, 0);
    }

    /* Area 2 (80-159): right battery */
    uint16_t batt_x2 = 90;
    draw_battery_vertical(batt_x2, batt_y, st->right_battery);
    if (st->right_battery > 0) {
        snprintf(buf, sizeof(buf), "%u%%", st->right_battery);
        display_write_text(batt_x2 + 19, batt_y + 7, buf,
                           batt_color(st->right_battery), COLOR_BLACK, 2, 0);
    } else {
        display_write_text(batt_x2 + 19, batt_y + 7, "--",
                           COLOR_ORANGE, COLOR_BLACK, 2, 0);
    }

    /* Area 3 (160-239): WPM */
    display_write_text(193, by + 2, "WPM", COLOR_WHITE, COLOR_BLACK, 1, 0);
    snprintf(buf, sizeof(buf), "%u", st->current_wpm);
    display_write_text(185, by + 11, buf, COLOR_WHITE, COLOR_BLACK, 3, 0);

    /* Area 4 (240-319): PEAK */
    display_write_text(270, by + 2, "PEAK", COLOR_PINK, COLOR_BLACK, 1, 0);
    snprintf(buf, sizeof(buf), "%u", st->peak_wpm);
    display_write_text(258, by + 11, buf, COLOR_PINK, COLOR_BLACK, 3, 0);
}

void pacman_status_render(pacman_status_t *st) {
    if (!st->dirty_zones || !st->initialized) return;

    /* Render only the zones that changed, then invalidate just those
     * LVGL image objects so lv_refr_now() sends only the dirty rows
     * over SPI instead of the full 172-row framebuffer. */
    if (st->dirty_zones & DIRTY_TOP) {
        render_top_bar(st);
        display_inv_zone_top();
    }
    if (st->dirty_zones & DIRTY_MAIN) {
        render_zone(st);
        display_inv_zone_main();
    }
    if (st->dirty_zones & DIRTY_BOTTOM) {
        render_bottom_bar(st);
        display_inv_zone_bottom();
    }

    display_flush();
    st->dirty_zones = 0;
}
