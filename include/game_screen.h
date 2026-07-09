#pragma once

#include "screen.h"
#include "board.h"
#include "raylib.h"

class GameScreen : public Screen {
public:
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

    void loadLevel(int idx);
    void doMerge(int fromIdx, int toIdx);
};
