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

// 25 Beginner cells must fit in 5 rows above the back button (y=636), so the
// cells are a touch smaller than the original 100px.
static const int   COLS = 5;
static const float CW = 86, CH = 86, GAP = 10;
static const float STEP = CH + GAP;
static const float GRID_TOP = 158;

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
    float sz = 30;
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
    tab      = (unlocked >= BEGINNER_COUNT) ? 1 : 0;
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
        titleDrawCenteredAtRot(TextFormat("%d", lvl + 1), cx, cy, 34, rot, INK);
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
