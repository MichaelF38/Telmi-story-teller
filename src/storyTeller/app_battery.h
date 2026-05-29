#ifndef STORYTELLER_BATTERY__
#define STORYTELLER_BATTERY__

#include "system/battery.h"
#include "./time_helper.h"

static int app_battery_percentage = 0;
static long int app_battery_percentageTime = 0;

static int app_battery_getPercentage(void)
{
    long int time = get_time();
    if(app_battery_percentageTime < time) {
        app_battery_percentageTime = time + 10;
        int p = battery_getPercentage();
        // A read of 0 almost always means a transient failure (batmon not
        // yet running, /tmp/percBat missing, or CPU-starved). Keep the
        // last known good value instead of caching a misleading 0% / red
        // empty-battery icon for the next 10s.
        if (p > 0 || app_battery_percentage == 0) {
            app_battery_percentage = p;
        }
    }
    return app_battery_percentage;
}

#endif // STORYTELLER_BATTERY__