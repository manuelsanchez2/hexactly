#include "levelselect_screen.h"
#include "config.h"
#include "levels.h"
#include "gamefont.h"
#include "tween.h"
#include "raylib.h"
#include <cmath>

static const float LS_WOB_DUR  = 0.25f;
static const float LS_WOB_DIST = 5.0f;
static const float LS_WOB_ROT  = 4.0f;

static const int   COLS = 5;
static const float CW = 100, CH = 100, GAP = 16;
static const float STEP = CH + GAP;
static const float GRID_TOP = 170;

static const int TAB_COUNT = 2;
static const char* TAB_NAMES[TAB_COUNT] = { "Beginner", "Advanced" };

static int tabFirst(int tab) { return tab == 0 ? 0 : BEGINNER_COUNT; }
static int tabCount(int tab) {
    return tab == 0 ? BEGINNER_COUNT : LEVEL_COUNT - BEGINNER_COUNT;
}
static bool tabOpen(int tab, int unlocked) {
    return tab == 0 || unlocked >= BEGINNER_COUNT;
}

static Rectangle tabRect(int t) {
    float w = 180, h = 40, gap = 24;
    float x0 = (SCREEN_WIDTH - (TAB_COUNT * w + (TAB_COUNT - 1) * gap)) / 2.0f;
    return { x0 + t * (w + gap), 108, w, h };
}

static float gridStartX() {
    float gridW = COLS * CW + (COLS - 1) * GAP;
    return (SCREEN_WIDTH - gridW) / 2.0f;
}

static Rectangle cellRect(int i) {
    int col = i % COLS, row = i / COLS;
    return { gridStartX() + col * STEP, GRID_TOP + row * STEP, CW, CH };
}

static void drawCheck(Rectangle r) {
    Texture2D t = tickTexture();
    float sz = 34;
    Rectangle dst = { r.x + r.width - sz - 8, r.y + r.height - sz - 8, sz, sz };
    DrawTexturePro(t, { 0, 0, (float)t.width, (float)t.height }, dst, { 0, 0 }, 0.0f, WHITE);
}

static void drawLock(Rectangle r) {
    float cx = r.x + r.width/2;
    float bodyW = r.width*0.34f, bodyH = r.height*0.26f;
    Rectangle body = { cx - bodyW/2, r.y + r.height*0.5f, bodyW, bodyH };
    DrawRectangleRec(body, (Color){ 120, 120, 128, 255 });
    DrawRing({ cx, body.y }, bodyW*0.30f, bodyW*0.42f, 180, 360, 24, (Color){ 120, 120, 128, 255 });
}

LevelSelectScreen::LevelSelectScreen() {
    progress = loadProgress();
    unlocked = firstIncomplete(progress, LEVEL_COUNT);
    tab      = (unlocked >= BEGINNER_COUNT) ? 1 : 0;
    layout = loadLayout("levelselect");
    menuFromLayout(menu, ids, layout);
    mtime = layoutFileTime("levelselect");
}

ScreenType LevelSelectScreen::update() {
    float dt = GetFrameTime();

    if (editorTick(editor, layout, menu, ids, mtime)) return ScreenType::NONE;

    int a = menuUpdate(menu, dt);
    if (a >= 0 && ids[a] == "back") return ScreenType::TITLE;

    for (int i = 0; i < LEVEL_COUNT; i++) if (wob[i] > 0) wob[i] -= dt;
    {
        int hi = -1;
        Vector2 m = GetMousePosition();
        for (int i = 0; i < tabCount(tab); i++) {
            int lvl = tabFirst(tab) + i;
            if (lvl <= unlocked && CheckCollisionPointRec(m, cellRect(i))) { hi = lvl; break; }
        }
        if (hi != hoverPrev) { if (hi >= 0) wob[hi] = LS_WOB_DUR; hoverPrev = hi; }
    }

    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        Vector2 m = GetMousePosition();
        for (int t = 0; t < TAB_COUNT; t++) {
            if (t != tab && tabOpen(t, unlocked) &&
                CheckCollisionPointRec(m, tabRect(t))) {
                tab = t;
                return ScreenType::NONE;
            }
        }
        for (int i = 0; i < tabCount(tab); i++) {
            int lvl = tabFirst(tab) + i;
            if (lvl <= unlocked && CheckCollisionPointRec(m, cellRect(i))) {
                gStartLevel = lvl;
                return ScreenType::GAME;
            }
        }
    }
    return ScreenType::NONE;
}

void LevelSelectScreen::draw() {
    ClearBackground(PAPER);

    layoutDrawLabels(layout);

    Vector2 mouse = GetMousePosition();

    for (int t = 0; t < TAB_COUNT; t++) {
        Rectangle r = tabRect(t);
        bool open   = tabOpen(t, unlocked);
        bool active = (t == tab);
        bool hover  = open && CheckCollisionPointRec(mouse, r);

        int done = 0;
        for (int i = 0; i < tabCount(t); i++)
            if (levelDone(progress, tabFirst(t) + i)) done++;
        const char* txt = open ? TextFormat("%s  %d/%d", TAB_NAMES[t], done, tabCount(t))
                               : TAB_NAMES[t];

        Color col = active ? INK : (hover ? (Color){ 100, 100, 100, 255 }
                                          : (Color){ 165, 165, 165, 255 });
        float sz = 24;
        float w  = titleMeasure(txt, sz).x;
        float lockW = open ? 0.0f : 26.0f;
        float tx = r.x + r.width / 2 - (w + lockW) / 2;
        float ty = r.y + r.height / 2 - sz / 2;
        titleDraw(txt, tx, ty, sz, col);
        if (!open) {
            float cx = tx + w + lockW - 8;
            float bw = 14, bh = 10;
            DrawRectangleRec({ cx - bw/2, ty + sz/2, bw, bh }, col);
            DrawRing({ cx, ty + sz/2 }, 3.5f, 6.0f, 180, 360, 16, col);
        }
        if (active) {
            DrawLineEx({ tx - 4, ty + sz + 6 }, { tx + w + 4, ty + sz + 6 }, 3.0f, HEXRED);
        }
    }

    for (int i = 0; i < tabCount(tab); i++) {
        int lvl = tabFirst(tab) + i;
        Rectangle r = cellRect(i);

        bool done     = levelDone(progress, lvl);
        bool playable = (lvl <= unlocked);
        bool hover    = playable && CheckCollisionPointRec(mouse, r);

        Texture2D sq = levelSquareTexture();
        Rectangle src = { 0, 0, (float)sq.width, (float)sq.height };

        if (!playable) {
            DrawTexturePro(sq, src, r, { 0, 0 }, 0.0f, (Color){ 70, 70, 76, 255 });
            drawLock(r);
            continue;
        }

        float wv  = (wob[lvl] > 0) ? wobbleAt(1.0f - wob[lvl] / LS_WOB_DUR) : 0.0f;
        float tx  = LS_WOB_DIST * wv;
        float rot = LS_WOB_ROT * wv;
        float cx  = r.x + r.width / 2 + tx;
        float cy  = r.y + r.height / 2;

        Color tint = hover ? WHITE : (Color){ 235, 235, 235, 255 };
        DrawTexturePro(sq, src, { cx, cy, r.width, r.height },
                       { r.width / 2, r.height / 2 }, rot, tint);
        titleDrawCenteredAtRot(TextFormat("%d", lvl + 1), cx, cy, 40, rot, INK);
        if (done) { Rectangle tr = { r.x + tx, r.y, r.width, r.height }; drawCheck(tr); }
    }

    menuDraw(menu);

    editorDrawOverlay(editor, layout);
}
