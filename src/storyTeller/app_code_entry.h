#ifndef STORYTELLER_APP_CODE_ENTRY__
#define STORYTELLER_APP_CODE_ENTRY__

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "./app_parameters.h"

// NOTE: relies on video_drawRectangle, video_screenWriteFont, font*, color*
// from sdl_helper.h — include this header AFTER those are defined.

typedef enum {
    CODE_MODE_VERIFY = 0,   // user must enter stored code
    CODE_MODE_SET    = 1    // user picks a new code
} CodeEntryMode;

// Result reported via callback. On CANCEL, no code change happened.
typedef enum {
    CODE_RESULT_VERIFIED = 0,
    CODE_RESULT_SET      = 1,
    CODE_RESULT_CANCELED = 2,
    CODE_RESULT_WRONG    = 3   // internal: signals wrong code, overlay stays open
} CodeEntryResult;

typedef void (*CodeEntryCallback)(CodeEntryResult result);

static bool             codeentry_open_flag = false;
static CodeEntryMode    codeentry_mode      = CODE_MODE_VERIFY;
static int              codeentry_digits[4] = {0, 0, 0, 0};
static int              codeentry_cursor    = 0;
static bool             codeentry_wrong     = false;
static CodeEntryCallback codeentry_cb       = NULL;

void codeentry_open(CodeEntryMode mode, CodeEntryCallback cb) {
    codeentry_open_flag = true;
    codeentry_mode = mode;
    codeentry_cursor = 0;
    codeentry_wrong = false;
    codeentry_cb = cb;
    for (int i = 0; i < 4; ++i) codeentry_digits[i] = 0;
}

void codeentry_close(void) {
    codeentry_open_flag = false;
    codeentry_cb = NULL;
}

bool codeentry_isOpen(void) {
    return codeentry_open_flag;
}

static void codeentry_buildString(char out[5]) {
    for (int i = 0; i < 4; ++i) {
        out[i] = '0' + codeentry_digits[i];
    }
    out[4] = '\0';
}

void codeentry_handleUp(void) {
    codeentry_wrong = false;
    codeentry_digits[codeentry_cursor] = (codeentry_digits[codeentry_cursor] + 1) % 10;
}

void codeentry_handleDown(void) {
    codeentry_wrong = false;
    codeentry_digits[codeentry_cursor] = (codeentry_digits[codeentry_cursor] + 9) % 10;
}

void codeentry_handleLeft(void) {
    codeentry_wrong = false;
    if (codeentry_cursor > 0) codeentry_cursor--;
}

void codeentry_handleRight(void) {
    codeentry_wrong = false;
    if (codeentry_cursor < 3) codeentry_cursor++;
}

void codeentry_handleA(void) {
    char entered[5];
    codeentry_buildString(entered);

    if (codeentry_mode == CODE_MODE_VERIFY) {
        if (strncmp(entered, parameters_getParentalLockCode(), 4) == 0) {
            CodeEntryCallback cb = codeentry_cb;
            codeentry_close();
            if (cb) cb(CODE_RESULT_VERIFIED);
        } else {
            codeentry_wrong = true;
            for (int i = 0; i < 4; ++i) codeentry_digits[i] = 0;
            codeentry_cursor = 0;
        }
    } else {
        parameters_setParentalLockCode(entered);
        parameters_save();
        CodeEntryCallback cb = codeentry_cb;
        codeentry_close();
        if (cb) cb(CODE_RESULT_SET);
    }
}

void codeentry_handleB(void) {
    CodeEntryCallback cb = codeentry_cb;
    codeentry_close();
    if (cb) cb(CODE_RESULT_CANCELED);
}

void codeentry_draw(void) {
    // Panel (purple, white border): 360x200 centered
    int px = 140, py = 140, pw = 360, ph = 200;
    video_drawRectangle(px, py, pw, ph, 37, 16, 58);
    video_drawRectangle(px,         py,          pw, 2,  255, 255, 255);
    video_drawRectangle(px,         py + ph - 2, pw, 2,  255, 255, 255);
    video_drawRectangle(px,         py,          2,  ph, 255, 255, 255);
    video_drawRectangle(px + pw - 2, py,         2,  ph, 255, 255, 255);

    const char *title = (codeentry_mode == CODE_MODE_SET) ? "Nouveau code" : "Entrer le code";
    video_screenWriteFont(title, fontBold24, colorWhite, 320, py + 18, SDL_ALIGN_CENTER);

    // 4 digit boxes: 50x60 each, gap 12, total 4*50 + 3*12 = 236, centered at x=320
    int boxW = 50, boxH = 60, gap = 12;
    int total = boxW * 4 + gap * 3;
    int x0 = 320 - total / 2;
    int yBox = py + 64;

    for (int i = 0; i < 4; ++i) {
        int bx = x0 + i * (boxW + gap);
        // background of box (slightly lighter purple)
        video_drawRectangle(bx, yBox, boxW, boxH, 60, 30, 90);
        SDL_Color borderColor = (i == codeentry_cursor) ? colorOrange : colorWhite60;
        video_drawRectangle(bx,            yBox,            boxW, 2,    borderColor.r, borderColor.g, borderColor.b);
        video_drawRectangle(bx,            yBox + boxH - 2, boxW, 2,    borderColor.r, borderColor.g, borderColor.b);
        video_drawRectangle(bx,            yBox,            2,    boxH, borderColor.r, borderColor.g, borderColor.b);
        video_drawRectangle(bx + boxW - 2, yBox,            2,    boxH, borderColor.r, borderColor.g, borderColor.b);

        char digit[2] = {'0' + codeentry_digits[i], '\0'};
        SDL_Color digitColor = (i == codeentry_cursor) ? colorOrange : colorWhite;
        video_screenWriteFont(digit, fontBold24, digitColor, bx + boxW / 2, yBox + 14, SDL_ALIGN_CENTER);
    }

    if (codeentry_wrong) {
        video_screenWriteFont("Code incorrect", fontRegular16, colorRed, 320, py + ph - 50, SDL_ALIGN_CENTER);
    } else {
        const char *hint = (codeentry_mode == CODE_MODE_SET)
            ? "HAUT/BAS chiffre   G/D position   A valider   B annuler"
            : "HAUT/BAS chiffre   G/D position   A valider   B annuler";
        video_screenWriteFont(hint, fontRegular16, colorWhite60, 320, py + ph - 30, SDL_ALIGN_CENTER);
    }
}

#endif // STORYTELLER_APP_CODE_ENTRY__
