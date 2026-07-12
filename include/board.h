#pragma once

#include "hex.h"

const int MAX_CELLS   = 32;
const int MAX_WALLS   = 16;
const int MAX_PORTALS = 8;
const int MAX_BOMBS   = 4;

// Operator tiles (daily challenges only). A value tile dragged onto an adjacent
// operator tile is transformed by it; the operator is consumed. Normal levels
// never contain these, so the mechanic stays scoped to dailies by data alone.
enum { OP_NONE = 0, OP_ADD2, OP_SUB2, OP_MUL2 };

struct Cell {
    Hex  pos;
    int  value;
    bool exists;
    bool isGoal;
    int  goalValue;
    int  op;          // OP_NONE for a normal tile; otherwise an operator tile
};

inline int applyOp(int op, int v) {
    switch (op) {
        case OP_ADD2: return v + 2;
        case OP_SUB2: return v - 2;
        case OP_MUL2: return v * 2;
        default:      return v;
    }
}

// Even-only rules: -2 on a 2 would reach 0, so it is disallowed. Every other
// operator keeps an even value even, so no odd-number cases can ever arise.
inline bool opAllowed(int op, int v) {
    if (op == OP_NONE) return false;
    if (op == OP_SUB2) return v > 2;
    return true;
}

struct Wall { Hex a, b; };

struct Portal { Hex a, b; };

// Bomb levels (21-25). A bomb occupies its own cell-less hex. While armed, any
// merge whose source or target touches it explodes it (level lost) - unless
// the merge lands exactly `value` on a touching cell, which defuses it.
// "Touching" is boardAdjacent: walls shield a neighbour, portals reach one.
struct Bomb {
    Hex  pos;
    int  value;
    bool armed;
};

struct BoardState {
    Cell   cells[MAX_CELLS];
    int    cellCount;
    int    movesLeft;
    Wall   walls[MAX_WALLS];
    int    wallCount;
    Portal portals[MAX_PORTALS];
    int    portalCount;
    Bomb   bombs[MAX_BOMBS];
    int    bombCount;
};

// Outcome of a merge (from -> to, producing result) on the board's bombs.
// Matching bombs are disarmed in place; explodedIdx / defusedIdx report the
// bomb hit (or -1). An explosion means the level is lost.
struct BombResult { int explodedIdx; int defusedIdx; };

int  cellIndexAt(const BoardState& b, Hex h);
bool boardAdjacent(const BoardState& b, Hex a, Hex c);
int  portalBetween(const BoardState& b, Hex a, Hex c);
int  tilesRemaining(const BoardState& b);
bool anyLegalMove(const BoardState& b);
int  goalIndex(const BoardState& b);
bool isWon(const BoardState& b);
bool bombGuards(const BoardState& b, Hex h);   // some armed bomb touches h
BombResult applyBombs(BoardState& b, Hex from, Hex to, int result);
BombResult previewBombs(const BoardState& b, Hex from, Hex to, int result);
