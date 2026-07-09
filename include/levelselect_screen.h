#pragma once

#include "screen.h"
#include "ui.h"
#include "progress.h"
#include "layout.h"
#include "editor.h"
#include <vector>
#include <string>

class LevelSelectScreen : public Screen {
public:
    LevelSelectScreen();
    ScreenType update() override;
    void       draw() override;

private:
    Progress progress;
    int      unlocked;

    Layout                   layout;
    Menu                     menu;
    std::vector<std::string> ids;
    Editor                   editor;
    long                     mtime = 0;

    float    wob[100] = { 0 };
    int      hoverPrev = -1;
};
