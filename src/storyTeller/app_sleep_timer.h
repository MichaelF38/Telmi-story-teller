#ifndef STORYTELLER_APP_SLEEP_TIMER__
#define STORYTELLER_APP_SLEEP_TIMER__

#include <stdbool.h>
#include <time.h>

static bool  app_sleeptimer_active = false;
static long int app_sleeptimer_target = 0;

void sleeptimer_set(int minutes) {
    app_sleeptimer_target = (long int)time(0) + (long int)(minutes * 60);
    app_sleeptimer_active = true;
}

void sleeptimer_cancel(void) {
    app_sleeptimer_active = false;
    app_sleeptimer_target = 0;
}

bool sleeptimer_isActive(void) {
    return app_sleeptimer_active;
}

bool sleeptimer_isExpired(void) {
    return app_sleeptimer_active && (long int)time(0) >= app_sleeptimer_target;
}

long int sleeptimer_getRemainingSeconds(void) {
    if (!app_sleeptimer_active) {
        return 0;
    }
    long int remaining = app_sleeptimer_target - (long int)time(0);
    return remaining > 0 ? remaining : 0;
}

#endif // STORYTELLER_APP_SLEEP_TIMER__
