#ifndef STORYTELLER_APP_SETTINGS_MENU__
#define STORYTELLER_APP_SETTINGS_MENU__

#include <stdbool.h>
#include <stdio.h>
#include <stddef.h>

#include "./app_sleep_timer.h"
#include "./app_parameters.h"
#include "./app_volume.h"
#include "./app_brightness.h"
#include "./app_code_entry.h"

// NOTE: relies on video_*, font*, color* from sdl_helper.h. Included at
// the bottom of sdl_helper.h so those symbols are visible.

typedef enum {
    SETTINGS_ROW_SLEEP_TIMER = 0,
    SETTINGS_ROW_MAX_VOLUME,
    SETTINGS_ROW_MAX_BRIGHTNESS,
    SETTINGS_ROW_DAILY_LIMIT,
    SETTINGS_ROW_PARENTAL_LOCK,
    SETTINGS_ROW_CHANGE_CODE,
    SETTINGS_ROW_COUNT
} SettingsRow;

static bool settings_isOpen           = false;
static int  settings_stagedTimerMin   = 0;     // 0 = Off, otherwise 5..60
static bool settings_timerRowChanged  = false; // commit timer only if user touched it
static int  settings_selectedRow      = SETTINGS_ROW_SLEEP_TIMER;
static bool settings_dirty            = false;

static int settingsmenu_roundToFive(int m) {
    int r = ((m + 2) / 5) * 5;
    if (r < 5)  r = 5;
    if (r > 60) r = 60;
    return r;
}

void settingsmenu_open(void) {
    settings_isOpen           = true;
    settings_selectedRow      = SETTINGS_ROW_SLEEP_TIMER;
    settings_dirty            = false;
    settings_timerRowChanged  = false;
    if (sleeptimer_isActive()) {
        int m = (int)((sleeptimer_getRemainingSeconds() + 30) / 60);
        settings_stagedTimerMin = settingsmenu_roundToFive(m);
    } else {
        settings_stagedTimerMin = 0;
    }
}

void settingsmenu_close(void) {
    settings_isOpen = false;
    if (settings_timerRowChanged) {
        if (settings_stagedTimerMin == 0) {
            sleeptimer_cancel();
        } else {
            sleeptimer_set(settings_stagedTimerMin);
        }
        settings_timerRowChanged = false;
    }
    if (settings_dirty) {
        parameters_save();
        settings_dirty = false;
    }
}

bool settingsmenu_isOpen(void) {
    return settings_isOpen;
}

static void settingsmenu_adjustSleepTimer(int delta) {
    int v = settings_stagedTimerMin + delta * 5;
    if (v < 0)  v = 0;
    if (v > 60) v = 60;
    if (v == settings_stagedTimerMin) return;
    settings_stagedTimerMin = v;
    settings_timerRowChanged = true;
}

static void settingsmenu_adjustMaxVolume(int delta) {
    int sysMax = parameters_getSystemAudioVolumeMax();
    int cur    = parameters_getAudioVolumeMax();
    int next   = cur + delta;
    if (next < 1)      next = 1;
    if (next > sysMax) next = sysMax;
    if (next == cur)   return;
    parameters_setAudioVolumeMaxRaw(next);
    if (app_volume_getCurrent() > next) {
        settings_setVolume(next, true);
    }
    settings_dirty = true;
}

static void settingsmenu_adjustMaxBrightness(int delta) {
    int sysMax = parameters_getSystemScreenBrightnessMax();
    int cur    = parameters_getScreenBrightnessMax();
    int next   = cur + delta;
    if (next < 1)      next = 1;
    if (next > sysMax) next = sysMax;
    if (next == cur)   return;
    parameters_setScreenBrightnessMaxRaw(next);
    if (app_brightness_getCurrent() > next) {
        settings_setBrightness(next, true, false);
    }
    settings_dirty = true;
}

static void settingsmenu_adjustDailyLimit(int delta) {
    int cur  = parameters_getDailyUsageLimitMinutes();
    int step = 15;
    int next = cur + delta * step;
    if (cur == 0 && delta > 0) next = 15;
    if (next < 0)   next = 0;
    if (next > 480) next = 480;
    if (next == cur) return;
    parameters_setDailyUsageLimitMinutes(next);
    settings_dirty = true;
}

static void settingsmenu_adjustParentalLock(int delta) {
    (void)delta;
    parameters_setParentalLockEnabled(!parameters_getParentalLockEnabled());
    settings_dirty = true;
}

void settingsmenu_handleLeft(void) {
    switch (settings_selectedRow) {
        case SETTINGS_ROW_SLEEP_TIMER:    settingsmenu_adjustSleepTimer(-1);    break;
        case SETTINGS_ROW_MAX_VOLUME:     settingsmenu_adjustMaxVolume(-1);     break;
        case SETTINGS_ROW_MAX_BRIGHTNESS: settingsmenu_adjustMaxBrightness(-1); break;
        case SETTINGS_ROW_DAILY_LIMIT:    settingsmenu_adjustDailyLimit(-1);    break;
        case SETTINGS_ROW_PARENTAL_LOCK:  settingsmenu_adjustParentalLock(-1);  break;
        default: break;
    }
}

void settingsmenu_handleRight(void) {
    switch (settings_selectedRow) {
        case SETTINGS_ROW_SLEEP_TIMER:    settingsmenu_adjustSleepTimer(+1);    break;
        case SETTINGS_ROW_MAX_VOLUME:     settingsmenu_adjustMaxVolume(+1);     break;
        case SETTINGS_ROW_MAX_BRIGHTNESS: settingsmenu_adjustMaxBrightness(+1); break;
        case SETTINGS_ROW_DAILY_LIMIT:    settingsmenu_adjustDailyLimit(+1);    break;
        case SETTINGS_ROW_PARENTAL_LOCK:  settingsmenu_adjustParentalLock(+1);  break;
        default: break;
    }
}

void settingsmenu_handleUp(void) {
    settings_selectedRow = (settings_selectedRow + SETTINGS_ROW_COUNT - 1) % SETTINGS_ROW_COUNT;
}

void settingsmenu_handleDown(void) {
    settings_selectedRow = (settings_selectedRow + 1) % SETTINGS_ROW_COUNT;
}

void settingsmenu_handleA(void) {
    if (settings_selectedRow == SETTINGS_ROW_CHANGE_CODE) {
        codeentry_open(CODE_MODE_SET, NULL);
    }
    // Sleep-timer row: A is a no-op; changes are committed on close.
}

void settingsmenu_handleX(void) {
    // X is reserved (no-op for now); timer Off is set via LEFT.
}

static void settingsmenu_formatRowValue(SettingsRow row, char *out, size_t outSize) {
    switch (row) {
        case SETTINGS_ROW_SLEEP_TIMER:
            if (settings_stagedTimerMin == 0) {
                snprintf(out, outSize, "Off");
            } else {
                snprintf(out, outSize, "%d min", settings_stagedTimerMin);
            }
            break;

        case SETTINGS_ROW_MAX_VOLUME: {
            int sysMax = parameters_getSystemAudioVolumeMax();
            int cur    = parameters_getAudioVolumeMax();
            int pct    = (sysMax > 0) ? (cur * 100 + sysMax / 2) / sysMax : 0;
            snprintf(out, outSize, "%d%%", pct);
            break;
        }
        case SETTINGS_ROW_MAX_BRIGHTNESS: {
            int sysMax = parameters_getSystemScreenBrightnessMax();
            int cur    = parameters_getScreenBrightnessMax();
            int pct    = (sysMax > 0) ? (cur * 100 + sysMax / 2) / sysMax : 0;
            snprintf(out, outSize, "%d%%", pct);
            break;
        }
        case SETTINGS_ROW_DAILY_LIMIT: {
            int m = parameters_getDailyUsageLimitMinutes();
            if (m <= 0)              snprintf(out, outSize, "Off");
            else if (m < 60)         snprintf(out, outSize, "%d min", m);
            else if (m % 60 == 0)    snprintf(out, outSize, "%dh", m / 60);
            else                     snprintf(out, outSize, "%dh%02d", m / 60, m % 60);
            break;
        }
        case SETTINGS_ROW_PARENTAL_LOCK:
            snprintf(out, outSize, "%s", parameters_getParentalLockEnabled() ? "Oui" : "Non");
            break;
        case SETTINGS_ROW_CHANGE_CODE:
            snprintf(out, outSize, "[A]");
            break;
        default:
            if (outSize > 0) out[0] = '\0';
            break;
    }
}

static const char *settingsmenu_rowLabel(SettingsRow row) {
    switch (row) {
        case SETTINGS_ROW_SLEEP_TIMER:    return "Minuteur";
        case SETTINGS_ROW_MAX_VOLUME:     return "Volume max";
        case SETTINGS_ROW_MAX_BRIGHTNESS: return "Luminosité max";
        case SETTINGS_ROW_DAILY_LIMIT:    return "Limite quotidienne";
        case SETTINGS_ROW_PARENTAL_LOCK:  return "Verrou parental";
        case SETTINGS_ROW_CHANGE_CODE:    return "Changer code";
        default: return "";
    }
}

void settingsmenu_draw(void) {
    int px = 70,  py = 40;
    int pw = 500, ph = 400;

    video_drawRectangle(px, py, pw, ph, 37, 16, 58);
    video_drawRectangle(px,          py,           pw, 2,  255, 255, 255);
    video_drawRectangle(px,          py + ph - 2,  pw, 2,  255, 255, 255);
    video_drawRectangle(px,          py,           2,  ph, 255, 255, 255);
    video_drawRectangle(px + pw - 2, py,           2,  ph, 255, 255, 255);

    video_screenWriteFont("Réglages", fontBold24, colorWhite, 320, py + 18, SDL_ALIGN_CENTER);
    video_drawRectangle(px + 2, py + 55, pw - 4, 1, 255, 255, 255);

    int rowY0 = py + 75;
    int rowH  = 38;
    int labelX = px + 24;
    int valueRightX = px + pw - 24;

    for (int i = 0; i < SETTINGS_ROW_COUNT; ++i) {
        int y = rowY0 + i * rowH;
        bool selected = (i == settings_selectedRow);

        if (selected) {
            video_drawRectangle(px + 6, y - 2, pw - 12, rowH - 4, 60, 30, 90);
        }

        const char *label = settingsmenu_rowLabel((SettingsRow)i);
        SDL_Color labelColor = selected ? colorWhite : colorWhite60;
        video_screenWriteFont(label, fontRegular20, labelColor, labelX, y + 6, SDL_ALIGN_LEFT);

        char valueStr[24];
        settingsmenu_formatRowValue((SettingsRow)i, valueStr, sizeof(valueStr));

        if (selected) {
            bool adjustable = (i != SETTINGS_ROW_CHANGE_CODE);
            if (adjustable) {
                video_screenWriteFont("<", fontBold20, colorOrange,
                                      valueRightX - 120, y + 6, SDL_ALIGN_LEFT);
                video_screenWriteFont(">", fontBold20, colorOrange,
                                      valueRightX, y + 6, SDL_ALIGN_RIGHT);
            }
            video_screenWriteFont(valueStr, fontBold20, colorOrange,
                                  valueRightX - 18, y + 6, SDL_ALIGN_RIGHT);
        } else {
            video_screenWriteFont(valueStr, fontRegular20, colorWhite60,
                                  valueRightX, y + 6, SDL_ALIGN_RIGHT);
        }
    }

    const char *hint;
    if (settings_selectedRow == SETTINGS_ROW_SLEEP_TIMER) {
        hint = "<-/-> Off / +5 min   démarre à la fermeture   [B] Fermer";
    } else if (settings_selectedRow == SETTINGS_ROW_CHANGE_CODE) {
        hint = "[A] Modifier le code   [B] Fermer";
    } else {
        hint = "HAUT/BAS naviguer   <-/-> modifier   [B] Fermer";
    }
    video_screenWriteFont(hint, fontRegular16, colorWhite60,
                          320, py + ph - 26, SDL_ALIGN_CENTER);
}

#endif // STORYTELLER_APP_SETTINGS_MENU__
