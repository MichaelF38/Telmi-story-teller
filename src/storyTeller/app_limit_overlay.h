#ifndef STORYTELLER_APP_LIMIT_OVERLAY__
#define STORYTELLER_APP_LIMIT_OVERLAY__

#include <stdbool.h>

#include "./time_helper.h"

// NOTE: relies on video_*, color* from sdl_helper.h. Included AFTER those
// are defined (same pattern as app_settings_menu / app_code_entry).

#define LIMIT_OVERLAY_DURATION_S 3

static long int limit_overlay_until = 0;

void limit_overlay_show(void) {
    limit_overlay_until = get_time() + LIMIT_OVERLAY_DURATION_S;
}

bool limit_overlay_isActive(void) {
    return get_time() < limit_overlay_until;
}

void limit_overlay_draw(void) {
    // Centered dark purple panel with white border + a single message line.
    int pw = 360, ph = 80;
    int px = 320 - pw / 2;
    int py = 240 - ph / 2;
    video_drawRectangle(px, py, pw, ph, 37, 16, 58);
    video_drawRectangle(px,          py,           pw, 2,  255, 255, 255);
    video_drawRectangle(px,          py + ph - 2,  pw, 2,  255, 255, 255);
    video_drawRectangle(px,          py,           2,  ph, 255, 255, 255);
    video_drawRectangle(px + pw - 2, py,           2,  ph, 255, 255, 255);

    video_screenWriteFont("Limite quotidienne atteinte", fontBold24, colorOrange,
                          320, py + (ph - 28) / 2, SDL_ALIGN_CENTER);
}

#endif // STORYTELLER_APP_LIMIT_OVERLAY__
