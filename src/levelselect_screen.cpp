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
static const float GRID_TOP = 158;

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
        for (int i = 0; i < LEVEL_COUNT; i++) {
            if (i <= unlocked && CheckCollisionPointRec(m, cellRect(i))) { hi = i; break; }
        }
        if (hi != hoverPrev) { if (hi >= 0) wob[hi] = LS_WOB_DUR; hoverPrev = hi; }
    }

    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        Vector2 m = GetMousePosition();
        for (int i = 0; i < LEVEL_COUNT; i++) {
            if (i <= unlocked && CheckCollisionPointRec(m, cellRect(i))) {
                gStartLevel = i;
                return ScreenType::GAME;
            }
        }
    }
    return ScreenType::NONE;
}

void LevelSelectScreen::draw() {
    ClearBackground(PAPER);

    layoutDrawLabels(layout);

    int doneCount = 0;
    for (int i = 0; i < LEVEL_COUNT; i++) if (levelDone(progress, i)) doneCount++;
    const char* prog = TextFormat("Completed  %d / %d", doneCount, LEVEL_COUNT);
    titleDraw(prog, SCREEN_WIDTH/2 - titleMeasure(prog, 22).x/2, 120, 22, INK);

    Vector2 mouse = GetMousePosition();

    for (int i = 0; i < LEVEL_COUNT; i++) {
        Rectangle r = cellRect(i);

        bool done     = levelDone(progress, i);
        bool playable = (i <= unlocked);
        bool hover    = playable && CheckCollisionPointRec(mouse, r);

        Texture2D sq = levelSquareTexture();
        Rectangle src = { 0, 0, (float)sq.width, (float)sq.height };

        if (!playable) {
            DrawTexturePro(sq, src, r, { 0, 0 }, 0.0f, (Color){ 70, 70, 76, 255 });
            drawLock(r);
            continue;
        }

        float wv  = (wob[i] > 0) ? wobbleAt(1.0f - wob[i] / LS_WOB_DUR) : 0.0f;
        float tx  = LS_WOB_DIST * wv;
        float rot = LS_WOB_ROT * wv;
        float cx  = r.x + r.width / 2 + tx;
        float cy  = r.y + r.height / 2;

        Color tint = hover ? WHITE : (Color){ 235, 235, 235, 255 };
        DrawTexturePro(sq, src, { cx, cy, r.width, r.height },
                       { r.width / 2, r.height / 2 }, rot, tint);
        titleDrawCenteredAtRot(TextFormat("%d", i + 1), cx, cy, 40, rot, INK);
        if (done) { Rectangle tr = { r.x + tx, r.y, r.width, r.height }; drawCheck(tr); }
    }

    menuDraw(menu);

    editorDrawOverlay(editor, layout);
}
