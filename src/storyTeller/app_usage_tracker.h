#ifndef STORYTELLER_APP_USAGE_TRACKER__
#define STORYTELLER_APP_USAGE_TRACKER__

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "utils/json.h"
#include "./app_parameters.h"

#define APP_USAGE_PATH "/mnt/SDCARD/Saves/.usage"

static char     app_usage_date[16] = "";      // YYYY-MM-DD of stored counter
static long int app_usage_seconds = 0;        // accumulated seconds today
static long int app_usage_lastTick = 0;       // last wall-clock second seen
static long int app_usage_lastSaveTime = 0;   // last persist (epoch)

static void usage_today(char *out16)
{
    time_t now = time(NULL);
    struct tm *lt = localtime(&now);
    strftime(out16, 16, "%Y-%m-%d", lt);
}

void usage_init(void)
{
    char today[16];
    usage_today(today);

    cJSON *root = json_load(APP_USAGE_PATH);
    if (root != NULL) {
        char stored[16] = "";
        cJSON *d = cJSON_GetObjectItem(root, "date");
        if (d && cJSON_IsString(d)) {
            strncpy(stored, cJSON_GetStringValue(d), sizeof(stored) - 1);
        }
        int s = 0;
        json_getInt(root, "seconds", &s);
        if (strcmp(stored, today) == 0) {
            app_usage_seconds = s;
        } else {
            app_usage_seconds = 0;
        }
        cJSON_Delete(root);
    }

    strncpy(app_usage_date, today, sizeof(app_usage_date) - 1);
    app_usage_lastTick = (long int)time(NULL);
    app_usage_lastSaveTime = app_usage_lastTick;
}

void usage_save(void)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "date", app_usage_date);
    cJSON_AddNumberToObject(root, "seconds", app_usage_seconds);
    json_save(root, APP_USAGE_PATH);
    cJSON_Delete(root);
    app_usage_lastSaveTime = (long int)time(NULL);
}

// Call from main loop. Accumulates wall-clock seconds since last tick,
// rolls over at midnight, persists at most once per minute.
void usage_tick(void)
{
    long int now = (long int)time(NULL);
    if (app_usage_lastTick == 0) {
        app_usage_lastTick = now;
        return;
    }
    long int delta = now - app_usage_lastTick;
    app_usage_lastTick = now;
    if (delta <= 0) return;
    // Cap delta to prevent huge jumps from clock changes / suspend.
    if (delta > 60) delta = 60;

    char today[16];
    usage_today(today);
    if (strcmp(today, app_usage_date) != 0) {
        strncpy(app_usage_date, today, sizeof(app_usage_date) - 1);
        app_usage_seconds = 0;
    }
    app_usage_seconds += delta;

    if (now - app_usage_lastSaveTime >= 60) {
        usage_save();
    }
}

long int usage_getSecondsToday(void)
{
    return app_usage_seconds;
}

bool usage_isLimitReached(void)
{
    int limit = parameters_getDailyUsageLimitMinutes();
    if (limit <= 0) return false;
    return app_usage_seconds >= (long int)limit * 60;
}

#endif // STORYTELLER_APP_USAGE_TRACKER__
