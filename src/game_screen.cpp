#include "game_screen.h"
#include "screen.h"
#include "config.h"
#include "gamefont.h"
#include "levels.h"
#include "ui.h"
#include "raylib.h"

static const float BOARD_DROP = 110.0f;
static const float TILE_SCALE = 96.0f / 46.0f;

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

static void dimAndPanel(const char *title) {
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.5f));
    Rectangle p = modalPanel();
    DrawRectangleRec(p, PAPER);
    DrawRectangleLinesEx(p, 2, INK);
    titleDrawCentered(title, p.y + 26, 40, INK);
}

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
    won  = false;
    lost = false;
}

void GameScreen::doMerge(int fromIdx, int toIdx) {
    board.cells[toIdx].value  *= 2;
    board.cells[fromIdx].value = 0;
    board.movesLeft--;
    selectedIdx = -1;

    if (isWon(board)) won = true;
    else if (board.movesLeft <= 0 || !anyLegalMove(board)) lost = true;
}

ScreenType GameScreen::update() {
    if (!started) {
        currentLevel = gStartLevel;
        if (currentLevel < 0) currentLevel = 0;
        if (currentLevel >= LEVEL_COUNT) currentLevel = LEVEL_COUNT - 1;
        loadLevel(currentLevel);
        started = true;
    }

    if (paused) {
        if (buttonClicked(mBtn(0)) || IsKeyPressed(KEY_ESCAPE)) paused = false;
        if (buttonClicked(mBtn(1))) { loadLevel(currentLevel); paused = false; }
        if (buttonClicked(mBtn(2))) return ScreenType::TITLE;
        return ScreenType::NONE;
    }

    if (won) {
        bool last = (currentLevel + 1 >= LEVEL_COUNT);
        if (buttonClicked(mBtn(0))) {
            if (last) return ScreenType::LEVELSELECT;
            currentLevel++;
            loadLevel(currentLevel);
        }
        if (buttonClicked(mBtn(1))) return ScreenType::LEVELSELECT;
        return ScreenType::NONE;
    }

    if (lost) {
        if (buttonClicked(mBtn(0))) loadLevel(currentLevel);
        if (buttonClicked(mBtn(1))) return ScreenType::TITLE;
        return ScreenType::NONE;
    }

    if (IsKeyPressed(KEY_ESCAPE) || buttonClicked(pauseBtn())) {
        paused = true;
        return ScreenType::NONE;
    }

    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        Hex h = pixelToHex(GetMousePosition(), origin);
        int idx = cellIndexAt(board, h);
        if (idx >= 0 && board.cells[idx].value > 0 &&
            !board.cells[idx].isStone && !board.cells[idx].isCursed) {
            if (selectedIdx < 0)         selectedIdx = idx;
            else if (idx == selectedIdx) selectedIdx = -1;
            else if (board.cells[selectedIdx].value == board.cells[idx].value &&
                     boardAdjacent(board, board.cells[selectedIdx].pos, board.cells[idx].pos))
                doMerge(selectedIdx, idx);
            else                         selectedIdx = idx;
        } else {
            selectedIdx = -1;
        }
    }

    return ScreenType::NONE;
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

    drawButton(pauseBtn(), "Pause");

    if (paused) {
        dimAndPanel("Paused");
        drawButton(mBtn(0), "Resume");
        drawButton(mBtn(1), "Restart");
        drawButton(mBtn(2), "Main Menu");
    } else if (won) {
        bool last = (currentLevel + 1 >= LEVEL_COUNT);
        dimAndPanel(last ? "You finished!" : "Solved!");
        drawButton(mBtn(0), last ? "Back to Levels" : "Next Level");
        drawButton(mBtn(1), "Levels");
    } else if (lost) {
        dimAndPanel("No moves left");
        drawButton(mBtn(0), "Restart");
        drawButton(mBtn(1), "Main Menu");
    }
}
