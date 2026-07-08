#pragma once

#include "screen.h"

class GameScreen : public Screen {
public:
    ScreenType update() override;
    void       draw() override;

private:
    int  level  = 0;     
    bool paused = false; 
    bool started = false; 
};
