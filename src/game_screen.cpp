#include "game_screen.h"
#include "config.h"
#include "ui.h"
#include "raylib.h"

static Rectangle pauseButton()  { return { SCREEN_WIDTH - 130.0f, 20, 110, 44 }; }

static Rectangle modalPanel()   { return { SCREEN_WIDTH/2.0f - 180, SCREEN_HEIGHT/2.0f - 140, 360, 280 }; }
static Rectangle resumeButton() { return { SCREEN_WIDTH/2.0f - 130, SCREEN_HEIGHT/2.0f - 20, 260, 54 }; }
static Rectangle menuButton()   { return { SCREEN_WIDTH/2.0f - 130, SCREEN_HEIGHT/2.0f + 46, 260, 54 }; }

ScreenType GameScreen::update() {
    if (!started) {
        level = gStartLevel;
        started = true;
    }

    if (paused) {
        if (buttonClicked(resumeButton()) || IsKeyPressed(KEY_ESCAPE)) {
            paused = false;
        }
        if (buttonClicked(menuButton())) {
            return ScreenType::TITLE;
        }
        return ScreenType::NONE;
    }

    if (IsKeyPressed(KEY_ESCAPE) || buttonClicked(pauseButton())) {
        paused = true;
    }

    return ScreenType::NONE;
}

void GameScreen::draw() {
    ClearBackground(PAPER);

    DrawText(TextFormat("Level %d", level + 1), 20, 24, 32, INK);

    const char *msg = "hex board coming soon";
    int msgW = MeasureText(msg, 28);
    DrawText(msg, SCREEN_WIDTH/2 - msgW/2, SCREEN_HEIGHT/2 - 14, 28, HEXRED);

    drawButton(pauseButton(), "Pause");

    if (paused) {
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.5f));

        Rectangle panel = modalPanel();
        DrawRectangleRec(panel, PAPER);
        DrawRectangleLinesEx(panel, 2, INK);

        const char *t = "Paused";
        int tW = MeasureText(t, 40);
        DrawText(t, SCREEN_WIDTH/2 - tW/2, (int)panel.y + 28, 40, INK);

        drawButton(resumeButton(), "Resume");
        drawButton(menuButton(),   "Main Menu");
    }
}
