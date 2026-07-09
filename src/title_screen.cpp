#include "title_screen.h"
#include "config.h"
#include "gamefont.h"
#include "ui.h"
#include "raylib.h"

static Rectangle playButton()    { return { 240, 433, 240, 58 }; }
static Rectangle optionsButton() { return { 240, 509, 240, 58 }; }
static Rectangle quitButton()    { return { 240, 583, 240, 58 }; }

static void drawTitleButton(Rectangle r, const char *text) {
    bool hover = CheckCollisionPointRec(GetMousePosition(), r);

    Texture2D tex = primaryButtonTexture();
    Color tint = hover ? (Color){ 235, 235, 235, 255 } : WHITE;
    DrawTexturePro(tex, { 0, 0, (float)tex.width, (float)tex.height }, r,
                   { 0, 0 }, 0.0f, tint);

    float size = 30;
    Vector2 m = titleMeasure(text, size);
    titleDraw(text, r.x + r.width / 2 - m.x / 2, r.y + r.height / 2 - m.y / 2,
              size, hover ? HEXRED : INK);
}

ScreenType TitleScreen::update() {
    if (buttonClicked(playButton()))    return ScreenType::LEVELSELECT;
    if (buttonClicked(optionsButton())) return ScreenType::OPTIONS;
    if (buttonClicked(quitButton()))    return ScreenType::QUIT;
    return ScreenType::NONE;
}

void TitleScreen::draw() {
    ClearBackground(PAPER);

    Texture2D bg = bgTitleTexture();
    DrawTexturePro(bg, { 0, 0, (float)bg.width, (float)bg.height },
                   { 0, 0, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT },
                   { 0, 0 }, 0.0f, WHITE);

    titleDrawCentered("HEXACTLY", 210, 84, INK);
    titleDrawCentered("merge equal neighbours into one tile", 345, 22, INK);

    drawTitleButton(playButton(),    "Play");
    drawTitleButton(optionsButton(), "Options");
    drawTitleButton(quitButton(),    "Quit");

    titleDraw("v.0.0.1", 622, 685, 16, GRAY);
}
