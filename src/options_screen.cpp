#include "options_screen.h"
#include "config.h"
#include "audio.h"
#include "gamefont.h"
#include "progress.h"
#include "tween.h"
#include "raylib.h"
#include <cmath>

static const float MODAL_SLIDE = 340.0f;
static const float CLEARED_DUR = 2.2f;

static Rectangle confirmPanel() { return { SCREEN_WIDTH/2.0f - 170, SCREEN_HEIGHT/2.0f - 120, 340, 290 }; }

OptionsScreen::OptionsScreen() {
    layout = loadLayout("options");
    menuFromLayout(menu, ids, layout);
    mtime = layoutFileTime("options");

    float cx = SCREEN_WIDTH / 2.0f - 120;
    float cy = SCREEN_HEIGHT / 2.0f;
    menuAdd(confirmMenu, { cx, cy - 4, 240, 52 }, "Clear");
    menuAdd(confirmMenu, { cx, cy + 60, 240, 52 }, "Cancel");
}

void OptionsScreen::openConfirm() {
    confirming = true;
    confirmAnim = 0.0f;
    confirmMenu.focus = 0;
    confirmMenu.kbFocus = false;
    confirmMenu.litPrev = -1;
    confirmMenu.offset = { 0, 0 };
    for (Button& b : confirmMenu.buttons) { b.anim = 0.0f; b.wob = 0.0f; }
}

ScreenType OptionsScreen::update() {
    if (editorTick(editor, layout, menu, ids, mtime)) return ScreenType::NONE;

    float dt = GetFrameTime();
    if (clearedTimer > 0) clearedTimer -= dt;

    if (confirming) {
        confirmAnim += (1.0f - confirmAnim) * (1.0f - expf(-10.0f * dt));
        confirmMenu.offset.y = (1.0f - easeOutBack(confirmAnim)) * MODAL_SLIDE;
        int a = menuUpdate(confirmMenu, dt);
        if (a == 0) {
            Progress p;
            saveProgress(p);
            confirming = false;
            clearedTimer = CLEARED_DUR;
        }
        if (a == 1 || IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE)) confirming = false;
        return ScreenType::NONE;
    }

    int a = menuUpdate(menu, dt);
    if (a >= 0) {
        const std::string& id = ids[a];
        if (id == "mus_minus") audioSetMusicVolume(audioGetMusicVolume() - 0.1f);
        if (id == "mus_plus")  audioSetMusicVolume(audioGetMusicVolume() + 0.1f);
        if (id == "sfx_minus" || id == "sfx_plus") {
            audioSetSfxVolume(audioGetSfxVolume() + (id == "sfx_plus" ? 0.1f : -0.1f));
            playMerge();
        }
        if (id == "clear") { openConfirm(); return ScreenType::NONE; }
        if (id == "back") return ScreenType::TITLE;
        audioSaveSettings();
    }
    return ScreenType::NONE;
}

void OptionsScreen::draw() {
    ClearBackground(PAPER);

    layoutDrawLabels(layout);

    const char* mus = TextFormat("Music: %d", (int)(audioGetMusicVolume() * 100 + 0.5f));
    titleDraw(mus, SCREEN_WIDTH/2 - titleMeasure(mus, 24).x/2, 264, 24, INK);

    const char* sfx = TextFormat("SFX: %d", (int)(audioGetSfxVolume() * 100 + 0.5f));
    titleDraw(sfx, SCREEN_WIDTH/2 - titleMeasure(sfx, 24).x/2, 364, 24, INK);

    menuDraw(menu);

    if (clearedTimer > 0) {
        float alpha = clearedTimer < 0.5f ? clearedTimer / 0.5f : 1.0f;
        Font f = markerFont();
        const char* msg = "Data cleared!";
        float fs = 46.0f, sp = fs * 0.04f;
        Vector2 m = MeasureTextEx(f, msg, fs, sp);
        DrawTextEx(f, msg, { SCREEN_WIDTH/2.0f - m.x/2, 190 }, fs, sp, Fade(HEXRED, alpha));
    }

    if (confirming) {
        float alpha = confirmAnim;
        float dy    = confirmMenu.offset.y;

        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.55f * alpha));

        Rectangle panel = confirmPanel();
        panel.y += dy;
        DrawRectangleRec(panel, Fade(PAPER, alpha));
        DrawRectangleLinesEx(panel, 2, Fade(INK, alpha));

        titleDrawCentered("Clear all data?", panel.y + 26, 34, Fade(INK, alpha));
        titleDrawCentered("This cannot be undone.", panel.y + 74, 20, Fade(INK, alpha));

        menuDraw(confirmMenu);
    }

    editorDrawOverlay(editor, layout);
}
