#pragma once

const int LVL_MAX_CELLS   = 32;
const int LVL_MAX_WALLS   = 16;
const int LVL_MAX_PORTALS = 8;

const int F_GOAL   = 1; // the winning tile must end here, with a flag icon
const int F_STONE  = 2; // an inert rock
const int F_CURSED = 4; // locked until a merge happens next to it, will come later

struct LevelDef {
    const char* name;
    int         moveLimit;
    int         cellCount;
    struct { int q, r, value, flags, goalValue; } cells[LVL_MAX_CELLS];
    int         wallCount;
    struct { int q1, r1, q2, r2; } walls[LVL_MAX_WALLS];
    int         portalCount;
    struct { int q1, r1, q2, r2; } portals[LVL_MAX_PORTALS];
};

extern const LevelDef LEVELS[];
extern const int      LEVEL_COUNT;
