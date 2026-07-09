#include "game_screen.h"
#include "screen.h"
#include "config.h"
#include "gamefont.h"
#include "levels.h"
#include "tween.h"
#include "ui.h"
#include "raylib.h"
#include <cmath>

static const float BOARD_DROP = 110.0f;
static const float TILE_SCALE = 96.0f / 46.0f;
static const float MODAL_SLIDE = 360.0f;
static const float PAUSE_WOB_DUR = 0.25f;

static void drawHexTile(Vector2 c, float scale, Color tint) {
    Texture2D t = hexTileTexture();
    float S = HEX_SIZE * TILE_SCALE * scale;
    DrawTexturePro(t, { 0, 0, (float)t.width, (float)t.height },
                   { c.x, c.y, S, S }, { S / 2, S / 2 }, 0.0f, tint);
}

static void drawFlag(Vector2 c) {
    Texture2D t = flagGoalTexture();
    float S = t.width / 3.0f;
    Vector2 pos = { c.x + HEX_SIZE * 0.40f, c.y + HEX_SIZE * 0.38f };
    DrawTexturePro(t, { 0, 0, (float)t.width, (float)t.height },
                   { pos.x, pos.y, S, S }, { S / 2, S / 2 }, 0.0f, WHITE);
}

static Rectangle pauseBtn()   { return { SCREEN_WIDTH - 130.0f, 20, 110, 44 }; }
static Rectangle modalPanel() { return { SCREEN_WIDTH / 2.0f - 160, SCREEN_HEIGHT / 2.0f - 150, 320, 300 }; }
static Rectangle mBtn(int i)  { return { SCREEN_WIDTH / 2.0f - 120, SCREEN_HEIGHT / 2.0f - 90 + i * 70.0f, 240, 54 }; }

static void dimAndPanel(const char *title, float alpha, float dy) {
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.5f * alpha));
    Rectangle p = modalPanel();
    p.y += dy;
    DrawRectangleRec(p, Fade(PAPER, alpha));
    DrawRectangleLinesEx(p, 2, Fade(INK, alpha));
    titleDrawCentered(title, p.y + 26, 40, Fade(INK, alpha));
}

GameScreen::GameScreen() {
    menuAdd(pauseMenu, mBtn(0), "Resume");
    menuAdd(pauseMenu, mBtn(1), "Restart");
    menuAdd(pauseMenu, mBtn(2), "Main Menu");

    menuAdd(winMenu, mBtn(0), "Next Level");
    menuAdd(winMenu, mBtn(1), "Levels");

    menuAdd(lostMenu, mBtn(0), "Restart");
    menuAdd(lostMenu, mBtn(1), "Main Menu");
}

static void resetMenu(Menu &m, float &anim) {
    anim = 0.0f;
    m.focus = 0;
    m.kbFocus = false;
    m.litPrev = -1;
    m.offset = { 0, 0 };
    for (Button &b : m.buttons) { b.anim = 0.0f; b.wob = 0.0f; }
}

void GameScreen::openPause() { resetMenu(pauseMenu, pauseAnim); }

void GameScreen::openWin() {
    bool last = (currentLevel + 1 >= LEVEL_COUNT);
    winMenu.buttons[0].text = last ? "Back to Levels" : "Next Level";
    resetMenu(winMenu, winAnim);
}

void GameScreen::openLost() { resetMenu(lostMenu, lostAnim); }

void GameScreen::loadLevel(int idx) {
    const LevelDef& L = LEVELS[idx];
    board.cellCount = L.cellCount;
    board.movesLeft = L.moveLimit;
    for (int i = 0; i < L.cellCount; i++) {
        board.cells[i].pos       = { L.cells[i].q, L.cells[i].r };
        board.cells[i].value     = L.cells[i].value;
        board.cells[i].exists    = true;
        board.cells[i].isGoal    = (L.cells[i].flags & F_GOAL)   != 0;
        board.cells[i].isStone   = (L.cells[i].flags & F_STONE)  != 0;
        board.cells[i].isCursed  = (L.cells[i].flags & F_CURSED) != 0;
        board.cells[i].goalValue = L.cells[i].goalValue;
    }
    board.wallCount = L.wallCount;
    for (int i = 0; i < L.wallCount; i++) {
        board.walls[i].a = { L.walls[i].q1, L.walls[i].r1 };
        board.walls[i].b = { L.walls[i].q2, L.walls[i].r2 };
    }
    board.portalCount = L.portalCount;
    for (int i = 0; i < L.portalCount; i++) {
        board.portals[i].a = { L.portals[i].q1, L.portals[i].r1 };
        board.portals[i].b = { L.portals[i].q2, L.portals[i].r2 };
    }

    int gi = goalIndex(board);
    if (gi >= 0 && board.cells[gi].goalValue == 0) {
        int sum = 0;
        for (int i = 0; i < board.cellCount; i++) sum += board.cells[i].value;
        board.cells[gi].goalValue = sum;
    }

    Vector2 s = { 0, 0 };
    float minX = 1e9f, maxX = -1e9f, minY = 1e9f, maxY = -1e9f;
    for (int i = 0; i < board.cellCount; i++) {
        Vector2 px = hexToPixel(board.cells[i].pos, (Vector2){ 0, 0 });
        s.x += px.x; s.y += px.y;
        if (px.x < minX) minX = px.x;  if (px.x > maxX) maxX = px.x;
        if (px.y < minY) minY = px.y;  if (px.y > maxY) maxY = px.y;
    }
    Vector2 avg = { s.x / board.cellCount, s.y / board.cellCount };
    float ox = SCREEN_WIDTH  / 2.0f - avg.x;
    float oy = SCREEN_HEIGHT / 2.0f + BOARD_DROP - avg.y;

    const float PAD = 54.0f;
    const float TOP = 140.0f, BOT = SCREEN_HEIGHT - 36.0f;
    const float LEFT = 24.0f, RIGHT = SCREEN_WIDTH - 24.0f;
    { float lo = LEFT + PAD - minX, hi = RIGHT - PAD - maxX; if (ox > hi) ox = hi; if (ox < lo) ox = lo; }
    { float lo = TOP  + PAD - minY, hi = BOT   - PAD - maxY; if (oy > hi) oy = hi; if (oy < lo) oy = lo; }
    origin = (Vector2){ ox, oy };

    selectedIdx = -1;
    pressIdx    = -1;
    dragging    = false;
    won  = false;
    lost = false;
}

void GameScreen::doMerge(int fromIdx, int toIdx) {
    board.cells[toIdx].value  *= 2;
    board.cells[fromIdx].value = 0;
    board.movesLeft--;
    selectedIdx = -1;

    if (isWon(board)) { won = true; openWin(); }
    else if (board.movesLeft <= 0 || !anyLegalMove(board)) { lost = true; openLost(); }
}

ScreenType GameScreen::update() {
    if (!started) {
        currentLevel = gStartLevel;
        if (currentLevel < 0) currentLevel = 0;
        if (currentLevel >= LEVEL_COUNT) currentLevel = LEVEL_COUNT - 1;
        loadLevel(currentLevel);
        started = true;
    }

    float dt = GetFrameTime();

    if (paused) {
        pauseAnim += (1.0f - pauseAnim) * (1.0f - expf(-10.0f * dt));
        pauseMenu.offset.y = (1.0f - easeOutBack(pauseAnim)) * MODAL_SLIDE;
        int a = menuUpdate(pauseMenu, dt);
        if (a == 0 || IsKeyPressed(KEY_ESCAPE)) paused = false;
        if (a == 1) { loadLevel(currentLevel); paused = false; }
        if (a == 2) return ScreenType::TITLE;
        return ScreenType::NONE;
    }

    if (won) {
        winAnim += (1.0f - winAnim) * (1.0f - expf(-10.0f * dt));
        winMenu.offset.y = (1.0f - easeOutBack(winAnim)) * MODAL_SLIDE;
        bool last = (currentLevel + 1 >= LEVEL_COUNT);
        int a = menuUpdate(winMenu, dt);
        if (a == 0) {
            if (last) return ScreenType::LEVELSELECT;
            currentLevel++;
            loadLevel(currentLevel);
        }
        if (a == 1) return ScreenType::LEVELSELECT;
        return ScreenType::NONE;
    }

    if (lost) {
        lostAnim += (1.0f - lostAnim) * (1.0f - expf(-10.0f * dt));
        lostMenu.offset.y = (1.0f - easeOutBack(lostAnim)) * MODAL_SLIDE;
        int a = menuUpdate(lostMenu, dt);
        if (a == 0) loadLevel(currentLevel);
        if (a == 1) return ScreenType::TITLE;
        return ScreenType::NONE;
    }

    Vector2 mouse = GetMousePosition();
    bool overPause = CheckCollisionPointRec(mouse, pauseBtn());

    if (IsKeyPressed(KEY_ESCAPE) || (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && overPause)) {
        paused = true;
        openPause();
        return ScreenType::NONE;
    }

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !overPause) {
        pressStart = mouse;
        dragging   = false;
        Hex hx = pixelToHex(mouse, origin);
        int idx = cellIndexAt(board, hx);
        pressIdx = (idx >= 0 && board.cells[idx].value > 0 &&
                    !board.cells[idx].isStone && !board.cells[idx].isCursed) ? idx : -1;
    }

    if (pressIdx >= 0 && IsMouseButtonDown(MOUSE_LEFT_BUTTON) && !dragging) {
        float dx = mouse.x - pressStart.x, dy = mouse.y - pressStart.y;
        if (dx * dx + dy * dy > 8 * 8) {
            dragging = true;
            selectedIdx = pressIdx;
        }
    }

    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        Hex hx = pixelToHex(mouse, origin);
        int target = cellIndexAt(board, hx);
        bool targetTile = (target >= 0 && board.cells[target].value > 0 &&
                           !board.cells[target].isStone && !board.cells[target].isCursed);

        if (dragging && pressIdx >= 0) {
            if (targetTile && target != pressIdx &&
                board.cells[pressIdx].value == board.cells[target].value &&
                boardAdjacent(board, board.cells[pressIdx].pos, board.cells[target].pos)) {
                doMerge(pressIdx, target);
            } else {
                selectedIdx = -1;
            }
        } else {
            int t = targetTile ? target : -1;
            if (t < 0)                      selectedIdx = -1;
            else if (selectedIdx == -1)     selectedIdx = t;
            else if (selectedIdx == t)      selectedIdx = -1;
            else if (board.cells[selectedIdx].value == board.cells[t].value &&
                     boardAdjacent(board, board.cells[selectedIdx].pos, board.cells[t].pos))
                doMerge(selectedIdx, t);
            else                            selectedIdx = t;
        }

        pressIdx = -1;
        dragging = false;
    }

    return ScreenType::NONE;
}

void GameScreen::drawPauseButton() {
    Rectangle r = pauseBtn();
    float dt = GetFrameTime();
    bool hovered = CheckCollisionPointRec(GetMousePosition(), r);
    if (hovered && !pauseBtnHov) pauseBtnWob = PAUSE_WOB_DUR;
    pauseBtnHov = hovered;

    pauseBtnAnim += ((hovered ? 1.0f : 0.0f) - pauseBtnAnim) * (1.0f - expf(-14.0f * dt));
    if (pauseBtnWob > 0) pauseBtnWob -= dt;

    float wv    = (pauseBtnWob > 0) ? wobbleAt(1.0f - pauseBtnWob / PAUSE_WOB_DUR) : 0.0f;
    float tx    = 2.0f * wv;
    float angle = 2.0f * wv;

    float cx = r.x + r.width  / 2.0f + tx;
    float cy = r.y + r.height / 2.0f;

    Texture2D tex = primaryButtonTexture();
    unsigned char g = (unsigned char)(255.0f - 50.0f * pauseBtnAnim);
    Color tint = { g, g, g, 255 };
    DrawTexturePro(tex, { 0, 0, (float)tex.width, (float)tex.height },
                   { cx, cy, r.width, r.height }, { r.width / 2, r.height / 2 }, angle, tint);

    float fs = 24.0f, spacing = fs * 0.04f;
    Font f = titleFont();
    Vector2 m = MeasureTextEx(f, "Pause", fs, spacing);
    DrawTextPro(f, "Pause", { cx, cy }, { m.x / 2, m.y / 2 }, angle, fs, spacing, INK);
}

void GameScreen::draw() {
    ClearBackground(PAPER);

    titleDraw(TextFormat("Level %d / %d", currentLevel + 1, LEVEL_COUNT), 24, 22, 30, INK);
    int gi = goalIndex(board);
    if (gi >= 0) titleDraw(TextFormat("Target %d", board.cells[gi].goalValue), 24, 66, 22, INK);
    titleDraw(TextFormat("Moves %d", board.movesLeft), 24, 96, 22, INK);

    for (int i = 0; i < board.cellCount; i++) {
        const Cell& cell = board.cells[i];
        if (!cell.exists) continue;
        Vector2 c = hexToPixel(cell.pos, origin);
        if (cell.value > 0 || cell.isStone) {
            drawHexTile(c, (i == selectedIdx) ? 1.08f : 1.0f, WHITE);
            if (cell.value > 0)
                titleDrawCenteredAt(TextFormat("%d", cell.value), c.x, c.y, HEX_SIZE * 0.85f, INK);
        }
        if (cell.isGoal) drawFlag(c);
        if (i == selectedIdx) DrawPolyLinesEx(c, 6, HEX_SIZE, -90.0f, 4.0f, HEXRED);
    }

    if (dragging && pressIdx >= 0 && board.cells[pressIdx].value > 0) {
        Vector2 m   = GetMousePosition();
        Vector2 src = hexToPixel(board.cells[pressIdx].pos, origin);
        DrawLineEx(src, m, 3.0f, Fade(HEXRED, 0.35f));
        drawHexTile(m, 0.95f, Fade(WHITE, 0.9f));
        titleDrawCenteredAt(TextFormat("%d", board.cells[pressIdx].value),
                            m.x, m.y, HEX_SIZE * 0.8f, INK);
    }

    drawPauseButton();

    if (paused) {
        dimAndPanel("Paused", pauseAnim, pauseMenu.offset.y);
        menuDraw(pauseMenu);
    } else if (won) {
        bool last = (currentLevel + 1 >= LEVEL_COUNT);
        dimAndPanel(last ? "You finished!" : "Solved!", winAnim, winMenu.offset.y);
        menuDraw(winMenu);
    } else if (lost) {
        dimAndPanel("No moves left", lostAnim, lostMenu.offset.y);
        menuDraw(lostMenu);
    }
}
