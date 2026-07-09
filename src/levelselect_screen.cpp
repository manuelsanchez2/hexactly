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

// When the daily feature flag is off, drop the daily button so it is never
// drawn or clickable. Called after every menu (re)build, since the layout
// editor rebuilds the menu on live-reload.
static void applyFeatureFlags(Menu& menu, std::vector<std::string>& ids) {
#if !FEATURE_DAILY
    for (int i = 0; i < (int)ids.size(); i++) {
        if (ids[i] == "daily") {
            menu.buttons.erase(menu.buttons.begin() + i);
            ids.erase(ids.begin() + i);
            break;
        }
    }
#else
    (void)menu; (void)ids;
#endif
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

#if FEATURE_DAILY
    // Label the daily button with today's challenge number.
    for (int i = 0; i < (int)ids.size(); i++)
        if (ids[i] == "daily")
            menu.buttons[i].text = TextFormat("Daily #%d", dailyIndex() + 1);
#endif
    applyFeatureFlags(menu, ids);
}

ScreenType LevelSelectScreen::update() {
    float dt = GetFrameTime();

    if (editorTick(editor, layout, menu, ids, mtime)) return ScreenType::NONE;
    applyFeatureFlags(menu, ids);   // keep daily hidden across layout live-reloads

    int a = menuUpdate(menu, dt);
    if (a >= 0 && ids[a] == "back")  return ScreenType::TITLE;
#if FEATURE_DAILY
    if (a >= 0 && ids[a] == "daily") {
        // Already solved today's daily? The button is a no-op (shows a tick).
        if (progress.dailyLastDay != dailyEpochDay()) {
            gDailyMode = true;
            return ScreenType::GAME;
        }
    }
#endif

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
                gDailyMode  = false;
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

#if FEATURE_DAILY
    // Daily streak, shown only while it's still alive (won today or yesterday).
    long today = dailyEpochDay();
    if (progress.dailyStreak > 0 &&
        (progress.dailyLastDay == today || progress.dailyLastDay == today - 1)) {
        const char* streak = TextFormat("Daily streak: %d", progress.dailyStreak);
        titleDraw(streak, SCREEN_WIDTH/2 - titleMeasure(streak, 18).x/2, 610, 18, HEXRED);
    }
#endif

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

    // Icon just before the daily button's title: plain badge normally,
    // ticked badge once today's challenge is solved.
#if FEATURE_DAILY
    {
        bool doneToday = (progress.dailyLastDay == dailyEpochDay());
        for (int i = 0; i < (int)ids.size(); i++) {
            if (ids[i] != "daily") continue;
            Rectangle rb = menu.buttons[i].bounds;
            float ts     = titleMeasure(menu.buttons[i].text.c_str(), 24).x;
            float cx     = rb.x + rb.width / 2.0f;
            float cy     = rb.y + rb.height / 2.0f;
            float sz     = 30.0f;
            float ix     = cx - ts / 2.0f - sz - 8.0f;   // immediately left of the text
            Texture2D ic = doneToday ? btnDailyTickTexture() : btnDailyTexture();
            DrawTexturePro(ic, { 0, 0, (float)ic.width, (float)ic.height },
                           { ix, cy - sz / 2.0f, sz, sz }, { 0, 0 }, 0.0f, WHITE);
            break;
        }
    }
#endif

    editorDrawOverlay(editor, layout);
}
