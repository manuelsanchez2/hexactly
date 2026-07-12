#include "game_screen.h"
#include "config.h"
#include "levels.h"
#include "tween.h"
#include "audio.h"
#include "screen.h"
#include "progress.h"
#include "gamefont.h"
#include "raylib.h"
#include <cmath>

#if defined(HEX_DEV)
#include <string>
#include <unordered_set>
#include <vector>
#endif

static const float D2R = 3.14159265f / 180.0f;
static const float SLIDE_DUR = 0.15f;
static const float POP_DUR   = 0.22f;
static const float PAUSE_SLIDE = 380.0f;
static const float BOARD_DROP  = 80.0f;

static const float BOOM_DUR   = 0.90f;
static const float DEFUSE_DUR = 0.55f;

static const float CELEBRATE_DUR = 1.10f;
static const float SWAP_DUR      = 0.42f;
static const float SWAP_DIST     = SCREEN_WIDTH + 160.0f;
static const float PRAISE_IN     = 0.35f;
static const float HALO_DUR      = 0.55f;
static const float CONFETTI_LIFE = 0.90f;

static float clamp01(float t) { return t < 0 ? 0 : (t > 1 ? 1 : t); }

static Rectangle pausePanel() { return { SCREEN_WIDTH/2.0f - 170, SCREEN_HEIGHT/2.0f - 140, 340, 300 }; }
static Rectangle rulesPanel() { return { SCREEN_WIDTH/2.0f - 265, SCREEN_HEIGHT/2.0f - 240, 530, 480 }; }
static Rectangle congratsPanel() { return { SCREEN_WIDTH/2.0f - 260, SCREEN_HEIGHT/2.0f - 150, 520, 300 }; }

static Rectangle rectOf(const Layout& L, const char* id, Rectangle def) {
    const LayoutElement* e = layoutFind(L, id);
    return e ? (Rectangle){ e->x, e->y, e->w, e->h } : def;
}

static void hexCorners(Vector2 c, float size, float rotDeg, Vector2 out[6]) {
    for (int i = 0; i < 6; i++) {
        float a = (60.0f * i - 90.0f + rotDeg) * D2R;
        out[i] = (Vector2){ c.x + size * cosf(a), c.y + size * sinf(a) };
    }
}
static void hexOutline(Vector2 c, float size, float rotDeg, float thick, Color color) {
    Vector2 p[6]; hexCorners(c, size, rotDeg, p);
    for (int i = 0; i < 6; i++) DrawLineEx(p[i], p[(i + 1) % 6], thick, color);
}

static float wobbleDeg(Hex h) {
    int n = h.q * 7 + h.r * 13;
    return (float)((n % 5) - 2);
}

static const float TILE_SCALE = 96.0f / 46.0f;
static void drawHexTile(Vector2 c, float scale, Color tint, float rotDeg = 0.0f) {
    Texture2D t = hexTileTexture();
    float S = HEX_SIZE * TILE_SCALE * scale;
    DrawTexturePro(t, { 0, 0, (float)t.width, (float)t.height },
                   { c.x, c.y, S, S }, { S / 2, S / 2 }, rotDeg, tint);
}

static const float TILE_WOB_DUR  = 0.25f;
static const float TILE_WOB_DIST = 3.0f;
static const float TILE_WOB_ROT  = 4.0f;

static const float MOVES_WOB_DUR = 0.35f;   // moves-left number shake per spent move

static void drawFlag(Vector2 c, float rotDeg = 0.0f) {
    Texture2D t = flagGoalTexture();
    float S = t.width / 3.0f;
    Vector2 pos = { c.x + HEX_SIZE * 0.40f, c.y + HEX_SIZE * 0.38f };
    DrawTexturePro(t, { 0, 0, (float)t.width, (float)t.height },
                   { pos.x, pos.y, S, S }, { S / 2, S / 2 }, rotDeg, WHITE);
}

static void drawWall(Hex a, Hex b, Vector2 origin) {
    Vector2 c1 = hexToPixel(a, origin), c2 = hexToPixel(b, origin);
    Vector2 mid = { (c1.x + c2.x) / 2, (c1.y + c2.y) / 2 };
    Vector2 d = { c2.x - c1.x, c2.y - c1.y };
    float len = sqrtf(d.x*d.x + d.y*d.y);
    if (len < 0.001f) return;
    d.x /= len; d.y /= len;
    Vector2 perp = { -d.y, d.x };
    float half = HEX_SIZE * 0.55f;
    DrawLineEx({ mid.x - perp.x*half, mid.y - perp.y*half },
               { mid.x + perp.x*half, mid.y + perp.y*half }, 6.0f, INK);
}

static void drawPortalMarkers(const BoardState& b, Hex pos, Vector2 at) {
    const float S = 16.0f;
    const float D = HEX_SIZE * 0.50f;
    for (int k = 0; k < b.portalCount; k++) {
        if (!hexEqual(b.portals[k].a, pos) && !hexEqual(b.portals[k].b, pos)) continue;
        bool      isA = (k % 2 == 0);
        Texture2D t   = isA ? portalATexture() : portalBTexture();
        Vector2   p   = isA ? (Vector2){ at.x - D, at.y }
                            : (Vector2){ at.x - 1, at.y - D - 3 };
        DrawTexturePro(t, { 0, 0, (float)t.width, (float)t.height },
                       { p.x, p.y, S, S }, { S / 2, S / 2 }, 0.0f, WHITE);
    }
}

// A bomb hex: dark tile, black ball with a lit fuse, and the defuse number.
static void drawBomb(const Bomb& bomb, Vector2 origin) {
    Vector2 c = hexToPixel(bomb.pos, origin);
    c.y += sinf((float)GetTime() * 2.0f + (bomb.pos.q + bomb.pos.r)) * 1.5f;
    float rot = wobbleDeg(bomb.pos);

    drawHexTile(c, 1.0f, (Color){ 110, 105, 98, 255 }, rot);
    DrawCircleV({ c.x, c.y + 2 }, HEX_SIZE * 0.44f, INK);

    Vector2 base = { c.x + HEX_SIZE * 0.10f, c.y - HEX_SIZE * 0.36f };
    Vector2 tip  = { c.x + HEX_SIZE * 0.30f, c.y - HEX_SIZE * 0.54f };
    DrawLineEx(base, tip, 3.0f, INK);
    float pulse = 0.5f + 0.5f * sinf((float)GetTime() * 9.0f);
    DrawCircleV(tip, 2.5f + 2.5f * pulse, (Color){ 240, 185, 40, 255 });

    titleDrawCenteredAtRot(TextFormat("%d", bomb.value), c.x, c.y + 2,
                           HEX_SIZE * 0.55f, rot, PAPER);
}

// Pulsing red rim on every tile an armed bomb currently freezes.
static void drawDangerRim(const BoardState& b, Hex pos, Vector2 at, float rot) {
    if (!bombGuards(b, pos)) return;
    float pulse = 0.5f + 0.5f * sinf((float)GetTime() * 3.0f);
    hexOutline(at, HEX_SIZE - 5, rot, 2.5f, Fade(HEXRED, 0.20f + 0.25f * pulse));
}

static void drawTargetBadge(int target, Vector2 c) {
    float r = 44.0f;
    const char* lbl = "target";
    titleDraw(lbl, c.x - titleMeasure(lbl, 20).x / 2, c.y - r - 26, 20, INK);
    DrawPolyLinesEx(c, 6, r, -90.0f, 4.0f, HEXRED);
    titleDrawCenteredAt(TextFormat("%d", target), c.x, c.y, 40.0f, HEXRED);
}

static const char* opGlyph(int op) {
    switch (op) {
        case OP_ADD2: return "+2";
        case OP_SUB2: return "-2";
        case OP_MUL2: return "x2";
        default:      return "";
    }
}

static const float ICON_WOB_DUR  = 0.25f;
static const float ICON_WOB_DIST = 2.0f;
static const float ICON_WOB_ROT  = 3.0f;

static void drawDynLabel(const Layout& L, const char* id, const char* text,
                         Vector2 defPos, float defSize) {
    const LayoutElement* e = layoutFind(L, id);
    float x  = e ? e->x : defPos.x;
    float y  = e ? e->y : defPos.y;
    float sz = e ? e->size : defSize;
    bool center = e ? (e->align == Align::Center) : true;
    float w = titleMeasure(text, sz).x;
    titleDraw(text, center ? x - w / 2 : x, y, sz, INK);
}

GameScreen::GameScreen() {
    layout = loadLayout("game");
    mtime  = layoutFileTime("game");

    float pcx = SCREEN_WIDTH / 2.0f - 120;
    float pcy = SCREEN_HEIGHT / 2.0f;
    menuAdd(pauseMenu, { pcx, pcy - 60, 240, 52 }, "Resume");
    menuAdd(pauseMenu, { pcx, pcy +  4, 240, 52 }, "Restart");
    menuAdd(pauseMenu, { pcx, pcy + 68, 240, 52 }, "Levels");

    Rectangle rp = rulesPanel();
    menuAdd(rulesMenu, { SCREEN_WIDTH/2.0f - 100, rp.y + rp.height - 66, 200, 52 }, "Got it!");

    Rectangle cp = congratsPanel();
    menuAdd(congratsMenu, { SCREEN_WIDTH/2.0f - 100, cp.y + cp.height - 76, 200, 52 }, "Let's go!");

    daily = gDailyMode;
    if (daily) {
        loadDaily();
    } else {
        currentLevel = gStartLevel;
        loadLevel(currentLevel);
    }

    Progress pr = loadProgress();
    if (daily) {
        if (!pr.seenDailyOps) {
            pr.seenDailyOps = true;
            saveProgress(pr);
            if (!rulesActive) openRules();   // openRules explains operators when daily
        }
    } else if (!pr.seenRules) {
        pr.seenRules = true;
        saveProgress(pr);
        // loadLevel above may already have queued a first-time tip; don't clobber it.
        if (!rulesActive) openRules();
    }
}

void GameScreen::openRules() {
    if (daily) {
        ovTitle = "Daily: Operators";
        ovLines = {
            "Drag a number onto a neighbouring",
            "operator tile to transform it:",
            "",
            "   x2  double        +2  add two",
            "   -2  subtract two",
            "",
            "Hit the TARGET exactly to win.",
        };
    } else {
        ovTitle = "How to Play";
        ovLines = {
            "1.  Merge two equal neighbours - they double.",
            "2.  Click two tiles, or drag one onto the other.",
            "3.  Reach the TARGET number to win.",
            "4.  Leftover tiles are fine.",
            "5.  Walls block a shared edge.",
            "6.  Portals link the two marked cells.",
        };
    }
    if (!rulesMenu.buttons.empty()) rulesMenu.buttons[0].text = "Got it!";
    presentOverlay();
}

void GameScreen::showTip(int kind) {
    if (kind == 2) {
        ovTitle = "New: Bombs";
        ovLines = {
            "A bomb freezes its neighbourhood:",
            "merging any tile that touches it",
            "sets it off - and the level is lost.",
            "",
            "To defuse it, land its exact number",
            "on a cell right next to it. That",
            "merge is safe, and the bomb is gone.",
            "",
            "A wall shields a tile from a bomb.",
        };
        if (!rulesMenu.buttons.empty()) rulesMenu.buttons[0].text = "Ok!";
        presentOverlay();
        return;
    }
    if (kind == 1) {
        ovTitle = "New: Portals";
        ovLines = {
            "A portal links two far-apart cells,",
            "each marked with a matching symbol.",
            "",
            "Linked cells count as neighbours - a",
            "tile on one side can merge with a",
            "match on the other, straight across",
            "the gap between them.",
            "",
            "It reaches numbers you could never",
            "bring together otherwise.",
        };
    } else {
        ovTitle = "New: Walls";
        ovLines = {
            "A wall is the thick line drawn along",
            "the shared edge between two cells.",
            "",
            "Tiles on opposite sides can never",
            "merge across it - even when their",
            "numbers are a perfect match.",
            "",
            "There is always another way around,",
            "so plan your route carefully.",
        };
    }
    if (!rulesMenu.buttons.empty()) rulesMenu.buttons[0].text = "Ok!";
    presentOverlay();
}

// Shown once, after the very last level is cleared. Dismissing it returns to
// the level-select screen.
void GameScreen::showFinal() {
    ovTitle = "That's all!";
    ovLines = {
        "Thanks for playing - I really",
        "hope you liked it!",
        "",
        "Stay tuned for further updates",
        "on the game.",
        "",
        "And feel free to leave me some",
        "feedback on the itch page!",
    };
    if (!rulesMenu.buttons.empty()) rulesMenu.buttons[0].text = "Back to Levels";
    endGame = true;
    phase   = PH_DONE;
    presentOverlay();
}

void GameScreen::presentOverlay() {
    rulesActive = true; rulesClosing = false; rulesAnim = 0.0f;
    rulesMenu.focus = 0; rulesMenu.kbFocus = false; rulesMenu.litPrev = -1;
    for (Button& b : rulesMenu.buttons) b.anim = 0.0f;
}

void GameScreen::closeRules() { rulesClosing = true; }

void GameScreen::loadLevel(int idx)     { applyLevel(LEVELS[idx]); }
void GameScreen::loadDaily()            { applyLevel(makeDaily(dailyIndex())); }
void GameScreen::reloadCurrent()        { if (daily) loadDaily(); else loadLevel(currentLevel); }

void GameScreen::applyLevel(const LevelDef& L) {
    board.cellCount = L.cellCount;
    board.movesLeft = L.moveLimit;
    moveLimit       = L.moveLimit;
    movesWob        = 0.0f;
    for (int i = 0; i < L.cellCount; i++) {
        board.cells[i].pos       = { L.cells[i].q, L.cells[i].r };
        board.cells[i].value     = L.cells[i].value;
        board.cells[i].exists    = true;
        board.cells[i].isGoal    = (L.cells[i].flags & F_GOAL) != 0;
        board.cells[i].goalValue = L.cells[i].goalValue;
        board.cells[i].op        = L.cells[i].op;
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
    board.bombCount = L.bombCount;
    for (int i = 0; i < L.bombCount; i++) {
        board.bombs[i].pos   = { L.bombs[i].q, L.bombs[i].r };
        board.bombs[i].value = L.bombs[i].value;
        board.bombs[i].armed = true;
    }

    int gi = goalIndex(board);
    if (gi >= 0 && board.cells[gi].goalValue == 0) {
        int sum = 0;
        for (int i = 0; i < board.cellCount; i++) sum += board.cells[i].value;
        board.cells[gi].goalValue = sum;
    }

    Vector2 sum = { 0, 0 };
    float minX = 1e9f, maxX = -1e9f, minY = 1e9f, maxY = -1e9f;
    for (int i = 0; i < board.cellCount + board.bombCount; i++) {
        Hex h = (i < board.cellCount) ? board.cells[i].pos
                                      : board.bombs[i - board.cellCount].pos;
        Vector2 px = hexToPixel(h, (Vector2){ 0, 0 });
        sum.x += px.x; sum.y += px.y;
        if (px.x < minX) minX = px.x;  if (px.x > maxX) maxX = px.x;
        if (px.y < minY) minY = px.y;  if (px.y > maxY) maxY = px.y;
    }
    Vector2 avg = { sum.x / (board.cellCount + board.bombCount),
                    sum.y / (board.cellCount + board.bombCount) };

    float ox = SCREEN_WIDTH  / 2.0f - avg.x;
    float oy = SCREEN_HEIGHT / 2.0f + BOARD_DROP - avg.y;

    const float PAD  = 54.0f;
    const float TOP  = 140.0f,               BOT   = SCREEN_HEIGHT - 36.0f;
    const float LEFT = 24.0f,                RIGHT = SCREEN_WIDTH  - 24.0f;
    { float lo = LEFT + PAD - minX, hi = RIGHT - PAD - maxX; if (ox > hi) ox = hi; if (ox < lo) ox = lo; }
    { float lo = TOP  + PAD - minY, hi = BOT   - PAD - maxY; if (oy > hi) oy = hi; if (oy < lo) oy = lo; }

    origin = (Vector2){ ox, oy };

    selectedIdx = -1;
    undoCount   = 0;
    phase       = PH_PLAYING;
    popIdx      = -1;
    popTimer    = 0.0f;
    for (int i = 0; i < MAX_CELLS; i++) cellWob[i] = 0.0f;
    hoverPrev = -1;

    confettiCount = 0;
    haloTimer     = 0.0f;
    exploded      = false;
    boomTimer     = 0.0f;
    defuseTimer   = 0.0f;

    // First-time mechanic tips: show once when a mechanic first appears,
    // then remember it so it never interrupts a replay.
    Progress pr = loadProgress();
    if (board.wallCount > 0 && !pr.seenWalls) {
        pr.seenWalls = true;
        saveProgress(pr);
        showTip(0);
    } else if (board.portalCount > 0 && !pr.seenPortals) {
        pr.seenPortals = true;
        saveProgress(pr);
        showTip(1);
    } else if (board.bombCount > 0 && !pr.seenBombs) {
        pr.seenBombs = true;
        saveProgress(pr);
        showTip(2);
    }
}

void GameScreen::pushUndo() {
    if (undoCount < MAX_UNDO) undoStack[undoCount++] = board;
}

void GameScreen::doUndo() {
    if (undoCount > 0) {
        board = undoStack[--undoCount];
        phase = PH_PLAYING;
        selectedIdx = -1;
        exploded  = false;
        boomTimer = 0.0f;
    }
}

void GameScreen::doMerge(int fromIdx, int toIdx) {
    pushUndo();
    int v = board.cells[fromIdx].value;
    board.cells[toIdx].value  *= 2;
    board.cells[fromIdx].value = 0;
    board.movesLeft--;
    movesWob = MOVES_WOB_DUR;

    slideActive  = true;
    slideFrom    = hexToPixel(board.cells[fromIdx].pos, origin);
    slideTo      = hexToPixel(board.cells[toIdx].pos, origin);
    slideValue   = v;
    slideLandIdx = toIdx;
    slideT       = 0.0f;

    playMerge();
    selectedIdx = -1;

    BombResult br = applyBombs(board, board.cells[fromIdx].pos,
                               board.cells[toIdx].pos, v * 2);
    if (br.defusedIdx >= 0) {
        defuseTimer  = DEFUSE_DUR;
        defuseCenter = hexToPixel(board.bombs[br.defusedIdx].pos, origin);
    }
    if (br.explodedIdx >= 0) {
        exploded   = true;
        boomTimer  = BOOM_DUR;
        boomCenter = hexToPixel(board.bombs[br.explodedIdx].pos, origin);
        spawnConfetti(boomCenter);
        playInvalid();
        phase = PH_LOST;
        return;
    }
    checkEnd();
}

// Daily challenges only: a value tile is dragged onto a neighbouring operator
// tile, which transforms it and is consumed (the cell becomes a plain value).
void GameScreen::doApply(int fromIdx, int toIdx) {
    pushUndo();
    int v  = board.cells[fromIdx].value;
    int nv = applyOp(board.cells[toIdx].op, v);
    board.cells[toIdx].value   = nv;
    board.cells[toIdx].op      = OP_NONE;   // operator consumed
    board.cells[fromIdx].value = 0;
    board.movesLeft--;
    movesWob = MOVES_WOB_DUR;

    slideActive  = true;
    slideFrom    = hexToPixel(board.cells[fromIdx].pos, origin);
    slideTo      = hexToPixel(board.cells[toIdx].pos, origin);
    slideValue   = v;                       // shows incoming value, then pops to nv
    slideLandIdx = toIdx;
    slideT       = 0.0f;

    playMerge();
    selectedIdx = -1;
    checkEnd();
}

void GameScreen::checkEnd() {
    if (isWon(board)) {
        phase     = PH_CELEBRATE;
        winTimer  = 0.0f;
        haloTimer = HALO_DUR;
        playWin();

        Progress p = loadProgress();
        if (daily) {
            long today = dailyEpochDay();
            if (p.dailyLastDay != today) {            // count each day once
                p.dailyStreak  = (p.dailyLastDay == today - 1) ? p.dailyStreak + 1 : 1;
                p.dailyLastDay = today;
                if (p.dailyStreak > p.dailyBest) p.dailyBest = p.dailyStreak;
            }
        } else {
            if (currentLevel == BEGINNER_COUNT - 1 && !levelDone(p, currentLevel))
                pendingCongrats = true;
            markLevelDone(p, currentLevel);
        }
        saveProgress(p);

        int wi = -1;
        int g = goalIndex(board);
        if (g >= 0 && board.cells[g].goalValue > 0) {
            wi = g;
        } else {
            for (int i = 0; i < board.cellCount; i++)
                if (board.cells[i].exists && board.cells[i].value > 0) { wi = i; break; }
        }
        winCenter = (wi >= 0) ? hexToPixel(board.cells[wi].pos, origin)
                              : (Vector2){ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f };

        bool last = !daily && (currentLevel + 1 >= LEVEL_COUNT);
        static const char* WORDS[] = { "Nice!", "Great!", "Well Done!", "Sweet!", "Boom!" };
        praiseText = last ? "All done!" : WORDS[GetRandomValue(0, 4)];
        praiseRot  = (float)GetRandomValue(-6, 6);

        spawnConfetti(winCenter);
    } else if (board.movesLeft <= 0 || !anyLegalMove(board)) {
        phase = PH_LOST;
    }
}

void GameScreen::spawnConfetti(Vector2 at) {
    Color pal[3] = { INK, HEXRED, (Color){ 240, 185, 40, 255 } };
    confettiCount = 20;
    for (int i = 0; i < confettiCount; i++) {
        Confetti& p = confetti[i];
        p.pos    = at;
        float ang = (float)GetRandomValue(200, 340) * D2R;
        float spd = (float)GetRandomValue(180, 430);
        p.vel    = { cosf(ang) * spd, sinf(ang) * spd };
        p.rot    = (float)GetRandomValue(0, 360);
        p.rotVel = (float)GetRandomValue(-360, 360);
        p.col    = pal[GetRandomValue(0, 2)];
        p.life   = CONFETTI_LIFE * (0.7f + GetRandomValue(0, 60) / 100.0f);
    }
}

void GameScreen::beginSwap() {
    outgoing    = board;
    outOrigin   = origin;
    hasOutgoing = true;
    swapT       = 0.0f;

    if (currentLevel + 1 < LEVEL_COUNT) {
        finishing = false;
        loadLevel(++currentLevel);
        targetOrigin = origin;
        phase = PH_SWAP;
    } else {
        finishing = true;
        phase = PH_SWAP;
    }
}

ScreenType GameScreen::update() {
    float dt = GetFrameTime();
    if (popTimer  > 0) popTimer  -= dt;
    if (starTimer > 0) starTimer -= dt;

    if (slideActive) {
        slideT += dt / SLIDE_DUR;
        if (slideT >= 1.0f) {
            slideActive = false;
            popIdx = slideLandIdx; popTimer = POP_DUR;
            starTimer = POP_DUR;   starCenter = slideTo;
        }
    }

    if (haloTimer   > 0) haloTimer   -= dt;
    if (boomTimer   > 0) boomTimer   -= dt;
    if (defuseTimer > 0) defuseTimer -= dt;
    for (int i = 0; i < confettiCount; i++) {
        Confetti& p = confetti[i];
        p.vel.y += 900.0f * dt;
        p.pos.x += p.vel.x * dt;
        p.pos.y += p.vel.y * dt;
        p.rot   += p.rotVel * dt;
        p.life  -= dt;
    }

    if (phase == PH_CELEBRATE) {
        if (congratsActive) {
            congratsAnim += (1.0f - congratsAnim) * (1.0f - expf(-10.0f * dt));
            congratsMenu.offset.y = (1.0f - easeOutBack(clamp01(congratsAnim))) * PAUSE_SLIDE;
            int a = menuUpdate(congratsMenu, dt);
            if (a == 0 || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)) {
                congratsActive = false;
                beginSwap();
            }
            return ScreenType::NONE;
        }
        winTimer += dt;
        bool tap = winTimer > 0.2f &&
                   (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) ||
                    IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER));
        if (winTimer >= CELEBRATE_DUR || tap) {
            if (daily) return ScreenType::LEVELSELECT;   // dailies don't chain
            if (currentLevel + 1 >= LEVEL_COUNT) {
                showFinal();   // finished the game
            } else if (pendingCongrats) {
                pendingCongrats = false;
                congratsActive  = true;
                congratsAnim    = 0.0f;
                congratsMenu.focus = 0;
                for (Button& b : congratsMenu.buttons) b.anim = 0.0f;
            } else {
                beginSwap();
            }
        }
        return ScreenType::NONE;
    }

    if (phase == PH_SWAP) {
        swapT += dt / SWAP_DUR;
        if (swapT >= 1.0f) {
            if (finishing) return ScreenType::LEVELSELECT;
            phase       = PH_PLAYING;
            origin      = targetOrigin;
            hasOutgoing = false;
        }
        return ScreenType::NONE;
    }

    if (!editor.active) {
        long t = layoutFileTime("game");
        if (t != 0 && t != mtime) { layout = loadLayout("game"); mtime = t; }
    }
    if (editorUpdate(editor, layout)) mtime = layoutFileTime("game");
    if (editor.active) return ScreenType::NONE;

    if (rulesActive) {
        float target = rulesClosing ? 0.0f : 1.0f;
        rulesAnim += (target - rulesAnim) * (1.0f - expf(-10.0f * dt));
        rulesMenu.offset.y = (1.0f - easeOutBack(clamp01(rulesAnim))) * PAUSE_SLIDE;
        if (rulesClosing && rulesAnim < 0.02f) {
            rulesActive = false; rulesClosing = false;
            if (endGame) { endGame = false; return ScreenType::LEVELSELECT; }
        } else {
            int a = menuUpdate(rulesMenu, dt);
            if (!rulesClosing && (a == 0 || IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE))) closeRules();
        }
        return ScreenType::NONE;
    }

    for (int i = 0; i < board.cellCount; i++) if (cellWob[i] > 0) cellWob[i] -= dt;
    if (movesWob > 0) movesWob -= dt;
    if (!paused && phase == PH_PLAYING) {
        Hex hh = pixelToHex(GetMousePosition(), origin);
        int hi = cellIndexAt(board, hh);
        if (hi >= 0 && board.cells[hi].value == 0) hi = -1;
        if (hi != hoverPrev) { if (hi >= 0) cellWob[hi] = TILE_WOB_DUR; hoverPrev = hi; }
    } else {
        hoverPrev = -1;
    }

    if (paused) {
        pauseAnim += (1.0f - pauseAnim) * (1.0f - expf(-10.0f * dt));
        pauseMenu.offset.y = (1.0f - easeOutBack(pauseAnim)) * PAUSE_SLIDE;

        int a = menuUpdate(pauseMenu, dt);
        if (a == 0 || IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE)) paused = false;
        if (a == 1) { reloadCurrent(); paused = false; }
        if (a == 2) return ScreenType::LEVELSELECT;
        return ScreenType::NONE;
    }

    Rectangle infoR  = rectOf(layout, "info",  { 24, 24, 60, 60 });
    Rectangle pauseR = rectOf(layout, "pause", { SCREEN_WIDTH - 84.0f, 24, 60, 60 });
    Rectangle undoR  = rectOf(layout, "undo",  { 30, SCREEN_HEIGHT - 90.0f, 60, 60 });

    {
        Vector2 m = GetMousePosition();
        bool overPause = CheckCollisionPointRec(m, pauseR);
        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE) ||
            (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && overPause)) {
            paused = true;
            pauseAnim = 0.0f;
            pauseMenu.focus = 0;
            for (Button& b : pauseMenu.buttons) b.anim = 0.0f;
            return ScreenType::NONE;
        }
    }

    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) &&
        CheckCollisionPointRec(GetMousePosition(), infoR)) {
        openRules();
        return ScreenType::NONE;
    }

    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) &&
        CheckCollisionPointRec(GetMousePosition(), undoR)) {
        doUndo();
        return ScreenType::NONE;
    }
    if (IsKeyPressed(KEY_U) || IsKeyPressed(KEY_Z)) doUndo();
    if (IsKeyPressed(KEY_R)) reloadCurrent();
#if defined(HEX_DEV)
    if (IsKeyPressed(KEY_D) && phase == PH_PLAYING) debugSolve();
#endif

    bool overHud = false;
    Vector2 mouse = GetMousePosition();
    if (CheckCollisionPointRec(mouse, infoR))  overHud = true;
    if (CheckCollisionPointRec(mouse, pauseR)) overHud = true;
    if (CheckCollisionPointRec(mouse, undoR))  overHud = true;

    if (phase == PH_PLAYING) {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !overHud) {
            pressStart = mouse;
            dragging   = false;
            Hex hx = pixelToHex(mouse, origin);
            int idx = cellIndexAt(board, hx);
            pressIdx = (idx >= 0 && board.cells[idx].value > 0) ? idx : -1;
        }

        if (pressIdx >= 0 && IsMouseButtonDown(MOUSE_LEFT_BUTTON) && !dragging) {
            float dx = mouse.x - pressStart.x, dy = mouse.y - pressStart.y;
            if (dx*dx + dy*dy > 8*8) {
                dragging = true;
                selectedIdx = pressIdx;
            }
        }

        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            Hex hx = pixelToHex(mouse, origin);
            int target = cellIndexAt(board, hx);
            bool targetTile = (target >= 0 && board.cells[target].value > 0);

            auto isOpCell = [&](int i) {
                return i >= 0 && board.cells[i].exists &&
                       board.cells[i].value == 0 && board.cells[i].op != OP_NONE;
            };
            auto legal = [&](int a, int b) {
                return a >= 0 && b >= 0 && a != b &&
                       board.cells[a].value > 0 && board.cells[b].value > 0 &&
                       board.cells[a].value == board.cells[b].value &&
                       boardAdjacent(board, board.cells[a].pos, board.cells[b].pos);
            };
            auto legalOp = [&](int a, int b) {
                return a >= 0 && b >= 0 && a != b &&
                       board.cells[a].value > 0 &&
                       isOpCell(b) && opAllowed(board.cells[b].op, board.cells[a].value) &&
                       boardAdjacent(board, board.cells[a].pos, board.cells[b].pos);
            };

            if (dragging && pressIdx >= 0) {
                if (legal(pressIdx, target)) {
                    doMerge(pressIdx, target);
                } else if (legalOp(pressIdx, target)) {
                    doApply(pressIdx, target);
                } else {
                    if ((targetTile || isOpCell(target)) && target != pressIdx) playInvalid();
                    selectedIdx = -1;
                }
            } else if (selectedIdx >= 0 && isOpCell(target)) {
                if (legalOp(selectedIdx, target)) doApply(selectedIdx, target);
                else                              playInvalid();
            } else {
                int t = targetTile ? target : -1;
                if (t < 0)                       selectedIdx = -1;
                else if (selectedIdx == -1)      selectedIdx = t;
                else if (selectedIdx == t)       selectedIdx = -1;
                else if (legal(selectedIdx, t))  doMerge(selectedIdx, t);
                else { playInvalid();            selectedIdx = t; }
            }

            pressIdx = -1;
            dragging = false;
        }
    }

    return ScreenType::NONE;
}

void GameScreen::draw() {
    ClearBackground(PAPER);
    titleDrawCentered("HEXACTLY", 16, 36, INK);

    if (phase == PH_SWAP) {
        float e = easeOutQuad(clamp01(swapT));
        if (hasOutgoing)
            drawStaticBoard(outgoing, { outOrigin.x - e * SWAP_DIST, outOrigin.y });
        if (!finishing)
            drawStaticBoard(board, { targetOrigin.x + (1.0f - e) * SWAP_DIST, targetOrigin.y });
        drawConfetti();
        drawPraise(1.0f - clamp01(swapT), clamp01(swapT) * 120.0f);
        return;
    }

    drawDynLabel(layout, "level",
                 daily ? TextFormat("Daily #%d", dailyIndex() + 1)
                       : TextFormat("Level %d", currentLevel + 1),
                 { SCREEN_WIDTH / 2.0f, 54 }, 28);
    bool lastMove = (phase == PH_PLAYING && board.movesLeft == 1 && moveLimit > 1);

    if (phase != PH_CELEBRATE && phase != PH_DONE) {
        drawDynLabel(layout, "moves", "moves left:", { SCREEN_WIDTH / 2.0f, 78 }, 20);

        const LayoutElement* e = layoutFind(layout, "movesval");
        float mx = e ? e->x    : SCREEN_WIDTH / 2.0f;
        float my = e ? e->y    : 102.0f;
        float ms = e ? e->size : 60.0f;

        float wv   = (movesWob > 0) ? wobbleAt(1.0f - movesWob / MOVES_WOB_DUR) : 0.0f;
        float rot  = 6.0f * wv;
        float dx   = 4.0f * wv;
        float size = ms;
        Color col  = INK;
        if (lastMove) {
            float pulse = 0.5f + 0.5f * sinf((float)GetTime() * 5.0f);
            size *= 1.0f + 0.10f * pulse;
            col   = HEXRED;
        }
        titleDrawCenteredAtRot(TextFormat("%d", board.movesLeft),
                               mx + dx, my + ms * 0.5f, size, rot, col);
    }

    for (int i = 0; i < board.cellCount; i++) {
        const Cell& cell = board.cells[i];
        if (!cell.exists) continue;

        Vector2 c = hexToPixel(cell.pos, origin);
        c.y += sinf((float)GetTime() * 2.0f + (cell.pos.q + cell.pos.r)) * 1.5f;
        float rot = wobbleDeg(cell.pos);

        if (cell.value == 0 && cell.op != OP_NONE) {
            drawHexTile(c, 1.0f, WHITE, rot);
            titleDrawCenteredAtRot(opGlyph(cell.op), c.x, c.y,
                                   HEX_SIZE * 0.66f, rot, HEXRED);
            if (cell.isGoal) drawFlag(c, rot);
            drawPortalMarkers(board, cell.pos, c);
            continue;
        }

        if (cell.value == 0) {
            if (cell.isGoal) {
                drawFlag(c);
            } else {
                hexOutline(c, HEX_SIZE - 3, rot, 2.0f, (Color){ 210, 208, 200, 255 });
            }
            continue;
        }

        int displayValue = cell.value;
        if (slideActive && i == slideLandIdx) displayValue = slideValue;

        float pop = 1.0f;
        if (i == popIdx && popTimer > 0) pop = 1.0f + 0.30f * (popTimer / POP_DUR);

        bool anySel    = (selectedIdx >= 0);
        bool isSel     = (i == selectedIdx);
        bool isPartner = anySel && !isSel &&
                         board.cells[selectedIdx].value == cell.value &&
                         boardAdjacent(board, board.cells[selectedIdx].pos, cell.pos);
        bool dim       = anySel && !isSel && !isPartner;

        Color tileTint = dim ? (Color){ 140, 140, 140, 255 } : WHITE;
        Color numCol   = dim ? (Color){ 120, 120, 120, 255 } : INK;

        float tileScale = pop * (isSel ? 1.10f : 1.0f);
        Vector2 dc = c;
        if (isSel) {
            Texture2D tx = hexTileTexture();
            float S = HEX_SIZE * TILE_SCALE * tileScale;
            DrawTexturePro(tx, { 0, 0, (float)tx.width, (float)tx.height },
                           { c.x + 4, c.y + 9, S, S }, { S / 2, S / 2 }, 0.0f,
                           Fade(BLACK, 0.28f));
            dc.y -= 6;
        }

        float wv   = (cellWob[i] > 0) ? wobbleAt(1.0f - cellWob[i] / TILE_WOB_DUR) : 0.0f;
        float wrot = TILE_WOB_ROT * wv;
        Vector2 wc = { dc.x + TILE_WOB_DIST * wv, dc.y };

        drawHexTile(wc, tileScale, tileTint, wrot);

        if (cell.isGoal) drawFlag(wc, wrot);

        titleDrawCenteredAtRot(TextFormat("%d", displayValue), wc.x, wc.y,
                               HEX_SIZE * 0.85f, wrot, numCol);

        drawPortalMarkers(board, cell.pos, wc);
        drawDangerRim(board, cell.pos, wc, wrot);
    }

    for (int i = 0; i < board.bombCount; i++) {
        if (board.bombs[i].armed) drawBomb(board.bombs[i], origin);
    }

    for (int i = 0; i < board.wallCount; i++) {
        drawWall(board.walls[i].a, board.walls[i].b, origin);
    }

    if (dragging && pressIdx >= 0 && board.cells[pressIdx].value > 0) {
        Vector2 m   = GetMousePosition();
        Vector2 src = hexToPixel(board.cells[pressIdx].pos, origin);
        DrawLineEx(src, m, 3.0f, Fade(HEXRED, 0.35f));

        drawHexTile(m, 0.95f, Fade(WHITE, 0.9f));
        titleDrawCenteredAt(TextFormat("%d", board.cells[pressIdx].value),
                            m.x, m.y, HEX_SIZE * 0.8f, INK);
    }

    if (slideActive) {
        float e = slideT * slideT;
        Vector2 p = { lerpf(slideFrom.x, slideTo.x, e),
                      lerpf(slideFrom.y, slideTo.y, e) };
        drawHexTile(p, 1.0f, WHITE);
        titleDrawCenteredAt(TextFormat("%d", slideValue), p.x, p.y,
                            HEX_SIZE * 0.85f, INK);
    }

    if (starTimer > 0) {
        float a = starTimer / POP_DUR;
        float r0 = HEX_SIZE * 0.5f;
        float r1 = HEX_SIZE * (0.9f + (1.0f - a) * 0.6f);
        for (int i = 0; i < 6; i++) {
            float ang = (60.0f * i - 90.0f) * D2R;
            Vector2 d = { cosf(ang), sinf(ang) };
            Vector2 p0 = { starCenter.x + d.x * r0, starCenter.y + d.y * r0 };
            Vector2 p1 = { starCenter.x + d.x * r1, starCenter.y + d.y * r1 };
            DrawLineEx(p0, p1, 3.0f, Fade(HEXRED, a));
        }
    }

    if (defuseTimer > 0) {
        float a = defuseTimer / DEFUSE_DUR;
        float r = HEX_SIZE * (0.5f + (1.0f - a) * 1.2f);
        DrawRing(defuseCenter, r - 3, r, 0, 360, 36,
                 Fade((Color){ 240, 185, 40, 255 }, a));
    }

    if (boomTimer > 0) {
        float a = boomTimer / BOOM_DUR;
        float r = HEX_SIZE * (0.6f + (1.0f - a) * 3.2f);
        DrawRing(boomCenter, r - 5, r, 0, 360, 48, Fade(HEXRED, a));
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(HEXRED, 0.30f * a));
        drawConfetti();
    }

    if (phase == PH_CELEBRATE) {
        if (haloTimer > 0) {
            float a = haloTimer / HALO_DUR;
            float r = HEX_SIZE * (0.8f + (1.0f - a) * 1.8f);
            DrawRing(winCenter, r - 3, r, 0, 360, 48, Fade(HEXRED, a * 0.8f));
        }
        drawConfetti();
        drawPraise(1.0f, 0.0f);

        if (congratsActive) {
            float a  = clamp01(congratsAnim);
            float dy = congratsMenu.offset.y;

            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.6f * a));

            Rectangle panel = congratsPanel();
            panel.y += dy;
            DrawRectangleRec(panel, Fade(PAPER, a));
            DrawRectangleLinesEx(panel, 2, Fade(INK, a));

            titleDrawCentered("Congratulations!", panel.y + 26, 40, Fade(HEXRED, a));

            static const char* UNLOCK_LINES[] = {
                "You have unlocked the advanced mode,",
                "the real game starts there!",
            };
            float y = panel.y + 96;
            for (const char* line : UNLOCK_LINES) {
                float w = titleMeasure(line, 22).x;
                titleDraw(line, SCREEN_WIDTH / 2.0f - w / 2, y, 22, Fade(INK, a));
                y += 38;
            }

            menuDraw(congratsMenu);
        }
        return;
    }

    int gh = goalIndex(board);
    if (gh >= 0 && board.cells[gh].goalValue > 0) {
        Rectangle tb = rectOf(layout, "target", { 586, 494, 88, 88 });
        drawTargetBadge(board.cells[gh].goalValue,
                        { tb.x + tb.width / 2, tb.y + tb.height / 2 });
    }

    drawIconButton(0, btnUndoTexture(),  rectOf(layout, "undo",  { 30, SCREEN_HEIGHT - 90.0f, 60, 60 }));
    drawIconButton(1, btnPauseTexture(), rectOf(layout, "pause", { SCREEN_WIDTH - 84.0f, 24, 60, 60 }));
    drawIconButton(2, btnInfoTexture(),  rectOf(layout, "info",  { 24, 24, 60, 60 }));

    if (phase == PH_LOST) {
        titleDrawCentered(exploded ? "Boom! Undo or Restart"
                                   : "Stuck! Undo or Restart",
                          SCREEN_HEIGHT - 58.0f, 30, HEXRED);
    }

    if (paused) {
        float alpha = pauseAnim;
        float dy    = pauseMenu.offset.y;

        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.55f * alpha));

        Rectangle panel = pausePanel();
        panel.y += dy;
        DrawRectangleRec(panel, Fade(PAPER, alpha));
        DrawRectangleLinesEx(panel, 2, Fade(INK, alpha));

        titleDrawCentered("Paused", panel.y + 26, 44, Fade(INK, alpha));

        menuDraw(pauseMenu);
    }

    if (rulesActive) {
        float a  = clamp01(rulesAnim);
        float dy = rulesMenu.offset.y;

        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.6f * a));

        // Size the panel width to the widest line so rows don't trail off into
        // dead space. Height/button stay fixed; the panel stays screen-centered.
        const float PADX = 36.0f;
        float maxw = titleMeasure(ovTitle.c_str(), 40).x;
        for (const std::string& ln : ovLines) {
            float w = titleMeasure(ln.c_str(), 20).x;
            if (w > maxw) maxw = w;
        }
        float panelW = maxw + PADX * 2.0f;
        if (panelW > 580.0f) panelW = 580.0f;

        Rectangle panel = rulesPanel();
        panel.width = panelW;
        panel.x     = SCREEN_WIDTH / 2.0f - panelW / 2.0f;
        panel.y += dy;
        DrawRectangleRec(panel, Fade(PAPER, a));
        DrawRectangleLinesEx(panel, 2, Fade(INK, a));

        titleDrawCentered(ovTitle.c_str(), panel.y + 22, 40, Fade(INK, a));

        float y = panel.y + 84;
        for (const std::string& ln : ovLines) {
            if (!ln.empty()) titleDraw(ln.c_str(), panel.x + PADX, y, 20, Fade(INK, a));
            y += 32;
        }

        menuDraw(rulesMenu);
    }

    editorDrawOverlay(editor, layout);
}

void GameScreen::drawStaticBoard(const BoardState& b, Vector2 org) {
    for (int i = 0; i < b.cellCount; i++) {
        const Cell& cell = b.cells[i];
        if (!cell.exists) continue;

        Vector2 c = hexToPixel(cell.pos, org);
        c.y += sinf((float)GetTime() * 2.0f + (cell.pos.q + cell.pos.r)) * 1.5f;
        float rot = wobbleDeg(cell.pos);

        if (cell.value == 0 && cell.op != OP_NONE) {
            drawHexTile(c, 1.0f, WHITE, rot);
            titleDrawCenteredAtRot(opGlyph(cell.op), c.x, c.y,
                                   HEX_SIZE * 0.66f, rot, HEXRED);
            if (cell.isGoal) drawFlag(c, rot);
            drawPortalMarkers(b, cell.pos, c);
            continue;
        }
        if (cell.value == 0) {
            if (cell.isGoal) drawFlag(c);
            else hexOutline(c, HEX_SIZE - 3, rot, 2.0f, (Color){ 210, 208, 200, 255 });
            continue;
        }
        drawHexTile(c, 1.0f, WHITE);
        if (cell.isGoal) drawFlag(c);
        titleDrawCenteredAtRot(TextFormat("%d", cell.value), c.x, c.y,
                               HEX_SIZE * 0.85f, 0.0f, INK);
        drawPortalMarkers(b, cell.pos, c);
        drawDangerRim(b, cell.pos, c, rot);
    }
    for (int i = 0; i < b.bombCount; i++) {
        if (b.bombs[i].armed) drawBomb(b.bombs[i], org);
    }
    for (int i = 0; i < b.wallCount; i++)
        drawWall(b.walls[i].a, b.walls[i].b, org);
}

void GameScreen::drawIconButton(int slot, Texture2D tex, Rectangle r) {
    float dt = GetFrameTime();
    bool hovered = CheckCollisionPointRec(GetMousePosition(), r);
    if (hovered && !iconHov[slot]) iconWob[slot] = ICON_WOB_DUR;
    iconHov[slot] = hovered;

    float k = 1.0f - expf(-14.0f * dt);
    iconAnim[slot] += ((hovered ? 1.0f : 0.0f) - iconAnim[slot]) * k;
    if (iconWob[slot] > 0) iconWob[slot] -= dt;

    float wv    = (iconWob[slot] > 0) ? wobbleAt(1.0f - iconWob[slot] / ICON_WOB_DUR) : 0.0f;
    float tx    = ICON_WOB_DIST * wv;
    float angle = ICON_WOB_ROT * wv;
    unsigned char g = (unsigned char)(255.0f - 50.0f * iconAnim[slot]);
    Color tint = { g, g, g, 255 };

    float cx = r.x + r.width / 2.0f + tx;
    float cy = r.y + r.height / 2.0f;
    DrawTexturePro(tex, { 0, 0, (float)tex.width, (float)tex.height },
                   { cx, cy, r.width, r.height }, { r.width / 2, r.height / 2 }, angle, tint);
}

void GameScreen::drawConfetti() {
    for (int i = 0; i < confettiCount; i++) {
        const Confetti& p = confetti[i];
        if (p.life <= 0) continue;
        float a    = clamp01(p.life / CONFETTI_LIFE);
        float half = 6.0f;
        float rad  = p.rot * D2R;
        Vector2 d  = { cosf(rad), sinf(rad) };
        DrawLineEx({ p.pos.x - d.x * half, p.pos.y - d.y * half },
                   { p.pos.x + d.x * half, p.pos.y + d.y * half },
                   3.0f, Fade(p.col, a));
    }
}

#if defined(HEX_DEV)
// Dev helper (D key): exhaustive search for a winning merge sequence from the
// current position, then play it out. Same rules as update(); memoises dead
// states so the trap-heavy Advanced boards stay fast.
static std::string stateKey(const BoardState& b) {
    std::string k;
    k.reserve(b.cellCount * 2 + b.bombCount + 1);
    for (int i = 0; i < b.cellCount; i++) {
        int v = b.cells[i].value, e = 0;
        while (v > 1) { v >>= 1; e++; }
        k.push_back((char)('0' + e));
        k.push_back((char)('0' + b.cells[i].op));   // dailies: ops get consumed
    }
    for (int i = 0; i < b.bombCount; i++)
        k.push_back(b.bombs[i].armed ? 'A' : 'd');
    k.push_back((char)('a' + b.movesLeft));
    return k;
}

static bool searchWin(const BoardState& b, std::vector<std::pair<int,int>>& out,
                      std::unordered_set<std::string>& dead) {
    if (isWon(b)) return true;
    if (b.movesLeft <= 0) return false;
    std::string key = stateKey(b);
    if (dead.count(key)) return false;
    for (int i = 0; i < b.cellCount; i++) {
        if (b.cells[i].value == 0) continue;
        for (int j = 0; j < b.cellCount; j++) {
            if (i == j) continue;
            bool merge = (b.cells[j].value == b.cells[i].value);
            bool apply = (b.cells[j].value == 0 &&
                          opAllowed(b.cells[j].op, b.cells[i].value));
            if (!merge && !apply) continue;
            if (!boardAdjacent(b, b.cells[i].pos, b.cells[j].pos)) continue;
            BoardState nb = b;
            if (merge) {
                nb.cells[j].value *= 2;
                BombResult br = applyBombs(nb, b.cells[i].pos, b.cells[j].pos,
                                           nb.cells[j].value);
                if (br.explodedIdx >= 0) continue;   // losing move, never take it
            } else {
                nb.cells[j].value = applyOp(nb.cells[j].op, b.cells[i].value);
                nb.cells[j].op    = OP_NONE;
            }
            nb.cells[i].value = 0;
            nb.movesLeft--;
            out.push_back({ i, j });
            if (searchWin(nb, out, dead)) return true;
            out.pop_back();
        }
    }
    dead.insert(key);
    return false;
}

void GameScreen::debugSolve() {
    std::vector<std::pair<int,int>> seq;
    std::unordered_set<std::string> dead;
    if (!searchWin(board, seq, dead)) return;   // no win from here: undo first
    for (const auto& m : seq) {
        if (board.cells[m.second].value == 0 && board.cells[m.second].op != OP_NONE)
            doApply(m.first, m.second);
        else
            doMerge(m.first, m.second);
    }
}
#endif

void GameScreen::drawPraise(float alpha, float yUp) {
    float scale = easeOutBack(clamp01(winTimer / PRAISE_IN));
    if (scale < 0.02f || alpha <= 0.0f) return;

    float fs      = 64.0f * scale;
    float spacing = fs * 0.04f;
    float idle    = sinf((float)GetTime() * 4.0f) * 2.0f;
    float rot     = praiseRot + idle;

    Font f = titleFont();
    Vector2 m   = MeasureTextEx(f, praiseText, fs, spacing);
    Vector2 pos = { SCREEN_WIDTH / 2.0f, 185.0f - yUp };
    DrawTextPro(f, praiseText, pos, { m.x / 2, m.y / 2 }, rot, fs, spacing,
                Fade(HEXRED, alpha));
}
