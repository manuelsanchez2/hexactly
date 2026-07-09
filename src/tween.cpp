#include "tween.h"
#include <cmath>

float lerpf(float a, float b, float t) {
    return a + (b - a) * t;
}

float easeOutQuad(float t) {
    return 1.0f - (1.0f - t) * (1.0f - t);
}

float easeOutBack(float t) {
    const float c1 = 1.70158f;
    const float c3 = c1 + 1.0f;
    float p = t - 1.0f;
    return 1.0f + c3 * p * p * p + c1 * p * p;
}

float wobbleAt(float p) {
    if (p <= 0.0f || p >= 1.0f) return 0.0f;
    return sinf(p * 3.14159265f * 3.0f) * (1.0f - p);
}
