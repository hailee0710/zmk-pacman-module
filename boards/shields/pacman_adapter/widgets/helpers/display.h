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

/* RGB565 colors */
#define COLOR_BLACK         0x0000
#define COLOR_WHITE         0xFFFF
#define COLOR_RED           0xF800
#define COLOR_GREEN         0x07E0
#define COLOR_BLUE          0x001F
#define COLOR_YELLOW        0xFFE0
#define COLOR_CYAN          0x07FF
#define COLOR_MAGENTA       0xF81F
#define COLOR_ORANGE        0xFD20
#define COLOR_PINK          0xFC18
#define COLOR_PURPLE        0x8010
#define COLOR_GRAY          0x8410
#define COLOR_DARK_GRAY     0x4208
#define COLOR_LIGHT_GRAY    0xC618
#define COLOR_NAVY          0x0010
#define COLOR_PACMAN_YELLOW 0xFFE0
#define COLOR_GHOST_RED     0xF800
#define COLOR_GHOST_PINK    0xFC18
#define COLOR_GHOST_CYAN    0x07FF
#define COLOR_GHOST_ORANGE  0xFD20
#define COLOR_DOT_WHITE     0xFFFF
#define COLOR_POWER_PELLET  0xFFE0
#define COLOR_MAZE_BLUE     0x001F

/* Direction constants */
#define DIR_RIGHT 0
#define DIR_DOWN  1
#define DIR_LEFT  2
#define DIR_UP    3

/* ---- Drawing primitives (write to framebuffer) ---- */
void display_fill(uint16_t color);
void display_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void display_draw_pixel(uint16_t x, uint16_t y, uint16_t color);
void display_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void display_draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void display_draw_circle(uint16_t cx, uint16_t cy, uint16_t r, uint16_t color);
void display_draw_filled_circle(uint16_t cx, uint16_t cy, uint16_t r, uint16_t color);
void display_write_text(uint16_t x, uint16_t y, const char *text,
                        uint16_t color, uint16_t bg_color, uint8_t scale);

/* ---- High-level shapes ---- */
void display_draw_pacman(uint16_t cx, uint16_t cy, uint16_t radius,
                         uint8_t direction, uint8_t mouth_open, uint16_t color);
void display_draw_ghost(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                        uint16_t color, bool eyes_left);
void display_draw_dot(uint16_t cx, uint16_t cy, uint8_t radius, uint16_t color);
void display_draw_power_pellet(uint16_t cx, uint16_t cy, uint8_t radius,
                               uint16_t color, bool blink);

/* ---- Frame management ---- */
void display_begin_frame(void);
void display_flush(void);

/* Init the framebuffer and bind to the display device */
int display_init(const struct device *display_dev);
