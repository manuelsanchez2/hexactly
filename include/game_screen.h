#pragma once

#include "screen.h"
#include "board.h"
#include "ui.h"
#include "layout.h"
#include "editor.h"
#include "raylib.h"

struct LevelDef;

const int MAX_UNDO = 64;

enum GamePhase { PH_PLAYING, PH_LOST, PH_CELEBRATE, PH_SWAP, PH_DONE };

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
    BoardState undoStack[MAX_UNDO];
    int        undoCount    = 0;
    int        selectedIdx  = -1;
    int        currentLevel = 0;
    Vector2    origin       = { 0, 0 };
    GamePhase  phase        = PH_PLAYING;

    int   popIdx   = -1;
    float popTimer = 0.0f;

    bool    slideActive  = false;
    Vector2 slideFrom     = { 0, 0 };
    Vector2 slideTo       = { 0, 0 };
    int     slideValue    = 0;
    int     slideLandIdx  = -1;
    float   slideT        = 0.0f;

    float   starTimer  = 0.0f;
    Vector2 starCenter = { 0, 0 };

    float    winTimer   = 0.0f;
    float    haloTimer  = 0.0f;
    Vector2  winCenter  = { 0, 0 };
    const char* praiseText = "";
    float    praiseRot  = 0.0f;

    float      swapT        = 0.0f;
    bool       finishing    = false;
    bool       hasOutgoing  = false;
    BoardState outgoing;
    Vector2    outOrigin    = { 0, 0 };
    Vector2    targetOrigin = { 0, 0 };

    Confetti  confetti[24];
    int       confettiCount = 0;

    Layout layout;
    Editor editor;
    long   mtime = 0;

    float iconAnim[3] = { 0, 0, 0 };
    float iconWob[3]  = { 0, 0, 0 };
    bool  iconHov[3]  = { false, false, false };
    void  drawIconButton(int slot, Texture2D tex, Rectangle r);

    bool  paused    = false;
    float pauseAnim = 0.0f;
    Menu  pauseMenu;

    bool  rulesActive  = false;
    bool  rulesClosing = false;
    float rulesAnim    = 0.0f;
    Menu  rulesMenu;

    bool  pendingCongrats = false;   // beat the last Beginner level this run
    bool  congratsActive  = false;
    float congratsAnim    = 0.0f;
    Menu  congratsMenu;

    std::string              ovTitle;   // current overlay heading
    std::vector<std::string> ovLines;   // current overlay body lines

    int     pressIdx   = -1;
    bool    dragging   = false;
    Vector2 pressStart = { 0, 0 };

    float cellWob[MAX_CELLS] = { 0 };
    int   hoverPrev = -1;

    int   moveLimit = 0;     // the loaded level's full move budget
    float movesWob  = 0.0f;  // wobble timer for the moves-left number

    void openRules();
    void showTip(bool portal);
    void showFinal();
    void presentOverlay();
    void closeRules();

    bool endGame = false;   // final "thanks for playing" modal is up

    bool daily = false;   // this run is a daily challenge, not a numbered level

    void applyLevel(const LevelDef& L);
    void loadLevel(int idx);
    void loadDaily();
    void reloadCurrent();
    void pushUndo();
    void doUndo();
    void doMerge(int fromIdx, int toIdx);
    void doApply(int fromIdx, int toIdx);
    void checkEnd();
#if defined(HEX_DEV)
    void debugSolve();
#endif

    void beginSwap();
    void spawnConfetti(Vector2 at);
    void drawStaticBoard(const BoardState& b, Vector2 org);
    void drawConfetti();
    void drawPraise(float alpha, float yUp);
};
