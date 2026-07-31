# Pacman Dongle Adapter Shield CMakeLists

# Add widget sources
target_sources(app PRIVATE
    custom_status_screen.c
    widgets/pacman.c
    widgets/theme.c
    widgets/battery_status.c
    widgets/layer_status.c
    widgets/output_status.c
    widgets/wpm.c
    widgets/helpers/display.c
    widgets/helpers/fonts.c
)
