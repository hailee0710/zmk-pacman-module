/*
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 *
 * Buzzer helper for sound effects
 */

#pragma once
#include <stdint.h>

void buzzer_init(void);
void buzzer_play_tone(uint16_t freq_hz, uint16_t duration_ms);
void buzzer_play_dot_eat(void);
void buzzer_play_ghost_eat(void);
void buzzer_play_power_up(void);
void buzzer_stop(void);
