#pragma once

#include "screen.h"
#include "ui.h"

class TitleScreen : public Screen {
public:
    TitleScreen();
    ScreenType update() override;
    void       draw() override;

private:
    Menu  menu;
    float introT = 0.0f;
};
