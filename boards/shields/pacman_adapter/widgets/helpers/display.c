/*
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 *
 * Display drawing helpers for ST7789P3 320x172 landscape
 * Bitmap font rendering matching original snake module approach
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <math.h>

#include <lvgl.h>
#include "display.h"
#include "fonts.h"

LOG_MODULE_REGISTER(display_helpers, CONFIG_DISPLAY_LOG_LEVEL);

static lv_display_t *lvgl_display = NULL;

const struct device *display_get_device(void) {
    return (const struct device *)lvgl_display;
}

/* ---- LVGL-based drawing primitives ---- */

void display_fill(const struct device *dev, uint16_t color) {
    lv_obj_t *scr = lv_scr_act();
    lv_color_t c = lv_color_hex(color);
    lv_obj_set_style_bg_color(scr, c, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    lv_obj_t *rect = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(rect);
    lv_obj_set_size(rect, DISPLAY_W, DISPLAY_H);
    lv_obj_set_pos(rect, 0, 0);
    lv_obj_set_style_bg_color(rect, c, 0);
    lv_obj_set_style_bg_opa(rect, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(rect, 0, 0);
    lv_obj_set_style_radius(rect, 0, 0);
    lv_refr_now(lvgl_display);
    lv_obj_delete(rect);
}

void display_fill_rect(const struct device *dev, uint16_t x1, uint16_t y1,
                       uint16_t x2, uint16_t y2, uint16_t color) {
    if (x2 < x1 || y2 < y1) return;
    lv_obj_t *rect = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(rect);
    lv_obj_set_size(rect, x2 - x1 + 1, y2 - y1 + 1);
    lv_obj_set_pos(rect, x1, y1);
    lv_obj_set_style_bg_color(rect, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(rect, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(rect, 0, 0);
    lv_obj_set_style_radius(rect, 0, 0);
    lv_refr_now(lvgl_display);
    lv_obj_delete(rect);
}

void display_draw_pixel(const struct device *dev, uint16_t x, uint16_t y,
                        uint16_t color) {
    display_fill_rect(dev, x, y, x, y, color);
}

void display_draw_rect(const struct device *dev, uint16_t x1, uint16_t y1,
                       uint16_t x2, uint16_t y2, uint16_t color) {
    lv_obj_t *rect = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(rect);
    lv_obj_set_size(rect, x2 - x1 + 1, y2 - y1 + 1);
    lv_obj_set_pos(rect, x1, y1);
    lv_obj_set_style_bg_opa(rect, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(rect, lv_color_hex(color), 0);
    lv_obj_set_style_border_width(rect, 1, 0);
    lv_obj_set_style_border_opa(rect, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(rect, 0, 0);
    lv_refr_now(lvgl_display);
    lv_obj_delete(rect);
}

void display_draw_line(const struct device *dev, uint16_t x1, uint16_t y1,
                       uint16_t x2, uint16_t y2, uint16_t color) {
    int16_t dx = abs((int16_t)x2 - (int16_t)x1);
    int16_t dy = -abs((int16_t)y2 - (int16_t)y1);
    int16_t sx = x1 < x2 ? 1 : -1;
    int16_t sy = y1 < y2 ? 1 : -1;
    int16_t err = dx + dy;
    while (1) {
        display_draw_pixel(dev, x1, y1, color);
        if (x1 == x2 && y1 == y2) break;
        int16_t e2 = 2 * err;
        if (e2 >= dy) { err += dy; x1 += sx; }
        if (e2 <= dx) { err += dx; y1 += sy; }
    }
}

void display_draw_circle(const struct device *dev, uint16_t cx, uint16_t cy,
                         uint16_t radius, uint16_t color) {
    int16_t x = radius, y = 0, err = 0;
    while (x >= y) {
        display_draw_pixel(dev, cx + x, cy + y, color);
        display_draw_pixel(dev, cx + y, cy + x, color);
        display_draw_pixel(dev, cx - y, cy + x, color);
        display_draw_pixel(dev, cx - x, cy + y, color);
        display_draw_pixel(dev, cx - x, cy - y, color);
        display_draw_pixel(dev, cx - y, cy - x, color);
        display_draw_pixel(dev, cx + y, cy - x, color);
        display_draw_pixel(dev, cx + x, cy - y, color);
        y += 1; err += 1 + 2 * y;
        if (2 * (err - x) + 1 > 0) { x -= 1; err += 1 - 2 * x; }
    }
}

void display_draw_filled_circle(const struct device *dev, uint16_t cx, uint16_t cy,
                                uint16_t radius, uint16_t color) {
    int16_t x = radius, y = 0, err = 0;
    while (x >= y) {
        display_draw_line(dev, cx - x, cy + y, cx + x, cy + y, color);
        display_draw_line(dev, cx - y, cy + x, cx + y, cy + x, color);
        display_draw_line(dev, cx - x, cy - y, cx + x, cy - y, color);
        display_draw_line(dev, cx - y, cy - x, cx + y, cy - x, color);
        y += 1; err += 1 + 2 * y;
        if (2 * (err - x) + 1 > 0) { x -= 1; err += 1 - 2 * x; }
    }
}

void display_draw_filled_arc(const struct device *dev, uint16_t cx, uint16_t cy,
                             uint16_t radius, int16_t start_angle, int16_t end_angle,
                             uint16_t color) {
    while (start_angle < 0) start_angle += 360;
    while (start_angle >= 360) start_angle -= 360;
    while (end_angle < 0) end_angle += 360;
    while (end_angle >= 360) end_angle -= 360;
    for (int16_t dy = -radius; dy <= (int16_t)radius; dy++) {
        for (int16_t dx = -radius; dx <= (int16_t)radius; dx++) {
            if (dx * dx + dy * dy <= (int16_t)(radius * radius)) {
                int16_t angle = (int16_t)(atan2f((float)dy, (float)dx) * 180.0f / 3.14159265f);
                if (angle < 0) angle += 360;
                bool in_arc;
                if (start_angle <= end_angle)
                    in_arc = (angle >= start_angle && angle <= end_angle);
                else
                    in_arc = (angle >= start_angle || angle <= end_angle);
                if (in_arc)
                    display_draw_pixel(dev, cx + dx, cy + dy, color);
            }
        }
    }
}

void display_write_bitmap(const struct device *dev, uint16_t x, uint16_t y,
                          const uint16_t *bitmap, uint16_t w, uint16_t h) {
    for (uint16_t row = 0; row < h; row++)
        for (uint16_t col = 0; col < w; col++)
            display_draw_pixel(dev, x + col, y + row, bitmap[row * w + col]);
}

/* ---- Bitmap font text rendering (matches snake module approach) ---- */

void display_write_text(const struct device *dev, uint16_t x, uint16_t y,
                        const char *text, uint16_t color, uint16_t bg_color,
                        uint8_t scale) {
    if (text == NULL || scale == 0) return;

    uint16_t cursor_x = x;
    uint8_t len = (uint8_t)strlen(text);

    for (uint8_t i = 0; i < len; i++) {
        Character c = char_to_enum(text[i]);
        const uint16_t *bitmap = get_bitmap_5x7(c);

        /* Render each row of the scaled character */
        for (uint8_t row = 0; row < FONT_5x7_H; row++) {
            for (uint8_t sr = 0; sr < scale; sr++) {
                uint8_t run_start = 0;
                bool in_run = false;
                bool run_is_fg = false;

                for (uint8_t col = 0; col < FONT_5x7_W; col++) {
                    bool is_fg = (bitmap[row * FONT_5x7_W + col] != 0);

                    if (!in_run) {
                        run_start = col;
                        run_is_fg = is_fg;
                        in_run = true;
                    } else if (is_fg != run_is_fg) {
                        /* Flush previous run */
                        uint16_t rx = cursor_x + run_start * scale;
                        uint16_t ry = y + (row * scale) + sr;
                        uint16_t rw = (col - run_start) * scale;
                        display_fill_rect(dev, rx, ry, rx + rw - 1, ry,
                                          run_is_fg ? color : bg_color);
                        run_start = col;
                        run_is_fg = is_fg;
                    }
                }
                /* Flush final run */
                if (in_run) {
                    uint16_t rx = cursor_x + run_start * scale;
                    uint16_t ry = y + (row * scale) + sr;
                    uint16_t rw = (FONT_5x7_W - run_start) * scale;
                    display_fill_rect(dev, rx, ry, rx + rw - 1, ry,
                                      run_is_fg ? color : bg_color);
                }
            }
        }

        cursor_x += FONT_5x7_W * scale;
    }
}

/* ---- Pacman / Ghost / Dot drawing ---- */

void display_draw_pacman(const struct device *dev, uint16_t cx, uint16_t cy,
                         uint16_t radius, uint8_t direction, uint8_t mouth_open,
                         uint16_t color) {
    uint16_t mouth_angle;
    switch (mouth_open) {
        case 0: mouth_angle = 5;  break;
        case 1: mouth_angle = 15; break;
        case 2: mouth_angle = 30; break;
        case 3: mouth_angle = 45; break;
        default:mouth_angle = 30; break;
    }
    int16_t sa, ea;
    switch (direction) {
        case DIR_RIGHT: sa = mouth_angle;     ea = 360 - mouth_angle; break;
        case DIR_DOWN:  sa = 90 + mouth_angle; ea = 90 - mouth_angle;
                        if (ea < 0) ea += 360; break;
        case DIR_LEFT:  sa = 180 + mouth_angle; ea = 180 - mouth_angle; break;
        case DIR_UP:    sa = 270 + mouth_angle; ea = 270 - mouth_angle;
                        if (ea < 0) ea += 360; break;
        default:        sa = mouth_angle;     ea = 360 - mouth_angle; break;
    }
    display_draw_filled_arc(dev, cx, cy, radius, sa, ea, color);
}

void display_draw_ghost(const struct device *dev, uint16_t x, uint16_t y,
                        uint16_t w, uint16_t h, uint16_t color, bool eyes_left) {
    uint16_t r = w / 2;
    uint16_t bh = h - 4;
    display_draw_filled_circle(dev, x + r, y + r, r, color);
    display_fill_rect(dev, x, y + r, x + w - 1, y + bh, color);
    uint16_t sy = y + bh;
    uint16_t ww = w / 3;
    for (int i = 0; i < 3; i++)
        display_draw_filled_circle(dev, x + i * ww + ww / 2, sy + 2, ww / 2, color);
    uint16_t ey = y + r - 2;
    uint16_t lex = x + r - 3, rex = x + r + 3;
    display_draw_filled_circle(dev, lex, ey, 3, COLOR_WHITE);
    display_draw_filled_circle(dev, rex, ey, 3, COLOR_WHITE);
    uint16_t po = eyes_left ? -1 : 1;
    display_draw_filled_circle(dev, lex + po, ey, 1, COLOR_BLUE);
    display_draw_filled_circle(dev, rex + po, ey, 1, COLOR_BLUE);
}

void display_draw_dot(const struct device *dev, uint16_t cx, uint16_t cy,
                      uint8_t radius, uint16_t color) {
    display_draw_filled_circle(dev, cx, cy, radius, color);
}

void display_draw_power_pellet(const struct device *dev, uint16_t cx, uint16_t cy,
                               uint8_t radius, uint16_t color, bool blink) {
    if (blink)
        display_draw_circle(dev, cx, cy, radius, color);
    else
        display_draw_filled_circle(dev, cx, cy, radius, color);
}

/* ---- Init ---- */

static int display_helpers_init(void) {
    lvgl_display = lv_display_get_default();
    if (lvgl_display == NULL) {
        LOG_ERR("No default LVGL display found");
        return -ENODEV;
    }
    LOG_INF("Display helpers ready (%dx%d), bitmap font renderer", DISPLAY_W, DISPLAY_H);
    return 0;
}

SYS_INIT(display_helpers_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
