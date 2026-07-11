#pragma once

#include "hex.h"

const int MAX_CELLS   = 32;
const int MAX_WALLS   = 16;
const int MAX_PORTALS = 8;

struct Cell {
    Hex  pos;
    int  value;
    bool exists;
    bool isGoal;
    int  goalValue;
};

struct Wall { Hex a, b; };

struct Portal { Hex a, b; };

struct BoardState {
    Cell   cells[MAX_CELLS];
    int    cellCount;
    int    movesLeft;
    Wall   walls[MAX_WALLS];
    int    wallCount;
    Portal portals[MAX_PORTALS];
    int    portalCount;
};

int  cellIndexAt(const BoardState& b, Hex h);
bool boardAdjacent(const BoardState& b, Hex a, Hex c);
int  portalBetween(const BoardState& b, Hex a, Hex c);
int  tilesRemaining(const BoardState& b);
bool anyLegalMove(const BoardState& b);
int  goalIndex(const BoardState& b);
bool isWon(const BoardState& b);
