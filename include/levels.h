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
    struct { int q, r, value, flags, goalValue, op; } cells[LVL_MAX_CELLS];
    int         wallCount;
    struct { int q1, r1, q2, r2; } walls[LVL_MAX_WALLS];
    int         portalCount;
    struct { int q1, r1, q2, r2; } portals[LVL_MAX_PORTALS];
};

extern const LevelDef LEVELS[];
extern const int      LEVEL_COUNT;

// ---- Daily challenge -------------------------------------------------------
// One of 31 puzzles, rotating by calendar day (epoch-day % 31) so every player
// worldwide gets the same one, with no server. Puzzles are generated as
// guaranteed-solvable ascending chains, so any day is always winnable.
const int DAILY_COUNT = 31;

long     dailyEpochDay();          // whole days since the Unix epoch (UTC)
int      dailyIndex();             // today's challenge, 0..DAILY_COUNT-1
LevelDef makeDaily(int idx);       // deterministic puzzle for a given index
