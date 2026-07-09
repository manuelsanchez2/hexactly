#pragma once

#include "screen.h"
#include "board.h"
#include "ui.h"
#include "raylib.h"

enum GamePhase { PH_PLAYING, PH_CELEBRATE, PH_SWAP };

struct Confetti {
    Vector2 pos, vel;
    float   rot, rotVel;
    Color   col;
    float   life;
};

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
    bool       lost         = false;
    bool       paused       = false;
    GamePhase  phase        = PH_PLAYING;

    Vector2 pressStart = { 0, 0 };
    int     pressIdx   = -1;
    bool    dragging   = false;

    Menu  pauseMenu;
    Menu  lostMenu;
    float pauseAnim = 0.0f;
    float lostAnim  = 0.0f;

    float pauseBtnAnim = 0.0f;
    float pauseBtnWob  = 0.0f;
    bool  pauseBtnHov  = false;

    float       winTimer   = 0.0f;
    float       haloTimer  = 0.0f;
    Vector2     winCenter  = { 0, 0 };
    const char* praiseText = "";
    float       praiseRot  = 0.0f;

    float      swapT        = 0.0f;
    bool       finishing    = false;
    bool       hasOutgoing  = false;
    BoardState outgoing;
    Vector2    outOrigin    = { 0, 0 };
    Vector2    targetOrigin = { 0, 0 };

    Confetti confetti[24];
    int      confettiCount = 0;

    void loadLevel(int idx);
    void doMerge(int fromIdx, int toIdx);
    void checkEnd();
    void openPause();
    void openLost();
    void drawPauseButton();

    void beginSwap();
    void spawnConfetti(Vector2 at);
    void drawStaticBoard(const BoardState& b, Vector2 org);
    void drawConfetti();
    void drawPraise(float alpha, float yUp);
};
