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
        if (!b.cells[i].exists || b.cells[i].value == 0) continue;
        for (int j = 0; j < b.cellCount; j++) {
            if (i == j || !b.cells[j].exists) continue;
            if (!boardAdjacent(b, b.cells[i].pos, b.cells[j].pos)) continue;
            // Merge two equal value tiles.
            if (b.cells[j].value > 0 && b.cells[i].value == b.cells[j].value)
                return true;
            // Apply an operator tile to this value tile (daily challenges).
            if (b.cells[j].value == 0 && opAllowed(b.cells[j].op, b.cells[i].value))
                return true;
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

bool bombGuards(const BoardState& b, Hex h) {
    for (int i = 0; i < b.bombCount; i++) {
        if (b.bombs[i].armed && boardAdjacent(b, b.bombs[i].pos, h)) return true;
    }
    return false;
}

BombResult previewBombs(const BoardState& b, Hex from, Hex to, int result) {
    BombResult r = { -1, -1 };
    for (int i = 0; i < b.bombCount; i++) {
        const Bomb& bomb = b.bombs[i];
        if (!bomb.armed) continue;
        if (boardAdjacent(b, bomb.pos, to) && result == bomb.value) {
            r.defusedIdx = i;
        } else if (boardAdjacent(b, bomb.pos, from) || boardAdjacent(b, bomb.pos, to)) {
            r.explodedIdx = i;
        }
    }
    return r;
}

BombResult applyBombs(BoardState& b, Hex from, Hex to, int result) {
    BombResult r = previewBombs(b, from, to, result);
    for (int i = 0; i < b.bombCount; i++) {
        Bomb& bomb = b.bombs[i];
        if (bomb.armed && result == bomb.value && boardAdjacent(b, bomb.pos, to))
            bomb.armed = false;
    }
    return r;
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
