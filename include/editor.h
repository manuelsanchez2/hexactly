#pragma once

#include "layout.h"

struct Editor {
    bool    active   = false;
    int     selected = -1;
    bool    dragging = false;
    Vector2 dragGrab = { 0, 0 };
    float   savedFlash = 0.0f;
};

bool editorUpdate(Editor& ed, Layout& layout);

void editorDrawOverlay(Editor& ed, const Layout& layout);

bool editorTick(Editor& ed, Layout& layout, Menu& menu,
                std::vector<std::string>& ids, long& mtime);
