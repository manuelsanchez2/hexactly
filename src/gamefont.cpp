#include "gamefont.h"
#include "config.h"

static Font      gTitle;
static Texture2D gTick;
static Texture2D gPrimaryBtn;
static Texture2D gSecondaryBtn;
static Texture2D gSquareBtn;
static Texture2D gLevelSquare;
static Texture2D gHexTile;
static Texture2D gFlag;
static Texture2D gPortalA;
static Texture2D gPortalB;
static Texture2D gBgTitle;
static Texture2D gBtnInfo;
static Texture2D gBtnPause;
static Texture2D gBtnUndo;
static bool      loaded = false;

static Texture2D loadTex(const char *path) {
    Texture2D t = LoadTexture(path);
    SetTextureFilter(t, TEXTURE_FILTER_BILINEAR);
    return t;
}

void loadFonts() {
    gTitle = LoadFontEx("resources/Oliver-Regular.ttf", 96, 0, 0);
    SetTextureFilter(gTitle.texture, TEXTURE_FILTER_BILINEAR);

    gTick         = loadTex("resources/tick.png");
    gPrimaryBtn   = loadTex("resources/primary-button.png");
    gSecondaryBtn = loadTex("resources/secondary-button.png");
    gSquareBtn    = loadTex("resources/small-square-buttons.png");
    gLevelSquare  = loadTex("resources/level-select-square.png");
    gHexTile      = loadTex("resources/hex-tile-new.png");
    gFlag         = loadTex("resources/flag-goal.png");
    gPortalA      = loadTex("resources/portal-a.png");
    gPortalB      = loadTex("resources/portal-b.png");
    gBgTitle      = loadTex("resources/bg-title.png");
    gBtnInfo      = loadTex("resources/btn-info.png");
    gBtnPause     = loadTex("resources/btn-pause.png");
    gBtnUndo      = loadTex("resources/btn-undo.png");

    loaded = true;
}

void unloadFonts() {
    if (loaded) {
        UnloadFont(gTitle);
        UnloadTexture(gTick);
        UnloadTexture(gPrimaryBtn);
        UnloadTexture(gSecondaryBtn);
        UnloadTexture(gSquareBtn);
        UnloadTexture(gLevelSquare);
        UnloadTexture(gHexTile);
        UnloadTexture(gFlag);
        UnloadTexture(gPortalA);
        UnloadTexture(gPortalB);
        UnloadTexture(gBgTitle);
        UnloadTexture(gBtnInfo);
        UnloadTexture(gBtnPause);
        UnloadTexture(gBtnUndo);
    }
    loaded = false;
}

Font&     titleFont()              { return gTitle; }
Texture2D tickTexture()            { return gTick; }
Texture2D primaryButtonTexture()   { return gPrimaryBtn; }
Texture2D secondaryButtonTexture() { return gSecondaryBtn; }
Texture2D squareButtonTexture()    { return gSquareBtn; }
Texture2D levelSquareTexture()     { return gLevelSquare; }
Texture2D hexTileTexture()         { return gHexTile; }
Texture2D flagGoalTexture()        { return gFlag; }
Texture2D portalATexture()         { return gPortalA; }
Texture2D portalBTexture()         { return gPortalB; }
Texture2D bgTitleTexture()         { return gBgTitle; }
Texture2D btnInfoTexture()         { return gBtnInfo; }
Texture2D btnPauseTexture()        { return gBtnPause; }
Texture2D btnUndoTexture()         { return gBtnUndo; }

static float spacingFor(float size) { return size * 0.04f; }

Vector2 titleMeasure(const char *text, float size) {
    return MeasureTextEx(gTitle, text, size, spacingFor(size));
}

void titleDraw(const char *text, float x, float y, float size, Color color) {
    DrawTextEx(gTitle, text, (Vector2){ x, y }, size, spacingFor(size), color);
}

void titleDrawCentered(const char *text, float y, float size, Color color) {
    float w = titleMeasure(text, size).x;
    titleDraw(text, (SCREEN_WIDTH - w) / 2.0f, y, size, color);
}

void titleDrawCenteredAt(const char *text, float cx, float cy, float size, Color color) {
    Vector2 m = titleMeasure(text, size);
    titleDraw(text, cx - m.x / 2.0f, cy - m.y / 2.0f, size, color);
}

void titleDrawCenteredAtRot(const char *text, float cx, float cy, float size, float rotDeg, Color color) {
    Vector2 m = titleMeasure(text, size);
    DrawTextPro(gTitle, text, (Vector2){ cx, cy }, (Vector2){ m.x / 2.0f, m.y / 2.0f },
                rotDeg, size, spacingFor(size), color);
}
