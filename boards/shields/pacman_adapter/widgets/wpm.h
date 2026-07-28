/*
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 *
 * WPM (Words Per Minute) tracker widget
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

/* WPM calculation window (ms) */
#define WPM_WINDOW_MS  5000
#define WPM_CHARS_PER_WORD 5

typedef struct {
    uint32_t key_timestamps[64];  /* Circular buffer of key press timestamps */
    uint8_t key_count;
    uint8_t key_index;
    uint8_t current_wpm;
    uint8_t peak_wpm;
    uint32_t last_calc_time;
} wpm_state_t;

void wpm_init(wpm_state_t *st);
void wpm_key_pressed(wpm_state_t *st);
void wpm_tick(wpm_state_t *st);
uint8_t wpm_get_current(wpm_state_t *st);
uint8_t wpm_get_peak(wpm_state_t *st);
void wpm_reset(wpm_state_t *st);
