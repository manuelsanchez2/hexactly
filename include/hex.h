#pragma once

#include "raylib.h"

struct Hex {
    int q;
    int r;
};

const float HEX_SIZE = 42.0f;

Vector2 hexToPixel(Hex h, Vector2 origin);
Hex     pixelToHex(Vector2 p, Vector2 origin);
bool    hexEqual(Hex a, Hex b);
bool    hexAdjacent(Hex a, Hex b);
