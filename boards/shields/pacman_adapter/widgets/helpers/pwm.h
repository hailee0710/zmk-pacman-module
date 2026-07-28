/*
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 *
 * PWM helper for buzzer/LED control
 */

#pragma once
#include <stdint.h>

void pwm_init(void);
void pwm_set_duty(uint8_t channel, uint16_t duty);
void pwm_set_frequency(uint16_t freq_hz);
void pwm_start(void);
void pwm_stop(void);
