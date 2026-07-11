#include "levels.h"
#include "board.h"     // OP_* operator constants and applyOp()
#include <ctime>

const LevelDef LEVELS[] = {
    { "first", 1, 2,
      { {0,0,2,0}, {1,0,2,F_GOAL} }, 0, {} },

    { "aim", 2, 3,
      { {0,0,2,0}, {1,0,2,0}, {2,-1,4,F_GOAL} }, 0, {} },

    { "corner", 3, 4,
      { {0,0,2,0}, {1,0,2,0}, {0,1,2,0}, {-1,1,2,F_GOAL} }, 0, {} },

    { "fork", 3, 4,
      { {0,0,2,F_GOAL}, {1,0,2,0}, {1,-1,2,0}, {0,1,2,0} }, 0, {} },

    { "chain", 3, 4,
      { {0,0,2,0}, {1,0,2,0}, {2,-1,4,0}, {3,-1,8,F_GOAL} }, 0, {} },

    { "escalate", 4, 5,
      { {0,0,2,0}, {1,0,2,0}, {2,-1,4,0}, {3,-1,8,0}, {4,-2,16,F_GOAL} }, 0, {} },

    { "octet", 7, 8,
      { {0,0,2,0}, {1,0,2,0}, {2,0,2,0}, {3,0,2,0},
        {0,1,2,0}, {1,1,2,F_GOAL}, {2,1,2,0}, {3,1,2,0} }, 0, {} },

    { "gate", 3, 4,
      { {0,0,2,0}, {1,0,2,0}, {1,-1,2,0}, {2,-1,2,F_GOAL} },
      1, { {1,0, 1,-1} } },

    { "sidestep", 3, 4,
      { {0,0,2,0}, {1,0,2,0}, {1,-1,4,0}, {2,-1,8,F_GOAL} },
      1, { {1,0, 1,-1} } },

    { "walled off", 4, 6,
      { {0,0,2,F_GOAL,32}, {1,0,2,0,0}, {0,-1,4,0,0}, {-1,0,8,0,0},
        {0,1,16,0,0}, {-1,1,16,0,0} },
      1, { {0,0, -1,1} } },

    { "crossroads", 7, 8,
      { {0,0,2,0}, {1,0,2,0}, {2,0,2,F_GOAL}, {3,0,2,0},
        {0,1,2,0}, {1,1,2,0}, {2,1,2,0}, {3,1,2,0} },
      1, { {1,0, 1,1} } },

    { "labyrinth", 7, 8,
      { {1,-1,2,0}, {0,0,2,0}, {1,0,2,0}, {2,0,2,0},
        {0,1,2,0}, {1,1,2,F_GOAL}, {2,1,2,0}, {1,2,2,0} },
      1, { {1,0, 1,1} } },

    { "grand finale", 5, 6,
      { {0,0,2,0}, {1,0,2,0}, {1,-1,4,0}, {2,-1,8,0},
        {2,-2,16,0}, {3,-2,32,F_GOAL} },
      1, { {1,0, 1,-1} } },

    { "wormhole", 1, 2,
      { {0,0,2,0}, {2,0,2,F_GOAL} },
      0, {},
      1, { {0,0, 2,0} } },

    { "shortcut", 3, 4,
      { {0,0,2,0}, {1,0,2,F_GOAL}, {3,0,2,0}, {4,0,2,0} },
      0, {},
      1, { {1,0, 3,0} } },

    { "reach", 3, 4,
      { {0,0,2,0}, {1,0,2,0}, {1,-1,4,0}, {5,0,8,F_GOAL} },
      0, {},
      1, { {1,-1, 5,0} } },

    { "portal feed", 4, 7,
      { {0,0,2,F_GOAL,32}, {1,0,2,0,0}, {0,-1,4,0,0}, {-1,0,8,0,0},
        {4,0,16,0,0}, {0,1,2,0,0}, {-1,1,2,0,0} },
      0, {},
      1, { {0,0, 4,0} } },

    { "double portal", 4, 6,
      { {0,0,2,F_GOAL,32}, {1,0,2,0,0}, {0,-1,4,0,0},
        {3,0,8,0,0}, {0,3,16,0,0}, {-1,0,2,0,0} },
      0, {},
      2, { {0,0, 3,0}, {0,0, 0,3} } },

    { "impostor", 4, 7,
      { {0,0,2,F_GOAL,32}, {1,0,2,0,0}, {0,-1,4,0,0}, {-1,0,8,0,0},
        {3,0,16,0,0}, {0,3,32,0,0}, {0,1,2,0,0} },
      0, {},
      2, { {0,0, 3,0}, {0,0, 0,3} } },

    { "gauntlet", 6, 8,
      { {0,0,2,F_GOAL,64}, {1,0,2,0,0}, {0,-1,4,0,0}, {-1,0,8,0,0},
        {0,1,8,0,0}, {-1,1,8,0,0}, {0,4,32,0,0}, {1,-1,2,0,0} },
      0, {},
      1, { {0,0, 0,4} } },
};

const int LEVEL_COUNT = sizeof(LEVELS) / sizeof(LEVELS[0]);

// ---- Daily challenge -------------------------------------------------------

long dailyEpochDay() {
    time_t now = time(nullptr);
    if (now < 0) now = 0;
    return (long)(now / 86400);        // 86400 seconds per day (UTC)
}

int dailyIndex() {
    long d = dailyEpochDay() % DAILY_COUNT;
    if (d < 0) d += DAILY_COUNT;
    return (int)d;
}

// Axial hex directions (must match hexToPixel / boardAdjacent in hex.cpp).
static const int DQ[6] = {  1,  1,  0, -1, -1,  0 };
static const int DR[6] = {  0, -1, -1,  0,  1,  1 };

// Build a guaranteed-solvable operator puzzle for a given index. A single value
// tile (2) sits at the head of a self-avoiding hex path; every other path cell
// is an operator tile (x2 / +2 / -2). Dragging the number along the path applies
// each operator in turn, and the last cell is the goal where the running value
// must land exactly on the target. Because the operators are chosen forward from
// 2, the target is whatever that chain produces, so it is always winnable in
// exactly (steps) moves. Even-only: -2 is only ever placed where the value stays
// >= 2, and every operator keeps an even value even. A couple of dead-end decoy
// operators branch off the path to tempt you into missing the exact target.
LevelDef makeDaily(int idx) {
    LevelDef L{};
    L.name = "daily";

    unsigned seed = (unsigned)idx * 2654435761u + 12345u;
    auto rnd = [&](unsigned n) {
        seed = seed * 1664525u + 1013904223u;
        return (n == 0) ? 0u : (seed >> 16) % n;
    };

    // Operator steps == moves. Short so the path fits the 720x720 board.
    int steps = 3 + (int)rnd(4);            // 3..6

    // Self-avoiding hex path: cell 0 is the value tile, cells 1..steps operators.
    int pq[8], pr[8];
    pq[0] = 0; pr[0] = 0;
    int dir   = idx % 6;
    int count = 1;
    while (count < steps + 1) {
        int d = dir;
        if (rnd(3) == 0) d = (dir + (rnd(2) ? 1 : 5)) % 6;   // occasional +/-60 turn

        bool placed = false;
        for (int t = 0; t < 6; t++) {
            int cand = (d + t) % 6;
            int nq = pq[count - 1] + DQ[cand];
            int nr = pr[count - 1] + DR[cand];
            bool clash = false;
            for (int k = 0; k < count; k++)
                if (pq[k] == nq && pr[k] == nr) { clash = true; break; }
            if (!clash) { pq[count] = nq; pr[count] = nr; dir = cand; placed = true; break; }
        }
        if (!placed) break;            // fully boxed in (won't happen on open plane)
        count++;
    }
    steps = count - 1;                 // in case the walk boxed in early

    // Head of the path: the only movable number.
    L.cells[0].q = pq[0]; L.cells[0].r = pr[0];
    L.cells[0].value = 2; L.cells[0].flags = 0; L.cells[0].goalValue = 0; L.cells[0].op = OP_NONE;

    // Operator tiles, chosen forward so the chain is always solvable. Values are
    // capped at 64 so tiles never grow past what the board comfortably shows.
    int cur = 2;
    for (int i = 1; i <= steps; i++) {
        int choices[3]; int nc = 0;
        if (cur * 2 <= 64) choices[nc++] = OP_MUL2;
        if (cur + 2 <= 64) choices[nc++] = OP_ADD2;
        if (cur - 2 >= 2)  choices[nc++] = OP_SUB2;   // never drops the value to 0
        int op = choices[nc == 0 ? 0 : (int)rnd(nc)]; // nc > 0 for any cur in [2,64]
        cur = applyOp(op, cur);

        L.cells[i].q = pq[i]; L.cells[i].r = pr[i];
        L.cells[i].value = 0; L.cells[i].flags = 0; L.cells[i].goalValue = 0; L.cells[i].op = op;
    }

    // Last operator tile is the goal; the chain lands here on the exact target.
    L.cells[steps].flags     = F_GOAL;
    L.cells[steps].goalValue = cur;
    L.cellCount              = steps + 1;

    // Decoy operators: optional dead-end branches off the solving path. They can
    // never block the guaranteed solution, and reaching the goal with the wrong
    // value still loses (isWon checks the target exactly).
    static const int OPS[3] = { OP_ADD2, OP_SUB2, OP_MUL2 };
    int decoys = 1 + (int)rnd(2);           // 1..2
    for (int d = 0; d < decoys && L.cellCount < LVL_MAX_CELLS; d++) {
        int anchor = (int)rnd(steps);       // a path cell the value passes through
        int start  = (int)rnd(6);
        for (int t = 0; t < 6; t++) {
            int cand = (start + t) % 6;
            int nq = pq[anchor] + DQ[cand];
            int nr = pr[anchor] + DR[cand];
            bool clash = false;
            for (int k = 0; k < L.cellCount; k++)
                if (L.cells[k].q == nq && L.cells[k].r == nr) { clash = true; break; }
            if (!clash) {
                int di = L.cellCount++;
                L.cells[di].q = nq; L.cells[di].r = nr;
                L.cells[di].value = 0; L.cells[di].flags = 0;
                L.cells[di].goalValue = 0; L.cells[di].op = OPS[(int)rnd(3)];
                break;
            }
        }
    }

    L.moveLimit   = steps;
    L.wallCount   = 0;
    L.portalCount = 0;
    return L;
}
