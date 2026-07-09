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

static const int   COLS   = 5;
static const float CW = 100, CH = 100, GAP = 16;
static const float STEP = CH + GAP;

static const float VIEW_TOP    = 156;
static const float VIEW_BOTTOM = SCREEN_HEIGHT - 110.0f;
static const float VIEW_H      = VIEW_BOTTOM - VIEW_TOP;

static int   rowCount()   { return (LEVEL_COUNT + COLS - 1) / COLS; }
static float contentH()   { return rowCount() * STEP; }
static float maxScroll()  { float m = contentH() - VIEW_H; return m > 0 ? m : 0; }

static float gridStartX() {
    float gridW = COLS * CW + (COLS - 1) * GAP;
    return (SCREEN_WIDTH - gridW) / 2.0f;
}

static Rectangle cellRect(int i, float scroll) {
    int col = i % COLS, row = i / COLS;
    return { gridStartX() + col * STEP, VIEW_TOP + row * STEP - scroll, CW, CH };
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

    scroll -= GetMouseWheelMove() * 40.0f;
    if (IsKeyDown(KEY_DOWN)) scroll += 300.0f * dt;
    if (IsKeyDown(KEY_UP))   scroll -= 300.0f * dt;
    if (scroll < 0) scroll = 0;
    if (scroll > maxScroll()) scroll = maxScroll();

    for (int i = 0; i < LEVEL_COUNT; i++) if (wob[i] > 0) wob[i] -= dt;
    {
        int hi = -1;
        Vector2 m = GetMousePosition();
        if (m.y >= VIEW_TOP && m.y <= VIEW_BOTTOM) {
            for (int i = 0; i < LEVEL_COUNT; i++) {
                if (i <= unlocked && CheckCollisionPointRec(m, cellRect(i, scroll))) { hi = i; break; }
            }
        }
        if (hi != hoverPrev) { if (hi >= 0) wob[hi] = LS_WOB_DUR; hoverPrev = hi; }
    }

    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        Vector2 m = GetMousePosition();
        if (m.y >= VIEW_TOP && m.y <= VIEW_BOTTOM) {
            for (int i = 0; i < LEVEL_COUNT; i++) {
                if (i <= unlocked && CheckCollisionPointRec(m, cellRect(i, scroll))) {
                    gStartLevel = i;
                    return ScreenType::GAME;
                }
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
    titleDraw(prog, SCREEN_WIDTH/2 - titleMeasure(prog, 22).x/2, 116, 22, INK);

    Vector2 mouse = GetMousePosition();

    BeginScissorMode(0, (int)VIEW_TOP, SCREEN_WIDTH, (int)VIEW_H);
    for (int i = 0; i < LEVEL_COUNT; i++) {
        Rectangle r = cellRect(i, scroll);
        if (r.y + r.height < VIEW_TOP || r.y > VIEW_BOTTOM) continue;

        bool done     = levelDone(progress, i);
        bool playable = (i <= unlocked);
        bool hover    = playable && CheckCollisionPointRec(mouse, r) &&
                        mouse.y >= VIEW_TOP && mouse.y <= VIEW_BOTTOM;

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
    EndScissorMode();

    if (maxScroll() > 0) {
        float trackX = SCREEN_WIDTH - 14.0f;
        float trackH = VIEW_H;
        DrawRectangle((int)trackX, (int)VIEW_TOP, 6, (int)trackH, (Color){ 220, 218, 210, 255 });
        float thumbH = trackH * (VIEW_H / contentH());
        float thumbY = VIEW_TOP + (scroll / maxScroll()) * (trackH - thumbH);
        DrawRectangle((int)trackX, (int)thumbY, 6, (int)thumbH, (Color){ 150, 148, 140, 255 });
    }

    menuDraw(menu);

    editorDrawOverlay(editor, layout);
}
