#include "board.h"

int cellIndexAt(const BoardState& b, Hex h) {
    for (int i = 0; i < b.cellCount; i++) {
        if (b.cells[i].exists && hexEqual(b.cells[i].pos, h)) return i;
    }
    return -1;
}

static bool isWalled(const BoardState& b, Hex a, Hex c) {
    for (int i = 0; i < b.wallCount; i++) {
        if ((hexEqual(b.walls[i].a, a) && hexEqual(b.walls[i].b, c)) ||
            (hexEqual(b.walls[i].a, c) && hexEqual(b.walls[i].b, a))) {
            return true;
        }
    }
    return false;
}

int portalBetween(const BoardState& b, Hex a, Hex c) {
    for (int i = 0; i < b.portalCount; i++) {
        if ((hexEqual(b.portals[i].a, a) && hexEqual(b.portals[i].b, c)) ||
            (hexEqual(b.portals[i].a, c) && hexEqual(b.portals[i].b, a))) {
            return i;
        }
    }
    return -1;
}

bool boardAdjacent(const BoardState& b, Hex a, Hex c) {
    if (portalBetween(b, a, c) >= 0) return true;
    return hexAdjacent(a, c) && !isWalled(b, a, c);
}

int tilesRemaining(const BoardState& b) {
    int n = 0;
    for (int i = 0; i < b.cellCount; i++) {
        if (b.cells[i].exists && b.cells[i].value > 0) n++;
    }
    return n;
}

bool anyLegalMove(const BoardState& b) {
    for (int i = 0; i < b.cellCount; i++) {
        if (!b.cells[i].exists || b.cells[i].value == 0 || b.cells[i].isCursed) continue;
        for (int j = 0; j < b.cellCount; j++) {
            if (i == j || !b.cells[j].exists || b.cells[j].value == 0 || b.cells[j].isCursed) continue;
            if (b.cells[i].value == b.cells[j].value &&
                boardAdjacent(b, b.cells[i].pos, b.cells[j].pos)) {
                return true;
            }
        }
    }
    return false;
}

int goalIndex(const BoardState& b) {
    for (int i = 0; i < b.cellCount; i++) {
        if (b.cells[i].exists && b.cells[i].isGoal) return i;
    }
    return -1;
}

bool isWon(const BoardState& b) {
    int g = goalIndex(b);

    if (g >= 0 && b.cells[g].goalValue > 0) {
        return b.cells[g].value == b.cells[g].goalValue;
    }

    if (tilesRemaining(b) != 1) return false;

    int only = -1;
    for (int i = 0; i < b.cellCount; i++) {
        if (b.cells[i].exists && b.cells[i].value > 0) { only = i; break; }
    }
    if (only < 0) return false;

    if (g < 0) return true;
    return b.cells[only].isGoal;
}
