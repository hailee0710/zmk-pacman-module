/*
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 *
 * Display drawing helpers for ST7789P3 320x172 landscape
 */

#pragma once

#include <zephyr/device.h>
#include <lvgl.h>

/* Display dimensions - landscape orientation */
#define DISPLAY_WIDTH  320
#define DISPLAY_HEIGHT 172
#define DISPLAY_W      320
#define DISPLAY_H      172

/* Color definitions (RGB565) */
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

/* ---- Drawing primitives ---- */
void display_fill(const struct device *dev, uint16_t color);
void display_fill_rect(const struct device *dev, uint16_t x1, uint16_t y1,
                       uint16_t x2, uint16_t y2, uint16_t color);
void display_draw_pixel(const struct device *dev, uint16_t x, uint16_t y,
                        uint16_t color);
void display_draw_rect(const struct device *dev, uint16_t x1, uint16_t y1,
                       uint16_t x2, uint16_t y2, uint16_t color);
void display_draw_line(const struct device *dev, uint16_t x1, uint16_t y1,
                       uint16_t x2, uint16_t y2, uint16_t color);
void display_draw_circle(const struct device *dev, uint16_t cx, uint16_t cy,
                         uint16_t radius, uint16_t color);
void display_draw_filled_circle(const struct device *dev, uint16_t cx, uint16_t cy,
                                uint16_t radius, uint16_t color);
void display_draw_filled_arc(const struct device *dev, uint16_t cx, uint16_t cy,
                             uint16_t radius, int16_t start_angle, int16_t end_angle,
                             uint16_t color);
void display_write_bitmap(const struct device *dev, uint16_t x, uint16_t y,
                          const uint16_t *bitmap, uint16_t w, uint16_t h);
void display_write_text(const struct device *dev, uint16_t x, uint16_t y,
                        const char *text, uint16_t color, uint16_t bg_color,
                        uint8_t scale);

/* ---- High-level drawing for Pacman theme ---- */
void display_draw_pacman(const struct device *dev, uint16_t cx, uint16_t cy,
                         uint16_t radius, uint8_t direction, uint8_t mouth_open,
                         uint16_t color);
void display_draw_ghost(const struct device *dev, uint16_t x, uint16_t y,
                        uint16_t w, uint16_t h, uint16_t color, bool eyes_left);
void display_draw_dot(const struct device *dev, uint16_t cx, uint16_t cy,
                      uint8_t radius, uint16_t color);
void display_draw_power_pellet(const struct device *dev, uint16_t cx, uint16_t cy,
                               uint8_t radius, uint16_t color, bool blink);

/* Direction constants */
#define DIR_RIGHT 0
#define DIR_DOWN  1
#define DIR_LEFT  2
#define DIR_UP    3

/* Get LVGL display device */
const struct device *display_get_device(void);
