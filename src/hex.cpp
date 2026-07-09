#include "hex.h"
#include <cmath>

static const int HEX_DIRS[6][2] = {
    {+1,  0}, {+1, -1}, { 0, -1},
    {-1,  0}, {-1, +1}, { 0, +1},
};

Vector2 hexToPixel(Hex h, Vector2 origin) {
    float x = HEX_SIZE * (sqrtf(3.0f) * h.q + sqrtf(3.0f) / 2.0f * h.r);
    float y = HEX_SIZE * (1.5f * h.r);
    return (Vector2){ origin.x + x, origin.y + y };
}

Hex pixelToHex(Vector2 p, Vector2 origin) {
    float px = p.x - origin.x, py = p.y - origin.y;
    float qf = (sqrtf(3.0f) / 3.0f * px - 1.0f / 3.0f * py) / HEX_SIZE;
    float rf = (2.0f / 3.0f * py) / HEX_SIZE;
    float sf = -qf - rf;
    float q = roundf(qf), r = roundf(rf), s = roundf(sf);
    float dq = fabsf(q - qf), dr = fabsf(r - rf), ds = fabsf(s - sf);
    if (dq > dr && dq > ds) q = -r - s;
    else if (dr > ds)       r = -q - s;
    return (Hex){ (int)q, (int)r };
}

bool hexEqual(Hex a, Hex b) {
    return a.q == b.q && a.r == b.r;
}

bool hexAdjacent(Hex a, Hex b) {
    int dq = b.q - a.q, dr = b.r - a.r;
    for (int i = 0; i < 6; i++) {
        if (dq == HEX_DIRS[i][0] && dr == HEX_DIRS[i][1]) return true;
    }
    return false;
}
