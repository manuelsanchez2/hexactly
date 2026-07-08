#pragma once

#include <memory> 

enum class ScreenType {
    NONE,         
    TITLE,
    OPTIONS,
    LEVELSELECT,
    GAME,
    QUIT         
};

class Screen {
public:
    virtual ~Screen() = default;

    virtual ScreenType update() = 0;

    virtual void draw() = 0;
};

std::unique_ptr<Screen> createScreen(ScreenType type);

extern int gStartLevel;
