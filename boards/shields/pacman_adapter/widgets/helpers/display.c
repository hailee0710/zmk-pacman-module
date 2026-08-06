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
#include <stdlib.h>
#include <string.h>

#include <lvgl.h>

#include "display.h"
#include "fonts.h"

LOG_MODULE_REGISTER(display_helpers, CONFIG_DISPLAY_LOG_LEVEL);

/* Full-screen framebuffer (320 × 172 × 2 bytes = 110,080 bytes).
 * Three LVGL image objects on lv_layer_top() each point into a different
 * horizontal band of this buffer: top (0-35), main (36-131), bottom (132-171).
 * When a zone's content changes, only that zone's image is invalidated so
 * lv_refr_now() sends just those rows over SPI instead of the full 172. */
static uint16_t fb[DISPLAY_W * DISPLAY_H];
static const struct device *disp_dev;
static lv_disp_t *lvgl_display;
static lv_obj_t *img_top, *img_main, *img_bottom;

/* CONFIG_LV_COLOR_16_SWAP=y (see pacman_adapter.conf) exists so the ST7789
 * driver can write LVGL's render buffer straight to SPI with no per-pixel
 * swap — LVGL's own color pipeline applies that swap automatically. This
 * framebuffer bypasses that pipeline (it's handed to LVGL as a pre-baked
 * LV_IMG_CF_TRUE_COLOR image, copied through verbatim), so every plain
 * RGB565 value from display.h's COLOR_* constants has to be swapped here,
 * at the point it's actually stored, or every color reaches the panel
 * byte-reversed. */
static inline uint16_t wire_color(uint16_t rgb565) {
    return (uint16_t)((rgb565 >> 8) | (rgb565 << 8));
}

/* ---- Internal: fast horizontal line in framebuffer ----
 * x/y/w are signed: callers derive them from center-radius arithmetic
 * (e.g. cx - r) that legitimately goes negative for shapes near the
 * left/top edge, and clipping that correctly requires a real sign rather
 * than wrapping around through uint16_t.
 */
static inline void fb_hline(int16_t x, int16_t y, int16_t w, uint16_t color) {
    if (y < 0 || y >= DISPLAY_H || w <= 0) return;
    if (x < 0) { w += x; x = 0; }
    if (x >= DISPLAY_W || w <= 0) return;
    if (x + w > DISPLAY_W) w = DISPLAY_W - x;
    uint16_t wc = wire_color(color);
    uint16_t *dst = &fb[y * DISPLAY_W + x];
    for (int16_t i = 0; i < w; i++) dst[i] = wc;
}

static inline void fb_pixel(int16_t x, int16_t y, uint16_t color) {
    if (x >= 0 && x < DISPLAY_W && y >= 0 && y < DISPLAY_H)
        fb[y * DISPLAY_W + x] = wire_color(color);
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

    /* DEBUG: fill entire framebuffer white — if screen stays black,
     * LVGL images on lv_layer_top() are never rendered. */
    for (uint32_t i = 0; i < DISPLAY_W * DISPLAY_H; i++) fb[i] = wire_color(COLOR_DOT_WHITE);

    /* Three zoned image objects — each points to its horizontal band of fb[].
     * Image sources are set once here; framebuffer content is updated in-place
     * and LVGL re-reads from fb[] on each invalidation. */
    static lv_img_dsc_t dsc_top, dsc_main, dsc_bottom;

    dsc_top.header.cf = LV_IMG_CF_TRUE_COLOR;
    dsc_top.header.w  = DISPLAY_W;
    dsc_top.header.h  = DISPLAY_ZONE_TOP_H;
    dsc_top.data_size = DISPLAY_W * DISPLAY_ZONE_TOP_H * 2;
    dsc_top.data      = (const uint8_t *)&fb[DISPLAY_ZONE_TOP_Y * DISPLAY_W];

    dsc_main.header.cf = LV_IMG_CF_TRUE_COLOR;
    dsc_main.header.w  = DISPLAY_W;
    dsc_main.header.h  = DISPLAY_ZONE_MAIN_H;
    dsc_main.data_size = DISPLAY_W * DISPLAY_ZONE_MAIN_H * 2;
    dsc_main.data      = (const uint8_t *)&fb[DISPLAY_ZONE_MAIN_Y * DISPLAY_W];

    dsc_bottom.header.cf = LV_IMG_CF_TRUE_COLOR;
    dsc_bottom.header.w  = DISPLAY_W;
    dsc_bottom.header.h  = DISPLAY_ZONE_BOTTOM_H;
    dsc_bottom.data_size = DISPLAY_W * DISPLAY_ZONE_BOTTOM_H * 2;
    dsc_bottom.data      = (const uint8_t *)&fb[DISPLAY_ZONE_BOTTOM_Y * DISPLAY_W];

    img_top    = lv_img_create(lv_layer_top());
    lv_obj_set_pos(img_top, 0, DISPLAY_ZONE_TOP_Y);
    lv_img_set_src(img_top, &dsc_top);

    img_main   = lv_img_create(lv_layer_top());
    lv_obj_set_pos(img_main, 0, DISPLAY_ZONE_MAIN_Y);
    lv_img_set_src(img_main, &dsc_main);

    img_bottom = lv_img_create(lv_layer_top());
    lv_obj_set_pos(img_bottom, 0, DISPLAY_ZONE_BOTTOM_Y);
    lv_img_set_src(img_bottom, &dsc_bottom);

    LOG_INF("Framebuffer ready (%dx%d, %u bytes)", DISPLAY_W, DISPLAY_H, (unsigned)sizeof(fb));
    return 0;
}

void display_begin_frame(void) {
    /* Optionally clear — caller can draw background manually */
}

/* ---- Drawing primitives ---- */

void display_fill(uint16_t color) {
    uint16_t wc = wire_color(color);
    for (uint32_t i = 0; i < DISPLAY_W * DISPLAY_H; i++) fb[i] = wc;
}

void display_fill_rect(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t color) {
    for (uint16_t row = 0; row < h; row++)
        fb_hline(x, y + row, w, color);
}

void display_draw_pixel(int16_t x, int16_t y, uint16_t color) {
    fb_pixel(x, y, color);
}

void display_draw_rect(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t color) {
    fb_hline(x, y, w, color);                    /* top */
    fb_hline(x, y + h - 1, w, color);            /* bottom */
    for (uint16_t i = 0; i < h; i++) {           /* left + right */
        fb_pixel(x, y + i, color);
        fb_pixel(x + w - 1, y + i, color);
    }
}

/* x1/y1/x2/y2 are signed so that the direction the walk steps (sx/sy) is
 * decided by a real signed comparison. Passing these through as uint16_t
 * (as this used to) meant a negative endpoint — e.g. cx - r for a circle
 * near the left edge — silently wrapped to a huge unsigned value; the
 * *distance* (dx/dy) still came out right after the (int16_t) cast below,
 * but the x1 < x2 comparison that picks sx/sy saw the wrapped value and
 * could step in the wrong direction, walking the "line" the long way
 * around all 65536 values instead of the short way to x2/y2. */
void display_draw_line(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color) {
    int16_t dx = abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
    int16_t dy = -abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
    int16_t err = dx + dy;
    while (1) {
        fb_pixel(x1, y1, color);
        if (x1 == x2 && y1 == y2) break;
        int16_t e2 = 2 * err;
        if (e2 >= dy) { err += dy; x1 += sx; }
        if (e2 <= dx) { err += dx; y1 += sy; }
    }
}

void display_draw_circle(int16_t cx, int16_t cy, uint16_t r, uint16_t color) {
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

void display_draw_filled_circle(int16_t cx, int16_t cy, uint16_t r, uint16_t color) {
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

void display_write_text(int16_t x, int16_t y, const char *text,
                        uint16_t color, uint16_t bg_color, uint8_t scale) {
    if (!text || scale == 0) return;
    int16_t cx = x;
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
                        int16_t rx = cx + run_start * scale;
                        int16_t ry = y + row * scale + sr;
                        fb_hline(rx, ry, (col - run_start) * scale, run_fg ? color : bg_color);
                        run_start = col; run_fg = fg;
                    }
                }
                if (in_run) {
                    int16_t rx = cx + run_start * scale;
                    int16_t ry = y + row * scale + sr;
                    fb_hline(rx, ry, (FONT_5x7_W - run_start) * scale, run_fg ? color : bg_color);
                }
            }
        }
        cx += FONT_5x7_W * scale;
    }
}

/* ---- Pacman / Ghost / Dot ---- */

/* Integer sin LUT: sin(0°..90°) scaled by 256, i.e. lut[d] = round(256*sin(d)).
 * Exactly 91 entries for d = 0..90 inclusive — the previous table had 92
 * initializers (an "excess elements" overflow silently dropped the last
 * one) and was a half-wave over 0..180° squeezed into a quarter-wave table
 * peaking at ~135 instead of 256. */
static const int16_t sin_lut[91] = {
    0,  4,  9,  13, 18, 22, 27, 31, 36, 40, 44, 49, 53, 58, 62, 66,
    71, 75, 79, 83, 88, 92, 96, 100,104,108,112,116,120,124,128,132,
    136,139,143,147,150,154,158,161,165,168,171,175,178,181,184,187,
    190,193,196,199,202,204,207,210,212,215,217,219,222,224,226,228,
    230,232,234,236,237,239,241,242,243,245,246,247,248,249,250,251,
    252,253,254,254,255,255,255,256,256,256,256
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
static void fb_filled_arc(int16_t cx, int16_t cy, uint16_t r,
                           int16_t sa, int16_t ea, uint16_t color) {
    /* Normalize angles to 0..359 */
    while (sa < 0) { sa += 360; }
    while (sa >= 360) { sa -= 360; }
    while (ea < 0) { ea += 360; }
    while (ea >= 360) { ea -= 360; }

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

            /* Inside arc if: cross_s >= 0 (on or CCW of start) AND cross_e <= 0 (on or CW of end).
             * Which combinator to use depends on the arc's sweep, not on whether
             * sa <= ea numerically — sa <= ea says nothing about the sweep once sa/ea
             * wrap past 360 (e.g. sa=30, ea=330 is a 300° sweep with sa < ea, where the
             * two bounding half-planes intersect to at most 180°, not OR). A sweep over
             * 180° needs the union of the two half-planes instead of their intersection. */
            int16_t sweep = ea - sa;
            if (sweep < 0) sweep += 360;
            bool inside = (sweep <= 180) ? (cross_s >= 0 && cross_e <= 0)
                                          : (cross_s >= 0 || cross_e <= 0);
            if (inside) fb_pixel(cx + dx, cy + dy, color);
        }
    }
}

void display_draw_pacman(int16_t cx, int16_t cy, uint16_t r,
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

void display_draw_ghost(int16_t x, int16_t y, uint16_t w, uint16_t h,
                        uint16_t color, bool eyes_left) {
    uint16_t r = w / 2, bh = h - 4;
    display_draw_filled_circle(x + r, y + r, r, color);
    display_fill_rect(x, y + r, w, bh - r + 1, color);
    int16_t sy = y + bh; uint16_t ww = w / 3;
    for (int i = 0; i < 3; i++)
        display_draw_filled_circle(x + i * ww + ww / 2, sy + 2, ww / 2, color);
    int16_t ey = y + r - 2, lex = x + r - 3, rex = x + r + 3;
    display_draw_filled_circle(lex, ey, 3, COLOR_WHITE);
    display_draw_filled_circle(rex, ey, 3, COLOR_WHITE);
    int16_t po = eyes_left ? -1 : 1;
    display_draw_filled_circle(lex + po, ey, 1, COLOR_BLUE);
    display_draw_filled_circle(rex + po, ey, 1, COLOR_BLUE);
}

void display_draw_dot(int16_t cx, int16_t cy, uint8_t r, uint16_t color) {
    display_draw_filled_circle(cx, cy, r, color);
}

void display_draw_power_pellet(int16_t cx, int16_t cy, uint8_t r,
                               uint16_t color, bool blink) {
    if (blink) display_draw_circle(cx, cy, r, color);
    else       display_draw_filled_circle(cx, cy, r, color);
}

/* ---- Flush framebuffer to display ----
 * Image sources are set once in display_init(). Each zone's content is
 * drawn directly into fb[]. To send updated rows to the panel:
 *   1. Render into fb[] (any drawing primitive).
 *   2. Call display_inv_zone_*() for each zone that was touched.
 *   3. Call display_flush() once — lv_refr_now() sends only the
 *      invalidated zones' rows over SPI.
 *
 * display_flush() alone (after display_inv_zone_top + main + bottom)
 * is equivalent to a full-screen refresh. */

void display_inv_zone_top(void)    { lv_obj_invalidate(img_top); }
void display_inv_zone_main(void)   { lv_obj_invalidate(img_main); }
void display_inv_zone_bottom(void) { lv_obj_invalidate(img_bottom); }

void display_flush(void) {
    lv_refr_now(lvgl_display);
}
