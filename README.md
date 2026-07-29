# zmk-pacman-module

A ZMK module that adds a Pacman-themed status screen to a dongle with an ST7789P3 172×320 LCD display. Each keypress spawns a dot that travels toward Pacman — type fast enough and ghosts appear. Includes WPM tracking, battery indicators, and BLE/USB host status.

## Features

- **Pacman animation** — dots travel across the screen with each keypress; ghosts appear at 80+ WPM
- **WPM tracker** — sliding-window words-per-minute with peak tracking
- **Battery gauge** — dual battery indicators in the top bar
- **Host status** — shows BLE or USB connection state
- **Dongle action behavior** — custom `&dongle_action` keymap binding for game/menu controls
- **Settings persistence** — theme, high score, and brightness survive reboots
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
CONFIG_ZMK_DONGLE_ACTION=y
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

Default pin assignments for the `pacman_adapter` shield:

| Display Pin | nRF52840 Pin | nice!nano v2 Label | Signal |
|-------------|-------------|---------------------|--------|
| SDA (MOSI)  | P0.06       | D4                  | SPI MOSI |
| SCL (SCK)   | P0.08       | D6                  | SPI SCK |
| CS          | P0.04       | D2                  | Chip select |
| DC          | P0.16       | —                   | Data/command |
| RESET       | P0.05       | D3                  | Hardware reset |
| BL          | *not wired* | —                   | Backlight (optional, active high) |
| VCC         | VCC (3.3V)  | —                   | Power |
| GND         | GND         | —                   | Ground |

> **Note:** The shield default overlay uses P0.17 for RESET, but the `nice_nano_v2.overlay` overrides this to P0.05 (D3) because P0.17 is connected to the onboard charge LED on nice!nano v2. If your board revision uses a different pin, or you've wired RESET elsewhere, see [Custom pin mapping](#custom-pin-mapping).

### 5. Add the behavior to your keymap

The module provides a `&dongle_action` behavior. Bind it in your keymap:

```dts
#include <zmk_dongle_events/dongle_action_event.h>

/ {
    keymap {
        compatible = "zmk,keymap";

        game_layer {
            bindings = <
                &none          &dongle_action DONGLE_ACTION_PACMAN_UP    &none
                &dongle_action DONGLE_ACTION_PACMAN_LEFT  &dongle_action DONGLE_ACTION_PACMAN_START &dongle_action DONGLE_ACTION_PACMAN_RIGHT
                &none          &dongle_action DONGLE_ACTION_PACMAN_DOWN  &none
            >;
        };
    };
};
```

Available action codes:

| Constant | Value | Purpose |
|----------|-------|---------|
| `DONGLE_ACTION_PACMAN_UP` | 0 | Navigate up |
| `DONGLE_ACTION_PACMAN_DOWN` | 1 | Navigate down |
| `DONGLE_ACTION_PACMAN_LEFT` | 2 | Navigate left |
| `DONGLE_ACTION_PACMAN_RIGHT` | 3 | Navigate right |
| `DONGLE_ACTION_PACMAN_START` | 4 | Start game or animation |
| `DONGLE_ACTION_PACMAN_PAUSE` | 5 | Toggle pause |
| `DONGLE_ACTION_PACMAN_QUIT` | 6 | Quit to status screen |

## Custom pin mapping

You **do not** need to edit the module's overlay files. Use a custom overlay in your own ZMK config to override any pin:

```dts
/* my_pacman_pins.overlay */
&spi0 {
    st7789p3: st7789p3@0 {
        /* Remap reset from P0.17 to P1.01 */
        reset-gpios = <&gpio1 1 GPIO_ACTIVE_LOW>;

        /* Change DC to P0.13 */
        dc-gpios = <&gpio0 13 GPIO_ACTIVE_LOW>;

        /* Lower SPI speed for longer wires */
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

## Kconfig options

### Module-level options (in module `Kconfig`)

| Option | Default | Description |
|--------|---------|-------------|
| `CONFIG_ZMK_DONGLE_PACMAN` | y | Master enable for the Pacman module |
| `CONFIG_ZMK_DONGLE_ACTION` | y | Enable the `&dongle_action` behavior |

### Shield-level options (in `Kconfig.shield`)

| Option | Default | Description |
|--------|---------|-------------|
| `CONFIG_ZMK_DONGLE_PACMAN_BUZZER` | n | Enable buzzer/sound effects support |

Disable the behavior if you don't need game controls to save flash:

```
CONFIG_ZMK_DONGLE_ACTION=n
```

## Project structure

```
zmk-pacman-module/
├── CMakeLists.txt                  # Module entry point
├── Kconfig                         # Module Kconfig options
├── README.md                       # This file
├── config/                         # (reserved for additional Kconfig fragments)
├── dts/bindings/
│   ├── behaviors/
│   │   └── zmk,behavior-dongle-action.yaml   # Behavior binding
│   └── display/
│       └── sitronix,st7789p3.yaml            # Display binding
├── drivers/display/
│   ├── display_st7789v.c           # ST7789P3 SPI display driver
│   └── display_st7789v.h
├── include/zmk_dongle_events/
│   └── dongle_action_event.h       # Event + action enum definitions
├── src/
│   ├── behaviors/
│   │   ├── CMakeLists.txt
│   │   └── behavior_dongle_action.c   # Dongle action behavior driver
│   └── events/
│       ├── CMakeLists.txt
│       └── dongle_action_event.c      # Event implementation
├── boards/shields/pacman_adapter/
│   ├── CMakeLists.txt
│   ├── Kconfig.defconfig           # Default Kconfig values for the shield
│   ├── Kconfig.shield              # Shield-specific Kconfig options
│   ├── pacman_adapter.conf         # Kconfig fragment (.conf)
│   ├── pacman_adapter.overlay      # Shield Devicetree overlay
│   ├── custom_status_screen.c      # Main orchestrator (ZMK events, rendering)
│   ├── custom_status_screen.h
│   ├── boards/
│   │   └── nice_nano_v2.overlay    # Board-specific pin overrides
│   └── widgets/
│       ├── pacman.c/h              # Core Pacman animation + rendering
│       ├── theme.c/h               # Theme state management
│       ├── battery_status.c/h      # Battery level tracking
│       ├── layer_status.c/h        # Active layer tracking
│       ├── output_status.c/h       # BLE/USB output tracking
│       ├── wpm.c/h                 # WPM sliding-window calculator
│       ├── action_button.c/h       # On-screen button widget
│       ├── configuration.c/h       # Configuration state
│       ├── logo.c/h                # Boot logo screen
│       ├── splash.c/h              # Splash screen
│       ├── modifier.c/h            # Modifier key status
│       └── helpers/
│           ├── display.c/h         # Framebuffer rendering primitives
│           ├── fonts.c/h           # 5×7 and 3×5 bitmap fonts
│           ├── list.c/h            # Simple list utility
│           ├── settings.c/h        # Persistent settings (ZMK settings subsystem)
│           ├── buzzer.c/h          # Buzzer/sound effects (stub)
│           └── pwm.c/h             # PWM helper (stub)
└── zephyr/
    └── module.yml                  # Zephyr module manifest
```

## Build troubleshooting

**Module not found at build time:**
- Verify `module.yml` `depends:` includes `zmk`
- Run `west update` and check the module appears under `modules/` in the build directory
- Check that `ZMK_DONGLE_PACMAN=y` appears in your `.config` (under `build/zephyr/.config`)

**Display stays blank:**
- Confirm SPI and GPIO are enabled: `CONFIG_SPI=y`, `CONFIG_GPIO=y`
- Verify the `zmk,display` chosen node points to your display: check `build/zephyr/devicetree_generated.h` for `DT_CHOSEN_zmk_display`
- Check the display driver init log: `CONFIG_DISPLAY_LOG_LEVEL=4` for debug output
- Verify pin assignments match your physical wiring

**Behavior not working in keymap:**
- Confirm `CONFIG_ZMK_DONGLE_ACTION=y` is set
- The binding syntax uses the C constant directly: `&dongle_action DONGLE_ACTION_PACMAN_START` (include the event header first)
- The `#binding-cells` is 1 — you must pass exactly one integer parameter

## License

MIT — see source files for SPDX headers.

## Credits

Based on the snake-module by joaopedropio. Adapted for ST7789P3 172×320 landscape display with Pacman theme.
