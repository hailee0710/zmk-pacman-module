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

#include <lvgl.h>

#include "display.h"
#include "fonts.h"

LOG_MODULE_REGISTER(display_helpers, CONFIG_DISPLAY_LOG_LEVEL);

/* Full-screen framebuffer (320 × 172 × 2 bytes = 110,080 bytes) */
static uint16_t fb[DISPLAY_W * DISPLAY_H];
static const struct device *disp_dev;
static lv_disp_t *lvgl_display;
static lv_obj_t *flush_img;

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
    lvgl_display = lv_disp_get_default();
    if (!lvgl_display) {
        LOG_ERR("No LVGL display found");
        return -ENODEV;
    }
    memset(fb, 0, sizeof(fb));
    flush_img = lv_img_create(lv_layer_top());
    lv_obj_set_pos(flush_img, 0, 0);
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

/* Integer sin LUT: sin(0°..90°) scaled by 256 */
static const int16_t sin_lut[91] = {
    0,4,8,13,17,22,26,31,35,39,44,48,52,56,60,64,68,72,76,80,
    83,87,90,94,97,100,103,106,109,112,114,117,119,121,123,125,
    127,129,130,131,132,133,134,134,135,135,135,135,135,134,134,
    133,132,131,130,129,127,125,123,121,119,117,114,112,109,106,
    103,100,97,94,90,87,83,80,76,72,68,64,60,56,52,48,44,39,35,
    31,26,22,17,13,8,4
};

/* Get sin(deg) scaled by 256 for any integer degree 0..359 */
static inline int16_t isin(int16_t d) {
    d = d % 360; if (d < 0) d += 360;
    return (d <= 90)  ? sin_lut[d] :
           (d <= 180) ? sin_lut[180 - d] :
           (d <= 270) ? -sin_lut[d - 180] : -sin_lut[360 - d];
}

/* Get cos(deg) scaled by 256 for any integer degree 0..359 */
static inline int16_t icos(int16_t d) { return isin(d + 90); }

/* Draw a filled circular arc from start_angle to end_angle (degrees, 0-359).
   Uses integer arithmetic only (cross products + LUT for sin/cos). */
static void fb_filled_arc(uint16_t cx, uint16_t cy, uint16_t r,
                           int16_t sa, int16_t ea, uint16_t color) {
    /* Normalize angles to 0..359 */
    while (sa < 0) sa += 360; while (sa >= 360) sa -= 360;
    while (ea < 0) ea += 360; while (ea >= 360) ea -= 360;

    /* Direction vectors (scaled by 256) for the two bounding rays */
    int16_t sx = icos(sa), sy = isin(sa); /* start ray (counter-clockwise edge) */
    int16_t ex = icos(ea), ey = isin(ea); /* end ray (clockwise edge) */

    int16_t ir = (int16_t)r;
    for (int16_t dy = -ir; dy <= ir; dy++) {
        for (int16_t dx = -ir; dx <= ir; dx++) {
            /* Skip pixels outside the circle */
            if ((int32_t)dx * dx + (int32_t)dy * dy > (int32_t)ir * ir) continue;

            /* Cross products determine which side of each bounding ray the point is on.
               cross > 0 means counter-clockwise from the ray, cross < 0 means clockwise.
               The arc sweeps from sa (CCW edge) to ea (CW edge).
               A point is inside if it's clockwise from sa AND counter-clockwise from ea. */
            int32_t cross_s = (int32_t)sx * dy - (int32_t)sy * dx; /* positive = CCW of start */
            int32_t cross_e = (int32_t)ex * dy - (int32_t)ey * dx; /* positive = CCW of end */

            /* Inside arc if: cross_s >= 0 (on or CCW of start) AND cross_e <= 0 (on or CW of end) */
            /* For wrap-around arcs (sa > ea), the condition is a logical OR instead */
            bool inside;
            if (sa <= ea) {
                inside = (cross_s >= 0 && cross_e <= 0);
            } else {
                inside = (cross_s >= 0 || cross_e <= 0);
            }
            if (inside) fb_pixel(cx + dx, cy + dy, color);
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
    static lv_img_dsc_t img_dsc;
    img_dsc.header.always_zero = 0;
    img_dsc.header.reserved    = 0;
    img_dsc.header.cf          = LV_IMG_CF_TRUE_COLOR;
    img_dsc.header.w           = DISPLAY_W;
    img_dsc.header.h           = DISPLAY_H;
    img_dsc.data_size          = sizeof(fb);
    img_dsc.data               = (const uint8_t *)fb;

    lv_img_set_src(flush_img, &img_dsc);
    lv_refr_now(lvgl_display);
}
