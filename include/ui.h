#pragma once

#include "raylib.h"
#include <string>
#include <vector>

bool buttonClicked(Rectangle bounds);

void drawButton(Rectangle bounds, const char *text);

struct Button {
    Rectangle   bounds;
    std::string text;
    float       anim   = 0.0f;
    float       wob    = 0.0f;
    float       slideY = 0.0f;
};

struct Menu {
    std::vector<Button> buttons;
    int     focus   = 0;
    Vector2 offset  = { 0, 0 };
    bool    kbFocus = false;
    int     litPrev = -1;
};

void menuAdd(Menu &menu, Rectangle bounds, const std::string &text);
int  menuUpdate(Menu &menu, float dt);
void menuDraw(const Menu &menu);
