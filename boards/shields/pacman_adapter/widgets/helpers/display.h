/*
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 *
 * Display drawing helpers — framebuffer-based rendering
 * All drawing writes to a full-screen buffer; flush() sends it once per frame.
 * Zero LVGL object allocations during rendering.
 */

#pragma once

#include <zephyr/device.h>
#include <stdbool.h>

/* Display dimensions */
#define DISPLAY_W 320
#define DISPLAY_H 172

/* Pixelwave palette — converted to RGB565 */
/*   #060026 → 0x0004   dark navy       */
/*   #fff056 → 0xFF8A   bright yellow   */
/*   #0170fe → 0x039F   bright blue     */
/*   #00125e → 0x008B   dark blue       */
/*   #ffd156 → 0xFE8A   warm yellow     */
/*   #090038 → 0x0807   deep purple     */
/*   #ff9156 → 0xFC8A   orange          */
/*   #ff5d56 → 0xFAEA   coral red       */
/*   #ff549d → 0xFAB3   hot pink        */

/* Backgrounds */
#define COLOR_BLACK         0x0807  /* deep purple bg */
#define COLOR_NAVY          0x0004  /* dark navy alt  */
#define COLOR_DARK_GRAY     0x008B  /* dark blue      */
#define COLOR_GRAY          0x008B  /* dark blue      */
#define COLOR_MAZE_BLUE     0x008B  /* dark blue      */

/* Text / bright elements */
#define COLOR_WHITE         0xFF8A  /* bright yellow  */
#define COLOR_YELLOW        0xFE8A  /* warm yellow    */
#define COLOR_LIGHT_GRAY    0x52AA  /* dim gray #555  */
#define COLOR_PACMAN_YELLOW 0xFF8A  /* bright yellow  */
#define COLOR_DOT_WHITE     0xFFFF  /* pure white     */
#define COLOR_POWER_PELLET  0xFE8A  /* warm yellow    */

/* Accents */
#define COLOR_RED           0xFAEA  /* coral red      */
#define COLOR_BLUE          0x039F  /* bright blue    */
#define COLOR_CYAN          0x039F  /* bright blue    */
#define COLOR_GREEN         0x0660  /* green #0c0    */
#define COLOR_ORANGE        0xFC8A  /* orange         */
#define COLOR_PINK          0xFAB3  /* hot pink       */
#define COLOR_MAGENTA       0xFAB3  /* hot pink       */
#define COLOR_PURPLE        0xFAB3  /* hot pink       */

/* Ghosts */
#define COLOR_GHOST_RED     0xFAEA  /* coral red      */
#define COLOR_GHOST_PINK    0xFAB3  /* hot pink       */
#define COLOR_GHOST_CYAN    0x039F  /* bright blue    */
#define COLOR_GHOST_ORANGE  0xFC8A  /* orange         */

/* Direction constants */
#define DIR_RIGHT 0
#define DIR_DOWN  1
#define DIR_LEFT  2
#define DIR_UP    3

/* ---- Drawing primitives (write to framebuffer) ----
 * Coordinates are signed: shapes are frequently positioned via center-minus-
 * radius arithmetic (e.g. cx - r) that legitimately goes negative for
 * anything near the left/top edge, and that needs a real sign to clip
 * correctly instead of wrapping around through an unsigned type. Sizes
 * (w/h/radius) stay unsigned — they're never negative. */
void display_fill(uint16_t color);
void display_fill_rect(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t color);
void display_draw_pixel(int16_t x, int16_t y, uint16_t color);
void display_draw_rect(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t color);
void display_draw_line(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);
void display_draw_circle(int16_t cx, int16_t cy, uint16_t r, uint16_t color);
void display_draw_filled_circle(int16_t cx, int16_t cy, uint16_t r, uint16_t color);
void display_write_text(int16_t x, int16_t y, const char *text,
                        uint16_t color, uint16_t bg_color, uint8_t scale,
                        uint8_t letter_spacing);

/* ---- High-level shapes ---- */
void display_draw_pacman(int16_t cx, int16_t cy, uint16_t radius,
                         uint8_t direction, uint8_t mouth_open, uint16_t color);
void display_draw_ghost(int16_t x, int16_t y, uint16_t w, uint16_t h,
                        uint16_t color, bool eyes_left);
void display_draw_dot(int16_t cx, int16_t cy, uint8_t radius, uint16_t color);
void display_draw_power_pellet(int16_t cx, int16_t cy, uint8_t radius,
                               uint16_t color, bool blink);

/* ---- Frame management ----
 * The screen is divided into three horizontal zones matching the Pacman
 * layout (top bar, main animation zone, bottom bar). Each zone gets its
 * own LVGL image object pointing into the shared framebuffer so that a
 * zone can be invalidated and flushed independently — when only the WPM
 * readout changes, only the 24-row bottom bar (14 % of the screen) goes
 * over SPI instead of the full 172-row framebuffer.
 *
 * Kept in sync with pacman.h: TOP_BAR_H=36, MAIN_ZONE_H=96, BOTTOM_BAR_H=40 */
#define DISPLAY_ZONE_TOP_Y     0
#define DISPLAY_ZONE_TOP_H    36
#define DISPLAY_ZONE_MAIN_Y   36
#define DISPLAY_ZONE_MAIN_H   96
#define DISPLAY_ZONE_BOTTOM_Y 132
#define DISPLAY_ZONE_BOTTOM_H  40

void display_begin_frame(void);
void display_flush(void);
void display_inv_zone_top(void);
void display_inv_zone_main(void);
void display_inv_zone_bottom(void);

/* Init the framebuffer and bind to the display device */
int display_init(const struct device *display_dev);
