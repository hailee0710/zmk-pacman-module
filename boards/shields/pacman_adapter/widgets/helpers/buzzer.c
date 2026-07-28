/*
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 *
 * Buzzer helper implementation - stub for when no buzzer is present
 */

#include <zephyr/kernel.h>
#include "buzzer.h"

void buzzer_init(void) {
    /* No buzzer configured */
}

void buzzer_play_tone(uint16_t freq_hz, uint16_t duration_ms) {
    /* No-op */
}

void buzzer_play_dot_eat(void) {
    /* No-op */
}

void buzzer_play_ghost_eat(void) {
    /* No-op */
}

void buzzer_play_power_up(void) {
    /* No-op */
}

void buzzer_stop(void) {
    /* No-op */
}
