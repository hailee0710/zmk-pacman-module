# Pacman Dongle Module
# Based on snake-module by joaopedropio
# Adapted for ST7789P3 172x320 display with Pacman theme

if(CONFIG_ZMK_DONGLE_PACMAN)
  zephyr_include_directories(include)

  add_subdirectory_ifdef(CONFIG_ZMK_DONGLE_ACTION src/events)
  add_subdirectory_ifdef(CONFIG_ZMK_DONGLE_ACTION src/behaviors)
endif()
