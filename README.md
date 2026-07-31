# zmk-pacman-module

A ZMK module that adds a Pacman-themed status screen to a dongle with an ST7789P3 172×320 LCD display. Each keypress spawns a dot that travels right-to-left toward a big stationary Pacman — type fast enough and ghosts appear. Includes WPM tracking, battery indicators, and BLE/USB host status.

## Features

- **Big Pacman** — large stationary Pacman (radius 52, nearly fills the 124px main zone) with animated mouth
- **Dot/ghost flow** — single-row dots travel right-to-left with each keypress; ghosts appear at 80+ WPM
- **WPM tracker** — sliding-window words-per-minute with peak tracking displayed in the bottom bar
- **Battery gauge** — dual battery indicators in the top bar
- **Host status** — shows BLE or USB connection state
- **Pixelwave palette** — synthwave color scheme by DogesArePros (9-color palette, dots kept as pure white)
- **Framebuffer rendering** — single full-screen flush per frame; no per-shape LVGL allocations

## Requirements

| Component | Details |
|-----------|---------|
| **Display** | ST7789P3 172×320 (landscape), SPI interface |
| **MCU** | nRF52840 (nice!nano v2 tested) |
| **Firmware** | ZMK with west workspace |
| **Connections** | 5 wires: MOSI, SCK, CS, DC, RESET (+ optional BL for backlight) |

## Installation

### 1. Add the module to your ZMK west workspace

In your `config/west.yml`, add the module:

```yaml
manifest:
  remotes:
    - name: zmkfirmware
      url-base: https://github.com/zmkfirmware
    - name: your-org
      url-base: https://github.com/your-org
  projects:
    - name: zmk
      remote: zmkfirmware
      revision: main
      import: app/west.yml
    - name: zmk-pacman-module
      remote: your-org
      revision: main
  self:
    path: config
```

Run `west update` to pull the module.

### 2. Enable the module

In your shield or board `.conf` file:

```
CONFIG_ZMK_DONGLE_PACMAN=y
CONFIG_ZMK_DISPLAY=y
CONFIG_ZMK_DISPLAY_STATUS_SCREEN_CUSTOM=y
```

### 3. Add the shield

In your `build.yaml`:

```yaml
include:
  - board: nice_nano_v2
    shield: pacman_adapter
```

### 4. Wire the display

Default pin assignments for nice!nano v2:

| Display Pin | nRF52840 Pin | Signal |
|-------------|-------------|--------|
| SDA (MOSI)  | P1.01       | SPI MOSI |
| SCL (SCK)   | P1.06       | SPI SCK |
| CS          | P0.09       | Chip select |
| DC          | P1.07       | Data/command |
| RST         | P1.02       | Hardware reset |
| BL          | *not wired* | Backlight (optional, active high) |
| VCC         | VCC (3.3V)  | Power |
| GND         | GND         | Ground |

> **Note:** P1.xx pins are on the inner row of the nice!nano v2. P0.09 is on the bottom edge. None conflict with the onboard charge LED (P0.17).

## Custom pin mapping

You **do not** need to edit the module's overlay files. Use a custom overlay in your own ZMK config to override any pin:

```dts
/* my_pacman_pins.overlay */
&spi0 {
    st7789p3: st7789p3@0 {
        reset-gpios = <&gpio0 5 GPIO_ACTIVE_LOW>;
        dc-gpios = <&gpio0 13 GPIO_ACTIVE_LOW>;
        spi-max-frequency = <20000000>;
    };
};
```

Add it to your build:

```yaml
# build.yaml
include:
  - board: nice_nano_v2
    shield: pacman_adapter
    cmake-args:
      - "-DEXTRA_DTC_OVERLAY_FILE=/path/to/my_pacman_pins.overlay"
```

### How the overlay inheritance works

```
pacman_adapter.overlay          ← module default (all pins)
        ↓
boards/nice_nano_v2.overlay     ← board-specific overrides from the module
        ↓
your_custom.overlay             ← your own overrides (wins)
```

The **last overlay to set a property wins**. You can override any property — pins, SPI speed, display dimensions, orientation (`madctl`), color format (`colmod`), or offsets (`x-offset`, `y-offset`).

## Display layout

```
  320×172 landscape

  ┌──────────────────────────────────────────────────┐
  │  [🔋 L%]  [🔋 R%]    PACMAN DONGLE    ● USB/BLE  │  Top bar (24px)
  ├──────────────────────────────────────────────────┤
  │                                                  │
  │     ╭──────╮                                     │
  │    ╱        ╲   Big Pacman (r=52, cx=80, cy=86)  │
  │   ╱    ●     ╲    ●  dot                         │
  │  ╱            ╲   👻  ghost (WPM ≥ 80)            │  Main zone (124px)
  │  ╲            ╱   ·  guide line (dim)            │
  │   ╲          ╱    ← all flow right-to-left       │
  │    ╲        ╱                                    │
  │     ╰──────╯                                     │
  ├──────────────────────────────────────────────────┤
  │  ██████████░░░░░░░│80         WPM:72   PEAK:95   │  Bottom bar (24px)
  └──────────────────────────────────────────────────┘
```

## Color palette

Uses the **Pixelwave** palette by DogesArePros — a 9-color synthwave scheme:

| Hex | Role |
|-----|------|
| `#090038` | Deep purple (background) |
| `#060026` | Dark navy (alternative background) |
| `#00125e` | Dark blue (guide line, gray) |
| `#0170fe` | Bright blue (accents, green substitute, ghost cyan) |
| `#fff056` | Bright yellow (Pacman, text, headings) |
| `#ffd156` | Warm yellow (WPM bar mid-range, secondary text) |
| `#ff9156` | Orange (ghost orange) |
| `#ff5d56` | Coral red (ghost red, low battery, WPM ghost zone) |
| `#ff549d` | Hot pink (ghost pink, magenta/purple substitute) |
| `#ffffff` | Pure white (dots only — kept outside palette for visibility) |

All colors are defined as RGB565 constants in `widgets/helpers/display.h`.

## Kconfig options

### Module-level options

| Option | Default | Description |
|--------|---------|-------------|
| `CONFIG_ZMK_DONGLE_PACMAN` | y | Master enable for the Pacman module |

### Shield-level options

| Option | Default | Description |
|--------|---------|-------------|
| `CONFIG_ZMK_DONGLE_PACMAN_BUZZER` | n | Enable buzzer/sound effects support |

## Project structure

```
zmk-pacman-module/
├── CMakeLists.txt                  # Module entry point
├── Kconfig                         # Module Kconfig options
├── README.md                       # This file
├── dts/bindings/
│   ├── behaviors/
│   │   └── zmk,behavior-dongle-action.yaml
│   └── display/
│       └── sitronix,st7789p3.yaml
├── drivers/display/
│   ├── display_st7789v.c
│   └── display_st7789v.h
├── include/zmk_dongle_events/
│   └── dongle_action_event.h
├── src/
│   ├── behaviors/
│   │   ├── CMakeLists.txt
│   │   └── behavior_dongle_action.c
│   └── events/
│       ├── CMakeLists.txt
│       └── dongle_action_event.c
├── boards/shields/pacman_adapter/
│   ├── CMakeLists.txt
│   ├── Kconfig.defconfig
│   ├── Kconfig.shield
│   ├── pacman_adapter.conf
│   ├── pacman_adapter.overlay
│   ├── custom_status_screen.c      # Event handlers, render orchestrator
│   ├── custom_status_screen.h
│   ├── boards/
│   │   └── nice_nano_v2.overlay    # Board-specific pin placeholders
│   └── widgets/
│       ├── pacman.c/h              # Pacman animation + rendering
│       ├── theme.c/h               # Theme state
│       ├── battery_status.c/h      # Battery tracking
│       ├── layer_status.c/h        # Layer tracking
│       ├── output_status.c/h       # BLE/USB output
│       ├── wpm.c/h                 # WPM calculator
│       ├── action_button.c/h       # On-screen button
│       ├── configuration.c/h       # Config state
│       ├── logo.c/h                # Boot logo
│       ├── splash.c/h              # Splash screen
│       ├── modifier.c/h            # Modifier key status
│       └── helpers/
│           ├── display.c/h         # Framebuffer drawing + Pixelwave palette
│           ├── fonts.c/h           # 5×7 and 3×5 bitmap fonts
│           ├── list.c/h            # List utility
│           ├── settings.c/h        # Persistent settings
│           ├── buzzer.c/h          # Buzzer (stub)
│           └── pwm.c/h             # PWM (stub)
└── zephyr/
    └── module.yml                  # Zephyr module manifest
```

## Build troubleshooting

**Module not found at build time:**
- Verify `module.yml` `depends:` includes `zmk`
- Run `west update` and check the module appears under `modules/` in the build directory
- Check that `CONFIG_ZMK_DONGLE_PACMAN=y` appears in `build/zephyr/.config`

**Display stays blank:**
- Confirm SPI and GPIO are enabled: `CONFIG_SPI=y`, `CONFIG_GPIO=y`
- Verify the `zmk,display` chosen node points to your display: check `build/zephyr/devicetree_generated.h` for `DT_CHOSEN_zmk_display`
- Check the display driver init log: `CONFIG_DISPLAY_LOG_LEVEL=4` for debug output
- Verify pin assignments match your physical wiring (especially the P1.xx bank)

**Ghost zone never triggers:**
- WPM ghost threshold is 80 — you need sustained fast typing
- Check `pacman_status_set_wpm()` is being called from `custom_status_screen.c`

## License

MIT — see source files for SPDX headers.

## Credits

Based on the snake-module by joaopedropio. Adapted for ST7789P3 172×320 landscape display with Pacman theme. Color palette Pixelwave by DogesArePros.
