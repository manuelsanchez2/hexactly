#include "title_screen.h"
#include "config.h"
#include "ui.h"
#include "raylib.h"

static Rectangle playButton()    { return { SCREEN_WIDTH/2.0f - 130, 300, 260, 58 }; }
static Rectangle levelsButton()  { return { SCREEN_WIDTH/2.0f - 130, 372, 260, 58 }; }
static Rectangle optionsButton() { return { SCREEN_WIDTH/2.0f - 130, 444, 260, 58 }; }
static Rectangle quitButton()    { return { SCREEN_WIDTH/2.0f - 130, 516, 260, 58 }; }

ScreenType TitleScreen::update() {
    if (buttonClicked(playButton()))    return ScreenType::GAME;
    if (buttonClicked(levelsButton()))  return ScreenType::LEVELSELECT;
    if (buttonClicked(optionsButton())) return ScreenType::OPTIONS;
    if (buttonClicked(quitButton()))    return ScreenType::QUIT;
    return ScreenType::NONE;
}

void TitleScreen::draw() {
    ClearBackground(PAPER);

    const char *title = "HEXACTLY";
    int titleSize = 88;
    int titleW = MeasureText(title, titleSize);
    DrawText(title, SCREEN_WIDTH/2 - titleW/2, 150, titleSize, INK);

    // A small red underline flourish for the doodle feel.
    DrawRectangle(SCREEN_WIDTH/2 - titleW/2, 150 + titleSize + 6, titleW, 6, HEXRED);

    drawButton(playButton(),    "Play");
    drawButton(levelsButton(),  "Level Select");
    drawButton(optionsButton(), "Options");
    drawButton(quitButton(),    "Quit");
}
