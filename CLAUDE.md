# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview

ZMK module that adds a Pacman-themed status screen to a keyboard dongle. Targets ST7789P3 172×320 landscape LCD on nRF52840 (nice!nano v2). Installed as a Zephyr module in a ZMK west workspace. Each keypress spawns a dot that travels right-to-left toward a big stationary Pacman; WPM ≥ 80 triggers ghosts.

## Build

This is a ZMK module — no standalone build. It is consumed by a ZMK west workspace:

```sh
west update      # pull the module into the workspace
west build ...   # standard ZMK build with CONFIG_ZMK_DONGLE_PACMAN=y
```

The module entry point is `zephyr/module.yml` (sets board root, DTS root, CMake entry, Kconfig entry).

## Architecture

### Framebuffer rendering (not LVGL widgets)

All drawing goes through a static `uint16_t fb[DISPLAY_W * DISPLAY_H]` buffer in `widgets/helpers/display.c`. Rendering functions (`display_draw_pacman()`, `display_fill_rect()`, etc.) write pixels directly to this buffer. A single `display_flush()` at the end of each frame wraps the buffer in an `lv_img_dsc_t` on `lv_layer_top()` and calls `lv_refr_now()` — one SPI transfer per frame, zero LVGL object allocations during drawing.

**Critical color detail:** Because `CONFIG_LV_COLOR_16_SWAP=y` is set (so the ST7789 driver can DMA LVGL's render buffer to SPI without per-pixel swap), the `fb[]` buffer that bypasses LVGL's color pipeline must store every pixel byte-swapped. `wire_color()` in `display.c` handles this — all `COLOR_*` constants in `display.h` are native RGB565 and get swapped on storage. If colors appear wrong on hardware, this is the first thing to check.

### ZMK event flow

`custom_status_screen.c` is the orchestrator. It:

1. Implements `zmk_display_status_screen()` — the hook ZMK calls after display/LVGL init.
2. Initializes all widget state structs (pacman, theme, battery, layer, output, WPM).
3. Seeds each `ZMK_DISPLAY_WIDGET_LISTENER` with current ZMK core state via `cs_*_init()` calls.
4. Starts a 33ms `lv_timer` for the render tick (~30fps).

ZMK events (BLE profile change, USB connect, battery, layer, keycode) arrive on arbitrary threads. Each listener's "get state" function runs under a ZMK mutex on the firing thread, then the "update callback" is marshalled onto `zmk_display_work_q()` — same queue as the render tick — so all widget state mutation is single-threaded and lock-free.

### Widget state pattern

Each widget has its own state struct, init, and setter functions:

| Widget | Struct | Purpose |
|--------|--------|---------|
| `pacman.c/h` | `pacman_status_t` | Central state: dots array, mouth animation, host status, batteries, layer, WPM. `tick()` moves dots and checks dirtiness; `render()` draws all three screen zones. |
| `wpm.c/h` | `wpm_state_t` | Sliding-window WPM calc (5s window, circular buffer of 64 timestamps, 5 chars/word) |
| `battery_status.c/h` | `battery_status_t` | Per-half battery levels (source 0/1 from `zmk_peripheral_battery_state_changed`) |
| `output_status.c/h` | `output_status_t` | BLE/USB connection state |
| `layer_status.c/h` | `layer_status_t` | Active ZMK layer name |
| `theme.c/h` | `theme_state_t` | Theme enum (currently only `THEME_PACMAN` exists) |
| `helpers/display.c/h` | — | Framebuffer, drawing primitives, `display_flush()` |
| `helpers/fonts.c/h` | — | 5×7 and 3×5 bitmap fonts |

### Display driver

`drivers/display/display_st7789v.c` implements Zephyr's `display_driver_api` for `sitronix,st7789p3` compatible. Handles SPI init sequence (MADCTL, gamma, porch control, etc.), hardware reset, and the `write()` callback that Zephyr's LVGL glue calls with pixel data. The driver supports devicetree properties `madctl`, `colmod`, `inversion`, `x-offset`, `y-offset` for panel-specific tuning without code changes.

### DTS overlay inheritance

```
pacman_adapter.overlay        ← module default (all pins, sets chosen zephyr,display)
       ↓
boards/nice_nano_v2.overlay   ← board-specific overrides
       ↓
user EXTRA_DTC_OVERLAY_FILE   ← user's custom overrides (wins)
```

Last overlay to set a property wins. Pin overrides should be done in user config, not by editing module files.

## Key constraints

- **ZMK v0.3**: This module is written against ZMK v0.3 display/behavior APIs. `CS_*_init()` hooks, `ZMK_DISPLAY_WIDGET_LISTENER` macros, and `zmk_display_status_screen()` signature are all v0.3-specific.
- **nRF52840 P0.09/P0.10**: Default CS pin (P0.09) requires `CONFIG_NFCT_PINS_AS_GPIOS=y` — these pins default to NFC antenna function. This is set in `pacman_adapter.conf`.
- **SPI caps at 8 MHz**: nRF52840 SPIM0/1/2 max at 8 MHz; only SPIM3 does 32 MHz. Default overlay uses SPIM0 at 8 MHz.
- **No heap allocations during rendering**: All drawing functions write to the static framebuffer. `lv_img_create()` is called once during `display_init()`, not per frame.

## Pin wiring (nice!nano v2 default)

MOSI=P1.01, SCK=P1.06, CS=P0.09, DC=P1.07, RST=P1.02, BL=not wired. P1.xx pins are on the inner row; P0.09 is bottom edge. None conflict with onboard charge LED (P0.17).
