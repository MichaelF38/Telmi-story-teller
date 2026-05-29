#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "system/system.h"
#include "system/keymap_hw.h"
#include "system/settings.h"
#include "system/settings_sync.h"
#include "system/display.h"

#include "./logs_helper.h"
#include "./time_helper.h"
#include "./app_lock.h"
#include "./app_brightness.h"
#include "./app_volume.h"
#include "./app_autosleep.h"
#include "./sdl_helper.h"
#include "./app_selector.h"
#include "./app_parameters.h"
#include "./app_usage_tracker.h"

// for ev.value
#define RELEASED 0
#define PRESSED 1
#define REPEAT 2

// Global Variables
static int input_fd;
static struct input_event ev;
static struct pollfd fds[1];

#ifdef PLATFORM_HOST
// SDL2_mixer < 2.6 (e.g., Ubuntu 22.04) lacks these symbols. Provide
// minimal fallbacks so the host build links; music duration/position
// UI just won't be accurate in that case.
__attribute__((weak)) double Mix_MusicDuration(Mix_Music *m) { (void)m; return -1.0; }
__attribute__((weak)) double Mix_GetMusicPosition(Mix_Music *m) { (void)m; return 0.0; }
#endif

#ifdef PLATFORM_HOST
// Map SDL keysyms to linux/input.h keycodes used as HW_BTN_* values.
static int host_sdl_to_hwcode(SDL_Keycode k)
{
    switch (k) {
        case SDLK_UP:        return HW_BTN_UP;
        case SDLK_DOWN:      return HW_BTN_DOWN;
        case SDLK_LEFT:      return HW_BTN_LEFT;
        case SDLK_RIGHT:     return HW_BTN_RIGHT;
        case SDLK_SPACE:     return HW_BTN_A;
        case SDLK_LCTRL:     return HW_BTN_B;
        case SDLK_LSHIFT:    return HW_BTN_X;
        case SDLK_LALT:      return HW_BTN_Y;
        case SDLK_e:         return HW_BTN_L1;
        case SDLK_t:         return HW_BTN_R1;
        case SDLK_TAB:       return HW_BTN_L2;
        case SDLK_BACKSPACE: return HW_BTN_R2;
        case SDLK_RCTRL:     return HW_BTN_SELECT;
        case SDLK_RETURN:    return HW_BTN_START;
        case SDLK_ESCAPE:    return HW_BTN_MENU;
        case SDLK_END:       return HW_BTN_POWER;
        case SDLK_PAGEUP:    return HW_BTN_VOLUME_UP;
        case SDLK_PAGEDOWN:  return HW_BTN_VOLUME_DOWN;
        default:             return -1;
    }
}

// Returns true if `ev` was populated with a valid PRESSED/RELEASED key event.
static bool host_poll_input(void)
{
    SDL_Event sev;
    while (SDL_PollEvent(&sev)) {
        if (sev.type == SDL_QUIT) {
            ev.type = EV_KEY;
            ev.code = HW_BTN_POWER;
            ev.value = PRESSED;
            // Force long-press detection in main loop to trigger shutdown.
            return true;
        }
        if (sev.type != SDL_KEYDOWN && sev.type != SDL_KEYUP) {
            continue;
        }
        // Ignore key auto-repeat from the OS; original code treats REPEAT
        // like PRESSED but downstream logic mostly only acts on RELEASED.
        if (sev.type == SDL_KEYDOWN && sev.key.repeat) {
            continue;
        }
        int code = host_sdl_to_hwcode(sev.key.keysym.sym);
        if (code < 0) {
            continue;
        }
        ev.type = EV_KEY;
        ev.code = code;
        ev.value = (sev.type == SDL_KEYDOWN) ? PRESSED : RELEASED;
        return true;
    }
    return false;
}
#endif

bool keyinput_isValid(void) {
    read(input_fd, &ev, sizeof(ev));

    if (ev.type != EV_KEY || ev.value > REPEAT) {
        return false;
    }

    return true;
}

// Callback fired when the parental lock code is verified successfully.
static void onCodeVerified(CodeEntryResult result) {
    if (result == CODE_RESULT_VERIFIED) {
        settingsmenu_open();
    }
}

// Decide what to do when the L2+R2 combo is released: either prompt for
// the parental code (if the lock is on) or open the menu directly.
static void requestSettingsMenu(void) {
    if (parameters_getParentalLockEnabled()) {
        codeentry_open(CODE_MODE_VERIFY, onCodeVerified);
    } else {
        settingsmenu_open();
    }
}

int main(int argc, char *argv[]) {
    srand(time(NULL));
    display_init();
    video_audio_init();
    settings_init();
    parameters_init();
    usage_init();
    settings_setVolume(parameters_getAudioVolumeStartup(), true);
    settings_setBrightness(parameters_getScreenBrightnessStartup(), true, false);

    autosleep_init(parameters_getScreenOnInactivityTime(), parameters_getScreenOffInactivityTime());
    app_init();

#ifdef PLATFORM_HOST
    input_fd = -1;
#else
    input_fd = open("/dev/input/event0", O_RDONLY);
    memset(&fds, 0, sizeof(fds));
    fds[0].fd = input_fd;
    fds[0].events = POLLIN;
#endif

    bool isMenuPressed = false;
    bool menuPreventDefault = false;
    bool startPowerPressed = false;
    long startPowerPressedTime = 0;

    // L2+R2 combo state (held together >= 1s opens settings menu)
    bool isL2Pressed = false;
    bool isR2Pressed = false;
    long int l2r2ComboStartTime = 0;
    bool l2r2ComboFired = false;

    // Throttle to 1Hz refresh whenever a sleep timer is running, so the
    // top-bar countdown ticks even when nothing else (idle / paused) is
    // driving redraws. Once-per-second is light enough that it doesn't
    // re-introduce the music-player flicker on the Miyoo.
    long int sleepTimerLastRefresh = 0;

    while (1) {
        usage_tick();
        if (autosleep_isSleepingTime() || sleeptimer_isExpired() ||
                (startPowerPressed && (get_time() - startPowerPressedTime) > 1)) {
            goto exit_loop;
        }

        bool forceRefreshScreen = applock_checkLock();
        forceRefreshScreen = app_volume_checkDisplay() || forceRefreshScreen;
        forceRefreshScreen = app_brightness_checkDisplay() || forceRefreshScreen;
        app_update();

        // L2+R2 held >= 1s (without MENU) opens the settings flow.
        if (isL2Pressed && isR2Pressed && !isMenuPressed && !l2r2ComboFired
                && l2r2ComboStartTime > 0
                && (get_time() - l2r2ComboStartTime) >= 1) {
            requestSettingsMenu();
            l2r2ComboFired = true;
            forceRefreshScreen = true;
            sleepTimerLastRefresh = get_time();
        }

        // 1Hz forced refresh while a sleep timer is running, regardless of
        // whether anything is playing — keeps the top-bar countdown alive.
        // Same idea while the limit overlay is showing so it auto-dismisses.
        if (sleeptimer_isActive() || limit_overlay_isActive()) {
            long int now = get_time();
            if (now != sleepTimerLastRefresh) {
                sleepTimerLastRefresh = now;
                forceRefreshScreen = true;
            }
        }

#ifdef PLATFORM_HOST
        SDL_Delay(10);
        if (host_poll_input()) {
#else
        if (poll(fds, 1, 0) > 0) {
            if (!keyinput_isValid()) {
                continue;
            }
#endif

            switch (ev.value) {
                case PRESSED:
                    switch (ev.code) {
                        case HW_BTN_MENU :
                            isMenuPressed = true;
                            forceRefreshScreen = applock_startTimer() || forceRefreshScreen;
                            if (applock_isLocked() || settingsmenu_isOpen()) {
                                menuPreventDefault = true;
                            }
                            break;
                        case HW_BTN_POWER :
                            if (!applock_isLocked()) {
                                startPowerPressedTime = get_time();
                                startPowerPressed = true;
                            }
                            break;
                        case HW_BTN_L2 :
                            isL2Pressed = true;
                            if (isR2Pressed && l2r2ComboStartTime == 0) {
                                l2r2ComboStartTime = get_time();
                                l2r2ComboFired = false;
                            }
                            break;
                        case HW_BTN_R2 :
                            isR2Pressed = true;
                            if (isL2Pressed && l2r2ComboStartTime == 0) {
                                l2r2ComboStartTime = get_time();
                                l2r2ComboFired = false;
                            }
                            break;
                    }
                    break;

                case RELEASED:
                    if (applock_isLocked()) {
                        if (ev.code == HW_BTN_MENU) {
                            forceRefreshScreen = applock_stopTimer() || forceRefreshScreen;
                        }
                        break;
                    }
                    autosleep_keepAwake();

                    // Code-entry overlay has top priority and consumes input.
                    if (codeentry_isOpen()) {
                        switch (ev.code) {
                            case HW_BTN_UP:    codeentry_handleUp();    break;
                            case HW_BTN_DOWN:  codeentry_handleDown();  break;
                            case HW_BTN_LEFT:  codeentry_handleLeft();  break;
                            case HW_BTN_RIGHT: codeentry_handleRight(); break;
                            case HW_BTN_A:     codeentry_handleA();     break;
                            case HW_BTN_B:
                            case HW_BTN_MENU:  codeentry_handleB();     break;
                            case HW_BTN_L2:
                                isL2Pressed = false;
                                l2r2ComboStartTime = 0;
                                l2r2ComboFired = false;
                                break;
                            case HW_BTN_R2:
                                isR2Pressed = false;
                                l2r2ComboStartTime = 0;
                                l2r2ComboFired = false;
                                break;
                            default: break;
                        }
                        forceRefreshScreen = true;
                        break;
                    }

                    // Settings menu intercepts all inputs while open.
                    if (settingsmenu_isOpen()) {
                        switch (ev.code) {
                            case HW_BTN_UP:    settingsmenu_handleUp();    break;
                            case HW_BTN_DOWN:  settingsmenu_handleDown();  break;
                            case HW_BTN_LEFT:  settingsmenu_handleLeft();  break;
                            case HW_BTN_RIGHT: settingsmenu_handleRight(); break;
                            case HW_BTN_A:     settingsmenu_handleA();     break;
                            case HW_BTN_X:     settingsmenu_handleX();     break;
                            case HW_BTN_B:
                                settingsmenu_close();
                                break;
                            case HW_BTN_MENU:
                                settingsmenu_close();
                                isMenuPressed = false;
                                menuPreventDefault = false;
                                forceRefreshScreen = applock_stopTimer() || forceRefreshScreen;
                                break;
                            case HW_BTN_L2:
                                isL2Pressed = false;
                                l2r2ComboStartTime = 0;
                                l2r2ComboFired = false;
                                break;
                            case HW_BTN_R2:
                                isR2Pressed = false;
                                l2r2ComboStartTime = 0;
                                l2r2ComboFired = false;
                                break;
                            default: break;
                        }
                        forceRefreshScreen = true;
                        break;
                    }

                    switch (ev.code) {
                        case HW_BTN_POWER :
                            startPowerPressed = false;
                            break;
                        case HW_BTN_MENU :
                            if (!menuPreventDefault) {
                                app_menu();
                            }
                            isMenuPressed = false;
                            menuPreventDefault = false;
                            forceRefreshScreen = applock_stopTimer() || forceRefreshScreen;
                            break;
                        case HW_BTN_LEFT :
                            app_previous();
                            break;
                        case HW_BTN_RIGHT :
                            app_next();
                            break;
                        case HW_BTN_UP :
                            app_up();
                            break;
                        case HW_BTN_DOWN :
                            app_down();
                            break;
                        case HW_BTN_START :
                        case HW_BTN_SELECT :
                            app_pause();
                            break;
                        case HW_BTN_A :
                        case HW_BTN_B :
                            app_ok();
                            break;
                        case HW_BTN_Y :
                        case HW_BTN_X :
                            app_home();
                            break;
                        case HW_BTN_L2 :
                            isL2Pressed = false;
                            l2r2ComboStartTime = 0;
                            l2r2ComboFired = false;
                            break;
                        case HW_BTN_R2 :
                            isR2Pressed = false;
                            l2r2ComboStartTime = 0;
                            l2r2ComboFired = false;
                            break;
                    }

                    if (isMenuPressed) {
                        switch (ev.code) {
                            case HW_BTN_L2 :
                            case HW_BTN_VOLUME_DOWN :
                                forceRefreshScreen = app_brightness_down();
                                applock_stopTimer();
                                menuPreventDefault = true;
                                break;
                            case HW_BTN_R2 :
                            case HW_BTN_VOLUME_UP :
                                forceRefreshScreen = app_brightness_up();
                                applock_stopTimer();
                                menuPreventDefault = true;
                                break;
                            default:
                                break;
                        }
                    } else {
                        switch (ev.code) {
                            case HW_BTN_VOLUME_DOWN :
                                forceRefreshScreen = app_volume_down();
                                break;
                            case HW_BTN_VOLUME_UP :
                                forceRefreshScreen = app_volume_up();
                                break;
                            default:
                                break;
                        }
                    }
                    break;

                default:
                    break;
            }
        }

        if(forceRefreshScreen) {
            app_forceRefreshScreen();
        }
    }

    exit_loop:
    usage_save();
    app_save();
    display_setScreen(true);
    video_audio_quit();
    system_shutdown();
    return EXIT_SUCCESS;
}
