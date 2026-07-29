/*
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 *
 * Bitmap fonts — ported from snake-module by joaopedropio
 *
 * Each character is a uint16_t array where:
 *   0 = background (transparent / bg_color)
 *   1 = foreground (text color)
 *
 * Available sizes: 3x5, 5x7 (scalable via the scale parameter)
 */

#pragma once

#include <stdint.h>

/* ---- Character enum ---- */
typedef enum {
    CHAR_0 = 0, CHAR_1, CHAR_2, CHAR_3, CHAR_4,
    CHAR_5, CHAR_6, CHAR_7, CHAR_8, CHAR_9,
    CHAR_A, CHAR_B, CHAR_C, CHAR_D, CHAR_E, CHAR_F,
    CHAR_G, CHAR_H, CHAR_I, CHAR_J, CHAR_K, CHAR_L,
    CHAR_M, CHAR_N, CHAR_O, CHAR_P, CHAR_Q, CHAR_R,
    CHAR_S, CHAR_T, CHAR_U, CHAR_V, CHAR_W, CHAR_X,
    CHAR_Y, CHAR_Z,
    CHAR_COLON, CHAR_DASH, CHAR_PERCENTAGE, CHAR_DOT,
    CHAR_EMPTY, CHAR_NONE,
} Character;

/* Convert ASCII char to Character enum */
Character char_to_enum(char ch);

/* ---- 3×5 font (all letters A-Z, digits 0-9, symbols) ---- */

#define FONT_3x5_W 3
#define FONT_3x5_H 5
/* Digit bitmaps — access via get_bitmap_3x5() below */
extern const uint16_t *num_bitmaps_3x5[10];

/* ---- 5×7 font (digits 0-9, letters used for labels) ---- */

#define FONT_5x7_W 5
#define FONT_5x7_H 7
/* Letter/digit bitmaps — access via get_bitmap_5x7() / get_bitmap_3x5() below */
extern const uint16_t *num_bitmaps_5x7[10];

/* Lookup: given a Character enum, return the 5x7 bitmap */
const uint16_t *get_bitmap_5x7(Character c);

/* Lookup: given a Character enum, return the 3x5 bitmap */
const uint16_t *get_bitmap_3x5(Character c);
