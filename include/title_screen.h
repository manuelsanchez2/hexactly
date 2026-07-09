#pragma once

#include "screen.h"
#include "ui.h"
#include "layout.h"
#include "editor.h"
#include <vector>
#include <string>

class TitleScreen : public Screen {
public:
    TitleScreen();
    ScreenType update() override;
    void       draw() override;

private:
    Layout                   layout;
    Menu                     menu;
    std::vector<std::string> ids;
    Editor                   editor;
    long                     mtime = 0;
    float                    introT = 0.0f;
};
