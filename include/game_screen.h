#pragma once

#include "screen.h"
#include "board.h"
#include "ui.h"
#include "raylib.h"

class GameScreen : public Screen {
public:
    GameScreen();
    ScreenType update() override;
    void       draw() override;

private:
    BoardState board;
    int        currentLevel = 0;
    int        selectedIdx  = -1;
    Vector2    origin       = { 0, 0 };
    bool       started      = false;
    bool       won          = false;
    bool       lost         = false;
    bool       paused       = false;

    Vector2 pressStart = { 0, 0 };
    int     pressIdx   = -1;
    bool    dragging   = false;

    Menu  pauseMenu;
    Menu  winMenu;
    Menu  lostMenu;
    float pauseAnim = 0.0f;
    float winAnim   = 0.0f;
    float lostAnim  = 0.0f;

    float pauseBtnAnim = 0.0f;
    float pauseBtnWob  = 0.0f;
    bool  pauseBtnHov  = false;

    void loadLevel(int idx);
    void doMerge(int fromIdx, int toIdx);
    void openPause();
    void openWin();
    void openLost();
    void drawPauseButton();
};
