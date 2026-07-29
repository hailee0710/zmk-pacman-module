/*
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 *
 * Display drawing helpers — framebuffer-based rendering
 *
 * All drawing functions write to a static full-screen framebuffer.
 * display_flush() sends the entire framebuffer to the display
 * in one LVGL operation. Zero heap allocations during rendering.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <math.h>
#include <lvgl.h>

#include "display.h"
#include "fonts.h"

LOG_MODULE_REGISTER(display_helpers, CONFIG_DISPLAY_LOG_LEVEL);

/* Full-screen framebuffer (320 × 172 × 2 bytes = 110,080 bytes) */
static uint16_t fb[DISPLAY_W * DISPLAY_H];
static const struct device *disp_dev;
static lv_display_t *lvgl_display;

/* ---- Internal: fast horizontal line in framebuffer ---- */
static inline void fb_hline(uint16_t x, uint16_t y, uint16_t w, uint16_t color) {
    if (x >= DISPLAY_W || y >= DISPLAY_H) return;
    if (x + w > DISPLAY_W) w = DISPLAY_W - x;
    uint16_t *dst = &fb[y * DISPLAY_W + x];
    for (uint16_t i = 0; i < w; i++) dst[i] = color;
}

static inline void fb_pixel(uint16_t x, uint16_t y, uint16_t color) {
    if (x < DISPLAY_W && y < DISPLAY_H)
        fb[y * DISPLAY_W + x] = color;
}

/* ---- Init ---- */
int display_init(const struct device *dev) {
    disp_dev = dev;
    lvgl_display = lv_display_get_default();
    if (!lvgl_display) {
        LOG_ERR("No LVGL display found");
        return -ENODEV;
    }
    memset(fb, 0, sizeof(fb));
    LOG_INF("Framebuffer ready (%dx%d, %u bytes)", DISPLAY_W, DISPLAY_H, (unsigned)sizeof(fb));
    return 0;
}

void display_begin_frame(void) {
    /* Optionally clear — caller can draw background manually */
}

/* ---- Drawing primitives ---- */

void display_fill(uint16_t color) {
    for (uint32_t i = 0; i < DISPLAY_W * DISPLAY_H; i++) fb[i] = color;
}

void display_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    if (x >= DISPLAY_W || y >= DISPLAY_H) return;
    if (x + w > DISPLAY_W) w = DISPLAY_W - x;
    if (y + h > DISPLAY_H) h = DISPLAY_H - y;
    for (uint16_t row = 0; row < h; row++)
        fb_hline(x, y + row, w, color);
}

void display_draw_pixel(uint16_t x, uint16_t y, uint16_t color) {
    fb_pixel(x, y, color);
}

void display_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    fb_hline(x, y, w, color);                    /* top */
    fb_hline(x, y + h - 1, w, color);            /* bottom */
    for (uint16_t i = 0; i < h; i++) {           /* left + right */
        fb_pixel(x, y + i, color);
        fb_pixel(x + w - 1, y + i, color);
    }
}

void display_draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) {
    int16_t dx = abs((int16_t)x2 - (int16_t)x1), sx = x1 < x2 ? 1 : -1;
    int16_t dy = -abs((int16_t)y2 - (int16_t)y1), sy = y1 < y2 ? 1 : -1;
    int16_t err = dx + dy;
    while (1) {
        fb_pixel(x1, y1, color);
        if (x1 == x2 && y1 == y2) break;
        int16_t e2 = 2 * err;
        if (e2 >= dy) { err += dy; x1 += sx; }
        if (e2 <= dx) { err += dx; y1 += sy; }
    }
}

void display_draw_circle(uint16_t cx, uint16_t cy, uint16_t r, uint16_t color) {
    int16_t x = r, y = 0, err = 0;
    while (x >= y) {
        fb_pixel(cx + x, cy + y, color); fb_pixel(cx + y, cy + x, color);
        fb_pixel(cx - y, cy + x, color); fb_pixel(cx - x, cy + y, color);
        fb_pixel(cx - x, cy - y, color); fb_pixel(cx - y, cy - x, color);
        fb_pixel(cx + y, cy - x, color); fb_pixel(cx + x, cy - y, color);
        y++; err += 1 + 2 * y;
        if (2 * (err - x) + 1 > 0) { x--; err += 1 - 2 * x; }
    }
}

void display_draw_filled_circle(uint16_t cx, uint16_t cy, uint16_t r, uint16_t color) {
    int16_t x = r, y = 0, err = 0;
    while (x >= y) {
        display_draw_line(cx - x, cy + y, cx + x, cy + y, color);
        display_draw_line(cx - y, cy + x, cx + y, cy + x, color);
        display_draw_line(cx - x, cy - y, cx + x, cy - y, color);
        display_draw_line(cx - y, cy - x, cx + y, cy - x, color);
        y++; err += 1 + 2 * y;
        if (2 * (err - x) + 1 > 0) { x--; err += 1 - 2 * x; }
    }
}

/* ---- Bitmap text rendering ---- */

void display_write_text(uint16_t x, uint16_t y, const char *text,
                        uint16_t color, uint16_t bg_color, uint8_t scale) {
    if (!text || scale == 0) return;
    uint16_t cx = x;
    uint8_t len = (uint8_t)strlen(text);
    for (uint8_t i = 0; i < len; i++) {
        Character c = char_to_enum(text[i]);
        const uint16_t *bm = get_bitmap_5x7(c);
        for (uint8_t row = 0; row < FONT_5x7_H; row++) {
            for (uint8_t sr = 0; sr < scale; sr++) {
                uint8_t run_start = 0;
                bool in_run = false, run_fg = false;
                for (uint8_t col = 0; col < FONT_5x7_W; col++) {
                    bool fg = (bm[row * FONT_5x7_W + col] != 0);
                    if (!in_run) { run_start = col; run_fg = fg; in_run = true; }
                    else if (fg != run_fg) {
                        uint16_t rx = cx + run_start * scale;
                        uint16_t ry = y + row * scale + sr;
                        fb_hline(rx, ry, (col - run_start) * scale, run_fg ? color : bg_color);
                        run_start = col; run_fg = fg;
                    }
                }
                if (in_run) {
                    uint16_t rx = cx + run_start * scale;
                    uint16_t ry = y + row * scale + sr;
                    fb_hline(rx, ry, (FONT_5x7_W - run_start) * scale, run_fg ? color : bg_color);
                }
            }
        }
        cx += FONT_5x7_W * scale;
    }
}

/* ---- Pacman / Ghost / Dot ---- */

static void fb_filled_arc(uint16_t cx, uint16_t cy, uint16_t r,
                           int16_t sa, int16_t ea, uint16_t color) {
    while (sa < 0) sa += 360; while (sa >= 360) sa -= 360;
    while (ea < 0) ea += 360; while (ea >= 360) ea -= 360;
    for (int16_t dy = -(int16_t)r; dy <= (int16_t)r; dy++) {
        for (int16_t dx = -(int16_t)r; dx <= (int16_t)r; dx++) {
            if (dx * dx + dy * dy <= (int16_t)(r * r)) {
                int16_t a = (int16_t)(atan2f((float)dy, (float)dx) * 180.0f / 3.14159265f);
                if (a < 0) a += 360;
                bool in = (sa <= ea) ? (a >= sa && a <= ea) : (a >= sa || a <= ea);
                if (in) fb_pixel(cx + dx, cy + dy, color);
            }
        }
    }
}

void display_draw_pacman(uint16_t cx, uint16_t cy, uint16_t r,
                         uint8_t dir, uint8_t m, uint16_t color) {
    uint16_t ma;
    switch (m) { case 0:ma=5; break; case 1:ma=15; break; case 2:ma=30; break;
                 case 3:ma=45; break; default:ma=30; break; }
    int16_t sa, ea;
    switch (dir) {
        case DIR_RIGHT: sa = ma;     ea = 360 - ma; break;
        case DIR_DOWN:  sa = 90+ma;  ea = 90-ma; if(ea<0)ea+=360; break;
        case DIR_LEFT:  sa = 180+ma; ea = 180-ma; break;
        case DIR_UP:    sa = 270+ma; ea = 270-ma; if(ea<0)ea+=360; break;
        default:        sa = ma;     ea = 360 - ma; break;
    }
    fb_filled_arc(cx, cy, r, sa, ea, color);
}

void display_draw_ghost(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                        uint16_t color, bool eyes_left) {
    uint16_t r = w / 2, bh = h - 4;
    display_draw_filled_circle(x + r, y + r, r, color);
    display_fill_rect(x, y + r, w, bh - r + 1, color);
    uint16_t sy = y + bh, ww = w / 3;
    for (int i = 0; i < 3; i++)
        display_draw_filled_circle(x + i * ww + ww / 2, sy + 2, ww / 2, color);
    uint16_t ey = y + r - 2, lex = x + r - 3, rex = x + r + 3;
    display_draw_filled_circle(lex, ey, 3, COLOR_WHITE);
    display_draw_filled_circle(rex, ey, 3, COLOR_WHITE);
    uint16_t po = eyes_left ? -1 : 1;
    display_draw_filled_circle(lex + po, ey, 1, COLOR_BLUE);
    display_draw_filled_circle(rex + po, ey, 1, COLOR_BLUE);
}

void display_draw_dot(uint16_t cx, uint16_t cy, uint8_t r, uint16_t color) {
    display_draw_filled_circle(cx, cy, r, color);
}

void display_draw_power_pellet(uint16_t cx, uint16_t cy, uint8_t r,
                               uint16_t color, bool blink) {
    if (blink) display_draw_circle(cx, cy, r, color);
    else       display_draw_filled_circle(cx, cy, r, color);
}

/* ---- Flush framebuffer to display ---- */

void display_flush(void) {
    /* Create an LVGL image descriptor pointing to our framebuffer */
    static lv_image_dsc_t img_dsc;
    img_dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
    img_dsc.header.cf    = LV_COLOR_FORMAT_RGB565;
    img_dsc.header.w     = DISPLAY_W;
    img_dsc.header.h     = DISPLAY_H;
    img_dsc.header.stride = DISPLAY_W * 2;
    img_dsc.data_size    = sizeof(fb);
    img_dsc.data         = fb;

    lv_obj_t *img = lv_image_create(lv_layer_top());
    lv_image_set_src(img, &img_dsc);
    lv_obj_set_pos(img, 0, 0);

    lv_refr_now(lvgl_display);
    lv_obj_delete(img);
}
