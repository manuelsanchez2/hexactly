#include "title_screen.h"
#include "config.h"
#include "gamefont.h"
#include "tween.h"
#include "ui.h"
#include "raylib.h"

static const float LOGO_DUR    = 0.55f;
static const float LOGO_FROM   = 460.0f;
static const float BTN_START   = 0.28f;
static const float BTN_STAGGER = 0.12f;
static const float BTN_DUR     = 0.50f;
static const float BTN_FROM    = 460.0f;
static const float VER_DUR     = 0.45f;
static const float VER_FROM    = 60.0f;

static float clamp01(float t) { return t < 0 ? 0 : (t > 1 ? 1 : t); }

static float introTotal(int buttonCount) {
    float last = BTN_START + (buttonCount > 0 ? (buttonCount - 1) : 0) * BTN_STAGGER + BTN_DUR;
    return last > LOGO_DUR ? last : LOGO_DUR;
}

TitleScreen::TitleScreen() {
    menuAdd(menu, { 240, 433, 240, 58 }, "Play");
    menuAdd(menu, { 240, 509, 240, 58 }, "Options");
    menuAdd(menu, { 240, 583, 240, 58 }, "Quit");
}

ScreenType TitleScreen::update() {
    float dt = GetFrameTime();
    introT += dt;

    if (introT < introTotal((int)menu.buttons.size())) return ScreenType::NONE;

    int a = menuUpdate(menu, dt);
    if (a == 0) return ScreenType::LEVELSELECT;
    if (a == 1) return ScreenType::OPTIONS;
    if (a == 2) return ScreenType::QUIT;
    return ScreenType::NONE;
}

void TitleScreen::draw() {
    ClearBackground(PAPER);

    Texture2D bg = bgTitleTexture();
    DrawTexturePro(bg, { 0, 0, (float)bg.width, (float)bg.height },
                   { 0, 0, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT },
                   { 0, 0 }, 0.0f, Fade(WHITE, 0.95f));

    float logoP   = clamp01(introT / LOGO_DUR);
    float logoOff = -(1.0f - easeOutBack(logoP)) * LOGO_FROM;

    titleDrawCentered("HEXACTLY", 210 + logoOff, 84, INK);
    titleDrawCentered("merge equal neighbours into one tile", 345 + logoOff, 22, INK);

    float verE   = easeOutQuad(clamp01((introT - introTotal((int)menu.buttons.size())) / VER_DUR));
    titleDraw("v.0.0.1", 622, 685 + (1.0f - verE) * VER_FROM, 16, Fade(GRAY, verE));

    for (int i = 0; i < (int)menu.buttons.size(); i++) {
        float t = introT - BTN_START - i * BTN_STAGGER;
        float p = clamp01(t / BTN_DUR);
        menu.buttons[i].slideY = (1.0f - easeOutBack(p)) * BTN_FROM;
    }
    static int dbg = 0;
    if (++dbg == 15) {
        for (int i = 0; i < (int)menu.buttons.size(); i++)
            TraceLog(LOG_WARNING, "BTN %d y=%.0f slideY=%.1f anim=%.2f introT=%.2f",
                     i, menu.buttons[i].bounds.y, menu.buttons[i].slideY, menu.buttons[i].anim, introT);
    }
    menuDraw(menu);
}
