#include "ui.h"
#include "config.h"

bool buttonClicked(Rectangle bounds) {
    return IsMouseButtonReleased(MOUSE_LEFT_BUTTON) &&
           CheckCollisionPointRec(GetMousePosition(), bounds);
}

void drawButton(Rectangle bounds, const char *text) {
    bool hovered = CheckCollisionPointRec(GetMousePosition(), bounds);

    if (hovered) DrawRectangleRec(bounds, INK);
    else         DrawRectangleRec(bounds, PAPER);
    DrawRectangleLinesEx(bounds, 2, INK);

    int fontSize = 24;
    int textW = MeasureText(text, fontSize);
    DrawText(text,
             (int)(bounds.x + bounds.width  / 2 - textW / 2),
             (int)(bounds.y + bounds.height / 2 - fontSize / 2),
             fontSize, hovered ? PAPER : INK);
}
