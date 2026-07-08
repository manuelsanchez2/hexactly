#pragma once

#include "screen.h"

class OptionsScreen : public Screen {
public:
    ScreenType update() override;
    void       draw() override;

private:
    bool soundOn = true;
    int  volume  = 70;
};
