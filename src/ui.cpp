#include "ui.h"
#include "tween.h"
#include "gamefont.h"
#include "config.h"
#include <cmath>

bool buttonClicked(Rectangle bounds) {
    return IsMouseButtonReleased(MOUSE_LEFT_BUTTON) &&
           CheckCollisionPointRec(GetMousePosition(), bounds);
}

void drawButton(Rectangle bounds, const char *text) {
    bool hovered = CheckCollisionPointRec(GetMousePosition(), bounds);

    if (hovered) DrawRectangleRec(bounds, INK);
    else         DrawRectangleRec(bounds, PAPER);
    DrawRectangleLinesEx(bounds, 2, INK);

    int fontSize = 24;
    int textW = MeasureText(text, fontSize);
    DrawText(text,
             (int)(bounds.x + bounds.width  / 2 - textW / 2),
             (int)(bounds.y + bounds.height / 2 - fontSize / 2),
             fontSize, hovered ? PAPER : INK);
}

static const float BTN_WOB_DUR  = 0.25f;
static const float BTN_WOB_DIST = 2.0f;
static const float BTN_WOB_ROT  = 2.0f;

static Color lerpColor(Color a, Color b, float t) {
    return (Color){
        (unsigned char)lerpf(a.r, b.r, t),
        (unsigned char)lerpf(a.g, b.g, t),
        (unsigned char)lerpf(a.b, b.b, t),
        (unsigned char)lerpf(a.a, b.a, t),
    };
}

static Rectangle placed(const Button &b, Vector2 offset) {
    return { b.bounds.x + offset.x, b.bounds.y + offset.y + b.slideY,
             b.bounds.width, b.bounds.height };
}

void menuAdd(Menu &menu, Rectangle bounds, const std::string &text) {
    Button b;
    b.bounds = bounds;
    b.text   = text;
    menu.buttons.push_back(b);
}

int menuUpdate(Menu &menu, float dt) {
    int n = (int)menu.buttons.size();
    if (n == 0) return -1;

    bool navKey = IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S) ||
                  IsKeyPressed(KEY_UP)   || IsKeyPressed(KEY_W) || IsKeyPressed(KEY_TAB);
    if (navKey) menu.kbFocus = true;
    Vector2 md = GetMouseDelta();
    if (md.x != 0.0f || md.y != 0.0f) menu.kbFocus = false;

    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) menu.focus = (menu.focus + 1) % n;
    if (IsKeyPressed(KEY_UP)   || IsKeyPressed(KEY_W)) menu.focus = (menu.focus - 1 + n) % n;
    if (IsKeyPressed(KEY_TAB)) {
        bool shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
        menu.focus = shift ? (menu.focus - 1 + n) % n : (menu.focus + 1) % n;
    }

    Vector2 mouse = GetMousePosition();
    int hoverIdx = -1;
    for (int i = 0; i < n; i++) {
        if (CheckCollisionPointRec(mouse, placed(menu.buttons[i], menu.offset))) hoverIdx = i;
    }
    int lit = menu.kbFocus ? menu.focus : hoverIdx;
    if (hoverIdx >= 0) menu.focus = hoverIdx;

    if (lit != menu.litPrev) {
        if (lit >= 0) menu.buttons[lit].wob = BTN_WOB_DUR;
        menu.litPrev = lit;
    }

    for (int i = 0; i < n; i++) {
        float target = (i == lit) ? 1.0f : 0.0f;
        float k = 1.0f - expf(-14.0f * dt);
        menu.buttons[i].anim += (target - menu.buttons[i].anim) * k;
        if (menu.buttons[i].wob > 0) menu.buttons[i].wob -= dt;
    }

    int activated = -1;
    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        for (int i = 0; i < n; i++) {
            if (CheckCollisionPointRec(mouse, placed(menu.buttons[i], menu.offset))) activated = i;
        }
    }
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) activated = menu.focus;
    return activated;
}

static void drawJuicyButton(const Button &b, Vector2 offset) {
    float wv    = (b.wob > 0) ? wobbleAt(1.0f - b.wob / BTN_WOB_DUR) : 0.0f;
    float tx    = BTN_WOB_DIST * wv;
    float angle = BTN_WOB_ROT * wv;

    Rectangle r = placed(b, offset);
    float cx = r.x + r.width  / 2.0f + tx;
    float cy = r.y + r.height / 2.0f;

    bool square = (r.width == r.height);
    Texture2D tex = square ? squareButtonTexture() : primaryButtonTexture();
    Color tint = lerpColor(WHITE, (Color){ 205, 205, 205, 255 }, b.anim);

    DrawTexturePro(tex, { 0, 0, (float)tex.width, (float)tex.height },
                   { cx, cy, r.width, r.height }, { r.width / 2, r.height / 2 }, angle, tint);

    float fontSize = 24.0f;
    float spacing  = fontSize * 0.04f;
    Font f = titleFont();
    Vector2 ts = MeasureTextEx(f, b.text.c_str(), fontSize, spacing);
    DrawTextPro(f, b.text.c_str(), { cx, cy }, { ts.x / 2, ts.y / 2 },
                angle, fontSize, spacing, INK);
}

void menuDraw(const Menu &menu) {
    for (const Button &b : menu.buttons) drawJuicyButton(b, menu.offset);
}
