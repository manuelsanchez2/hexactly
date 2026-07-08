#include "levelselect_screen.h"
#include "config.h"
#include "ui.h"
#include "raylib.h"

static const int cols = 3;
static const float cell = 140.0f; 
static const float gap  = 32.0f;  

static Rectangle levelButton(int i) {
    int rows = 3;
    float gridW = cols*cell + (cols - 1)*gap;
    float gridH = rows*cell + (rows - 1)*gap;
    float startX = SCREEN_WIDTH/2.0f  - gridW/2.0f;
    float startY = SCREEN_HEIGHT/2.0f - gridH/2.0f + 20;
    int r = i / cols;
    int c = i % cols;
    return { startX + c*(cell + gap), startY + r*(cell + gap), cell, cell };
}

static Rectangle backButton() { return { SCREEN_WIDTH/2.0f - 130, SCREEN_HEIGHT - 90.0f, 260, 58 }; }

ScreenType LevelSelectScreen::update() {
    for (int i = 0; i < levelCount; i++) {
        if (buttonClicked(levelButton(i))) {
            gStartLevel = i;              
            return ScreenType::GAME;      
        }
    }
    if (buttonClicked(backButton())) return ScreenType::TITLE;
    return ScreenType::NONE;
}

void LevelSelectScreen::draw() {
    ClearBackground(PAPER);

    const char *title = "Select Level";
    int titleSize = 48;
    int titleW = MeasureText(title, titleSize);
    DrawText(title, SCREEN_WIDTH/2 - titleW/2, 70, titleSize, INK);

    for (int i = 0; i < levelCount; i++) {
        drawButton(levelButton(i), TextFormat("%d", i + 1));
    }

    drawButton(backButton(), "Back");
}
