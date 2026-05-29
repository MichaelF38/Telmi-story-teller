#ifndef STORYTELLER_APP_PARAMETERS__
#define STORYTELLER_APP_PARAMETERS__

#include <string.h>
#include "utils/json.h"

static double app_parameters_audioVolumeStartup = 0.3;
static double app_parameters_audioVolumeMax = 0.6;
static double app_parameters_systemAudioVolumeMax = 25.0;
static double app_parameters_screenBrightnessStartup = 0.3;
static double app_parameters_screenBrightnessMax = 0.6;
static double app_parameters_systemScreenBrightnessMax = 10.0;
static int app_parameters_screenOnInactivityTime = 120;
static int app_parameters_screenOffInactivityTime = 300;
static int app_parameters_musicInactivityTime = 3600;
static bool app_parameters_storyDisplayTiles = false;
static bool app_parameters_storyDisableNightMode = false;
static bool app_parameters_storyDisableTimeline = false;
static bool app_parameters_musicDisableRepeatModes = false;
static bool app_parameters_parentalLockEnabled = false;
static char app_parameters_parentalLockCode[5] = "0000";
static int  app_parameters_dailyUsageLimitMinutes = 0; // 0 = unlimited

#define APP_PARAMETERS_PATH "/mnt/SDCARD/Saves/.parameters"

int parameters_getAudioVolumeStartup() {
    return (int) (app_parameters_audioVolumeStartup * app_parameters_systemAudioVolumeMax + 0.5);
}

int parameters_getAudioVolumeMax() {
    return (int) (app_parameters_audioVolumeMax * app_parameters_systemAudioVolumeMax + 0.5);
}

int parameters_getSystemAudioVolumeMax(void) {
    return app_parameters_systemAudioVolumeMax;
}

int parameters_getAudioVolumeValidation(int audioVolume) {
    return audioVolume > parameters_getAudioVolumeMax() ? parameters_getAudioVolumeMax() : audioVolume;
}

int parameters_getScreenBrightnessStartup() {
    return (int) (app_parameters_screenBrightnessStartup * app_parameters_systemScreenBrightnessMax + 0.5);
}

int parameters_getScreenBrightnessMax() {
    return (int) (app_parameters_screenBrightnessMax * app_parameters_systemScreenBrightnessMax + 0.5);
}

int parameters_getSystemScreenBrightnessMax(void) {
    return app_parameters_systemScreenBrightnessMax;
}

int parameters_getScreenBrightnessValidation(int brightness) {
    return brightness > parameters_getScreenBrightnessMax() ? parameters_getScreenBrightnessMax() : brightness;
}

int parameters_getScreenOnInactivityTime() {
    return app_parameters_screenOnInactivityTime;
}

int parameters_getScreenOffInactivityTime() {
    return app_parameters_screenOffInactivityTime;
}

int parameters_getMusicInactivityTime() {
    return app_parameters_musicInactivityTime;
}

bool parameters_getStoryDisplayNine() {
    return app_parameters_storyDisplayTiles;
}

bool parameters_getStoryDisableNightMode() {
    return app_parameters_storyDisableNightMode;
}

bool parameters_getStoryDisableTimeline() {
    return app_parameters_storyDisableTimeline;
}

bool parameters_getMusicDisableRepeatModes() {
    return app_parameters_musicDisableRepeatModes;
}

bool parameters_getParentalLockEnabled(void) {
    return app_parameters_parentalLockEnabled;
}

const char *parameters_getParentalLockCode(void) {
    return app_parameters_parentalLockCode;
}

int parameters_getDailyUsageLimitMinutes(void) {
    return app_parameters_dailyUsageLimitMinutes;
}

void parameters_setAudioVolumeMaxRaw(int raw) {
    if (raw < 1) raw = 1;
    if (raw > (int)app_parameters_systemAudioVolumeMax) raw = (int)app_parameters_systemAudioVolumeMax;
    app_parameters_audioVolumeMax = (double)raw / app_parameters_systemAudioVolumeMax;
}

void parameters_setScreenBrightnessMaxRaw(int raw) {
    if (raw < 1) raw = 1;
    if (raw > (int)app_parameters_systemScreenBrightnessMax) raw = (int)app_parameters_systemScreenBrightnessMax;
    app_parameters_screenBrightnessMax = (double)raw / app_parameters_systemScreenBrightnessMax;
}

void parameters_setParentalLockEnabled(bool enabled) {
    app_parameters_parentalLockEnabled = enabled;
}

void parameters_setParentalLockCode(const char *code) {
    if (code == NULL) return;
    strncpy(app_parameters_parentalLockCode, code, 4);
    app_parameters_parentalLockCode[4] = '\0';
}

void parameters_setDailyUsageLimitMinutes(int minutes) {
    if (minutes < 0) minutes = 0;
    if (minutes > 1440) minutes = 1440;
    app_parameters_dailyUsageLimitMinutes = minutes;
}

static void parameters_jsonReplaceNumber(cJSON *root, const char *key, double value) {
    cJSON *item = cJSON_GetObjectItem(root, key);
    if (item) {
        cJSON_DeleteItemFromObject(root, key);
    }
    cJSON_AddNumberToObject(root, key, value);
}

static void parameters_jsonReplaceBool(cJSON *root, const char *key, bool value) {
    cJSON *item = cJSON_GetObjectItem(root, key);
    if (item) {
        cJSON_DeleteItemFromObject(root, key);
    }
    cJSON_AddBoolToObject(root, key, value);
}

static void parameters_jsonReplaceString(cJSON *root, const char *key, const char *value) {
    cJSON *item = cJSON_GetObjectItem(root, key);
    if (item) {
        cJSON_DeleteItemFromObject(root, key);
    }
    cJSON_AddStringToObject(root, key, value);
}

void parameters_save(void) {
    cJSON *root = json_load(APP_PARAMETERS_PATH);
    if (root == NULL) {
        root = cJSON_CreateObject();
    }
    parameters_jsonReplaceNumber(root, "audioVolumeMax", app_parameters_audioVolumeMax);
    parameters_jsonReplaceNumber(root, "screenBrightnessMax", app_parameters_screenBrightnessMax);
    parameters_jsonReplaceBool(root, "parentalLockEnabled", app_parameters_parentalLockEnabled);
    parameters_jsonReplaceString(root, "parentalLockCode", app_parameters_parentalLockCode);
    parameters_jsonReplaceNumber(root, "dailyUsageLimitMinutes", app_parameters_dailyUsageLimitMinutes);
    json_save(root, APP_PARAMETERS_PATH);
    cJSON_Delete(root);
}

void parameters_init(void) {
    cJSON *parameters = json_load(APP_PARAMETERS_PATH);
    if (parameters != NULL) {
        json_getDouble(parameters, "audioVolumeStartup", &app_parameters_audioVolumeStartup);
        json_getDouble(parameters, "audioVolumeMax", &app_parameters_audioVolumeMax);
        json_getDouble(parameters, "screenBrightnessStartup", &app_parameters_screenBrightnessStartup);
        json_getDouble(parameters, "screenBrightnessMax", &app_parameters_screenBrightnessMax);
        json_getInt(parameters, "screenOnInactivityTime", &app_parameters_screenOnInactivityTime);
        json_getInt(parameters, "screenOffInactivityTime", &app_parameters_screenOffInactivityTime);
        json_getInt(parameters, "musicInactivityTime", &app_parameters_musicInactivityTime);
        if (!cJSON_IsNull(cJSON_GetObjectItem(parameters, "storyDisplayTiles"))) {
            json_getBool(parameters, "storyDisplayTiles", &app_parameters_storyDisplayTiles);
        }
        if (!cJSON_IsNull(cJSON_GetObjectItem(parameters, "storyDisableNightMode"))) {
            json_getBool(parameters, "storyDisableNightMode", &app_parameters_storyDisableNightMode);
        }
        if (!cJSON_IsNull(cJSON_GetObjectItem(parameters, "storyDisableTimeline"))) {
            json_getBool(parameters, "storyDisableTimeline", &app_parameters_storyDisableTimeline);
        }
        if (!cJSON_IsNull(cJSON_GetObjectItem(parameters, "musicDisableRepeatModes"))) {
            json_getBool(parameters, "musicDisableRepeatModes", &app_parameters_musicDisableRepeatModes);
        }
        if (!cJSON_IsNull(cJSON_GetObjectItem(parameters, "parentalLockEnabled"))) {
            json_getBool(parameters, "parentalLockEnabled", &app_parameters_parentalLockEnabled);
        }
        cJSON *codeItem = cJSON_GetObjectItem(parameters, "parentalLockCode");
        if (codeItem && cJSON_IsString(codeItem)) {
            const char *s = cJSON_GetStringValue(codeItem);
            int n = 0;
            for (int i = 0; i < 4 && s[i] >= '0' && s[i] <= '9'; ++i) n++;
            if (n == 4) {
                memcpy(app_parameters_parentalLockCode, s, 4);
                app_parameters_parentalLockCode[4] = '\0';
            }
        }
        json_getInt(parameters, "dailyUsageLimitMinutes", &app_parameters_dailyUsageLimitMinutes);
        cJSON_Delete(parameters);
    }
}

#endif // STORYTELLER_APP_PARAMETERS__