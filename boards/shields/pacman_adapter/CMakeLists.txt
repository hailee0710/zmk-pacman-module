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
    widgets/logo.c
    widgets/splash.c
    widgets/configuration.c
    widgets/modifier.c
    widgets/action_button.c
    widgets/helpers/display.c
    widgets/helpers/fonts.c
    widgets/helpers/list.c
    widgets/helpers/settings.c
)

if(CONFIG_ZMK_DONGLE_PACMAN_BUZZER)
    target_sources(app PRIVATE
        widgets/helpers/buzzer.c
        widgets/helpers/pwm.c
    )
endif()
