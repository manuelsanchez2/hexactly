#include "options_screen.h"
#include "config.h"
#include "ui.h"
#include "raylib.h"

static Rectangle soundButton() { return { SCREEN_WIDTH/2.0f - 130, 280, 260, 58 }; }
static Rectangle volMinus()    { return { SCREEN_WIDTH/2.0f - 130, 360,  58, 58 }; }
static Rectangle volPlus()     { return { SCREEN_WIDTH/2.0f +  72, 360,  58, 58 }; }
static Rectangle backButton()  { return { SCREEN_WIDTH/2.0f - 130, 470, 260, 58 }; }

ScreenType OptionsScreen::update() {
    if (buttonClicked(soundButton())) soundOn = !soundOn;
    if (buttonClicked(volMinus()))    volume = (volume > 0)   ? volume - 10 : 0;
    if (buttonClicked(volPlus()))     volume = (volume < 100) ? volume + 10 : 100;

    if (buttonClicked(backButton()))  return ScreenType::TITLE;

    return ScreenType::NONE;
}

void OptionsScreen::draw() {
    ClearBackground(PAPER);

    const char *title = "Options";
    int titleSize = 56;
    int titleW = MeasureText(title, titleSize);
    DrawText(title, SCREEN_WIDTH/2 - titleW/2, 150, titleSize, INK);

    drawButton(soundButton(), soundOn ? "Sound: ON" : "Sound: OFF");

    drawButton(volMinus(), "-");
    drawButton(volPlus(),  "+");
    const char *vol = TextFormat("Volume: %d", volume);
    int volW = MeasureText(vol, 24);
    DrawText(vol, SCREEN_WIDTH/2 - volW/2, 376, 24, INK);

    drawButton(backButton(), "Back");
}
