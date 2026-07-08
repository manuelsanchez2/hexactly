#pragma once

#include "screen.h"

class TitleScreen : public Screen {
public:
    ScreenType update() override;
    void       draw() override;
};
