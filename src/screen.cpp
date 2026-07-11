#include "screen.h"
#include "title_screen.h"
#include "options_screen.h"
#include "levelselect_screen.h"
#include "game_screen.h"

int  gStartLevel = 0;
bool gDailyMode  = false;

std::unique_ptr<Screen> createScreen(ScreenType type) {
    switch (type) {
        case ScreenType::TITLE:       return std::make_unique<TitleScreen>();
        case ScreenType::OPTIONS:     return std::make_unique<OptionsScreen>();
        case ScreenType::LEVELSELECT: return std::make_unique<LevelSelectScreen>();
        case ScreenType::GAME:        return std::make_unique<GameScreen>();

        default:                      return std::make_unique<TitleScreen>();
    }
}
