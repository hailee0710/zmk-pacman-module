/*
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 *
 * WPM (Words Per Minute) tracker implementation
 * Uses a sliding window of keypress timestamps
 */

#include <zephyr/kernel.h>
#include <string.h>
#include <stdlib.h>

#include "wpm.h"

void wpm_init(wpm_state_t *st) {
    memset(st, 0, sizeof(*st));
}

void wpm_key_pressed(wpm_state_t *st) {
    /* Record timestamp of this keypress */
    st->key_timestamps[st->key_index] = k_uptime_get_32();
    st->key_index = (st->key_index + 1) % 64;
    if (st->key_count < 64) st->key_count++;
}

void wpm_tick(wpm_state_t *st) {
    uint32_t now = k_uptime_get_32();

    /* Calculate how many keypresses in the last WPM_WINDOW_MS */
    uint16_t recent_keys = 0;
    for (int i = 0; i < st->key_count; i++) {
        if (now - st->key_timestamps[i] < WPM_WINDOW_MS) {
            recent_keys++;
        }
    }

    /* Convert to WPM: (keys / chars_per_word) * (60000 / window_ms) */
    if (recent_keys > 0) {
        uint16_t wpm = (uint16_t)((uint32_t)recent_keys * 60000 /
                                   (WPM_CHARS_PER_WORD * WPM_WINDOW_MS));
        if (wpm > 255) wpm = 255;
        st->current_wpm = (uint8_t)wpm;
    } else {
        /* Decay WPM when no typing */
        if (st->current_wpm > 0) {
            st->current_wpm--;
        }
    }

    if (st->current_wpm > st->peak_wpm) {
        st->peak_wpm = st->current_wpm;
    }

    st->last_calc_time = now;
}

uint8_t wpm_get_current(wpm_state_t *st) {
    return st->current_wpm;
}

uint8_t wpm_get_peak(wpm_state_t *st) {
    return st->peak_wpm;
}

void wpm_reset(wpm_state_t *st) {
    st->key_count = 0;
    st->key_index = 0;
    st->current_wpm = 0;
}
