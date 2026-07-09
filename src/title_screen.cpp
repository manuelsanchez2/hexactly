#include "title_screen.h"
#include "config.h"
#include "gamefont.h"
#include "tween.h"
#include "ui.h"
#include "layout.h"
#include "editor.h"
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

static void drawLabel(const Layout& layout, const char* id, float dy, float alpha) {
    const LayoutElement* e = layoutFind(layout, id);
    if (!e) return;
    float w = titleMeasure(e->text.c_str(), e->size).x;
    float x = (e->align == Align::Center) ? e->x - w / 2.0f : e->x;
    titleDraw(e->text.c_str(), x, e->y + dy, e->size, Fade(layoutColor(e->color), alpha));
}

TitleScreen::TitleScreen() {
    layout = loadLayout("title");
    menuFromLayout(menu, ids, layout);
    mtime = layoutFileTime("title");
}

ScreenType TitleScreen::update() {
    float dt = GetFrameTime();
    introT += dt;

    if (editorTick(editor, layout, menu, ids, mtime)) return ScreenType::NONE;

    if (introT < introTotal((int)menu.buttons.size())) return ScreenType::NONE;

    int a = menuUpdate(menu, dt);
    if (a >= 0) {
        const std::string& id = ids[a];
        if (id == "play")    return ScreenType::LEVELSELECT;
        if (id == "options") return ScreenType::OPTIONS;
        if (id == "quit")    return ScreenType::QUIT;
    }
    return ScreenType::NONE;
}

void TitleScreen::draw() {
    ClearBackground(PAPER);

    Texture2D bg = bgTitleTexture();
    DrawTexturePro(bg, { 0, 0, (float)bg.width, (float)bg.height },
                   { 0, 0, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT },
                   { 0, 0 }, 0.0f, Fade(WHITE, 0.95f));

    float logoOff = -(1.0f - easeOutBack(clamp01(introT / LOGO_DUR))) * LOGO_FROM;
    drawLabel(layout, "title", logoOff, 1.0f);
    drawLabel(layout, "subtitle", logoOff, 1.0f);

    float verE = easeOutQuad(clamp01((introT - introTotal((int)menu.buttons.size())) / VER_DUR));
    drawLabel(layout, "version", (1.0f - verE) * VER_FROM, verE);

    for (int i = 0; i < (int)menu.buttons.size(); i++) {
        float t = introT - BTN_START - i * BTN_STAGGER;
        menu.buttons[i].slideY = (1.0f - easeOutBack(clamp01(t / BTN_DUR))) * BTN_FROM;
    }
    menuDraw(menu);

    editorDrawOverlay(editor, layout);
}
