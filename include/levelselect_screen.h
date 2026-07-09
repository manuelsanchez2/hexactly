#pragma once

#include "screen.h"

class LevelSelectScreen : public Screen {
public:
    ScreenType update() override;
    void       draw() override;
};
